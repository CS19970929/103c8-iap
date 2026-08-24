#include "main.h"
#include "can_iap.h"
#include "can_iap_protocol.h"
#include "boot_control.h"
#include "iap_flash.h"
#include <string.h>

#define CAN_IAP_BLOCK_BYTES             ((UINT16)256U)
#define CAN_IAP_RX_TIMEOUT_10MS         ((UINT16)500U)
#define CAN_IAP_TX_WAIT_LOOP            ((UINT32)60000U)
#define CAN_IAP_HEARTBEAT_TX_WAIT_LOOP  ((UINT32)3000U)
#define CAN_IAP_CAN_PRESCALER_250K      ((UINT16)18U) /* PCLK1 36MHz / 8tq / 18 = 250k */

typedef struct
{
	UINT8 state;
	UINT8 node;
	UINT8 last_cmd;
	UINT8 last_error;
	UINT8 last_commit_valid;
	UINT16 expect_seq;
	UINT16 block_seq;
	UINT16 block_bytes;
	UINT16 fw_crc;
	UINT16 running_crc;
	UINT16 idle_10ms;
	UINT16 last_commit_seq;
	UINT16 last_commit_len;
	UINT16 last_commit_crc;
	UINT32 fw_size;
	UINT32 written;
	UINT8 block[CAN_IAP_BLOCK_BYTES];
} CAN_IAP_RUNTIME;

static CAN_IAP_RUNTIME s_can_iap;
static UINT8 s_can_iap_heartbeat_seq;
static UINT16 s_can_iap_heartbeat_10ms;

static void can_iap_reset_runtime(UINT8 state)
{
	memset(&s_can_iap, 0, sizeof(s_can_iap));
	s_can_iap.state = state;
	s_can_iap.node = CAN_IAP_NODE_DEFAULT;
	s_can_iap.running_crc = 0xFFFFU;
}

static void can_iap_enter_error(UINT8 error)
{
	IapFlash_Abort(IAP_FLASH_OWNER_CAN);
	s_can_iap.state = CAN_IAP_STATE_ERROR;
	s_can_iap.last_error = error;
	s_can_iap.block_bytes = 0U;
}

static UINT8 can_iap_transmit(CanTxMsg *tx, UINT32 wait_loop)
{
	UINT32 wait;
	UINT8 status;
	UINT8 mailbox;

	mailbox = CAN_Transmit(CAN1, tx);
	if (mailbox >= 3U)
	{
		return 0U;
	}

	wait = wait_loop;
	while ((wait > 0U) && (CAN_TransmitStatus(CAN1, mailbox) == CAN_TxStatus_Pending))
	{
		--wait;
	}

	status = CAN_TransmitStatus(CAN1, mailbox);
	if (status == CAN_TxStatus_Ok)
	{
		return 1U;
	}

	CAN_CancelTransmit(CAN1, mailbox);
	return 0U;
}

static UINT8 can_iap_send_ack(UINT8 cmd, UINT8 status, UINT8 code)
{
	CanTxMsg tx;

	s_can_iap.last_error = code;
	memset(&tx, 0, sizeof(tx));
	tx.ExtId = CanIap_AckId(s_can_iap.node);
	tx.IDE = CAN_ID_EXT;
	tx.RTR = CAN_RTR_DATA;
	tx.DLC = 8U;
	CanIap_BuildAck(cmd, status, s_can_iap.expect_seq, code, tx.Data);

	return can_iap_transmit(&tx, CAN_IAP_TX_WAIT_LOOP);
}

static UINT8 can_iap_send_nack(UINT8 cmd, UINT8 code)
{
	CanTxMsg tx;

	s_can_iap.last_error = code;
	memset(&tx, 0, sizeof(tx));
	tx.ExtId = CanIap_AckId(s_can_iap.node);
	tx.IDE = CAN_ID_EXT;
	tx.RTR = CAN_RTR_DATA;
	tx.DLC = 8U;
	CanIap_BuildNack(cmd, s_can_iap.expect_seq, code, tx.Data);

	return can_iap_transmit(&tx, CAN_IAP_TX_WAIT_LOOP);
}

static void can_iap_send_heartbeat(void)
{
	CanTxMsg tx;

	memset(&tx, 0, sizeof(tx));
	tx.StdId = CAN_IAP_HEARTBEAT_STD_ID;
	tx.IDE = CAN_ID_STD;
	tx.RTR = CAN_RTR_DATA;
	tx.DLC = 8U;
	tx.Data[0] = 0x49U; /* 'I' */
	tx.Data[1] = 0x41U; /* 'A' */
	tx.Data[2] = CAN_IAP_PROTOCOL_VERSION;
	tx.Data[3] = s_can_iap.node;
	tx.Data[4] = s_can_iap.state;
	tx.Data[5] = s_can_iap.last_cmd;
	tx.Data[6] = s_can_iap.last_error;
	tx.Data[7] = s_can_iap_heartbeat_seq++;

	(void)can_iap_transmit(&tx, CAN_IAP_HEARTBEAT_TX_WAIT_LOOP);
}

static void can_iap_handle_hello(const CanRxMsg *rx)
{
	if ((rx->DLC != 8U) || (rx->Data[1] != CAN_IAP_PROTOCOL_VERSION))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_HELLO, CAN_IAP_ERR_BAD_PARAM);
		return;
	}

	s_can_iap.node = rx->Data[2];
	if ((s_can_iap.node == 0U) || (s_can_iap.node > 0x7FU))
	{
		s_can_iap.node = CAN_IAP_NODE_DEFAULT;
	}
	(void)can_iap_send_ack(CAN_IAP_CMD_HELLO, s_can_iap.state, CAN_IAP_ERR_OK);
}

static void can_iap_handle_start(const CanRxMsg *rx)
{
	UINT32 size;
	UINT16 crc;

	if ((rx->DLC != 8U) || (rx->Data[1] != CAN_IAP_PROTOCOL_VERSION))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_START, CAN_IAP_ERR_BAD_PARAM);
		return;
	}

	size = CanIap_ReadBe32(&rx->Data[2]);
	crc = CanIap_ReadBe16(&rx->Data[6]);
	if ((size == 0U) || (size > (CAN_IAP_APP_LIMIT_ADDR - CAN_IAP_APP_BASE_ADDR)))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_START, CAN_IAP_ERR_BAD_PARAM);
		return;
	}

	BootCtrl_ClearRequest();
	if (IapFlash_Begin(IAP_FLASH_OWNER_CAN) == 0U)
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_START, CAN_IAP_ERR_FLASH);
		return;
	}

	can_iap_reset_runtime(CAN_IAP_STATE_RECEIVING);
	s_can_iap.node = (UINT8)(rx->ExtId & 0xFFU);
	if (s_can_iap.node == 0U)
	{
		s_can_iap.node = CAN_IAP_NODE_DEFAULT;
	}
	s_can_iap.fw_size = size;
	s_can_iap.fw_crc = crc;
	(void)can_iap_send_ack(CAN_IAP_CMD_START, s_can_iap.state, CAN_IAP_ERR_OK);
}

static void can_iap_handle_data(const CanRxMsg *rx)
{
	UINT16 seq;

	if (s_can_iap.state != CAN_IAP_STATE_RECEIVING)
	{
		(void)can_iap_send_nack(0U, CAN_IAP_ERR_BAD_STATE);
		return;
	}

	seq = (UINT16)((rx->ExtId >> 8) & 0xFFFFU);
	if ((rx->DLC != 8U) || (seq != s_can_iap.expect_seq) ||
		((UINT16)(s_can_iap.block_bytes + 8U) > CAN_IAP_BLOCK_BYTES))
	{
		(void)can_iap_send_nack(0U, CAN_IAP_ERR_BAD_SEQ);
		return;
	}

	memcpy(&s_can_iap.block[s_can_iap.block_bytes], rx->Data, 8U);
	s_can_iap.block_bytes = (UINT16)(s_can_iap.block_bytes + 8U);
	s_can_iap.expect_seq++;
	s_can_iap.idle_10ms = 0U;
}

static UINT8 can_iap_is_duplicate_commit(UINT16 block_seq, UINT16 block_len, UINT16 block_crc)
{
	if ((s_can_iap.last_commit_valid != 0U) && (s_can_iap.block_bytes == 0U) &&
		(block_seq == s_can_iap.last_commit_seq) &&
		(block_len == s_can_iap.last_commit_len) &&
		(block_crc == s_can_iap.last_commit_crc))
	{
		return 1U;
	}
	return 0U;
}

static void can_iap_handle_commit(const CanRxMsg *rx)
{
	UINT16 block_seq;
	UINT16 block_len;
	UINT16 block_crc;
	UINT32 next_written;

	if ((s_can_iap.state != CAN_IAP_STATE_RECEIVING) || (rx->DLC != 8U))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_BAD_STATE);
		return;
	}

	block_seq = CanIap_ReadBe16(&rx->Data[1]);
	block_len = CanIap_ReadBe16(&rx->Data[3]);
	block_crc = CanIap_ReadBe16(&rx->Data[5]);

	if (can_iap_is_duplicate_commit(block_seq, block_len, block_crc) != 0U)
	{
		s_can_iap.idle_10ms = 0U;
		(void)can_iap_send_ack(CAN_IAP_CMD_COMMIT, s_can_iap.state, CAN_IAP_ERR_OK);
		return;
	}

	next_written = s_can_iap.written + (UINT32)block_len;
	if ((block_seq != s_can_iap.block_seq) || (block_len == 0U) ||
		(block_len > s_can_iap.block_bytes) || (next_written > s_can_iap.fw_size))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_BAD_PARAM);
		return;
	}

	/* Non-final blocks must commit all received bytes. The final block may carry
	 * up to seven CAN padding bytes because DATA frames are always 8 bytes. */
	if (((next_written < s_can_iap.fw_size) && (block_len != s_can_iap.block_bytes)) ||
		((next_written == s_can_iap.fw_size) && ((UINT16)(s_can_iap.block_bytes - block_len) > 7U)))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_BAD_PARAM);
		return;
	}

	if (CanIap_Crc16Calc(s_can_iap.block, block_len) != block_crc)
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_CRC);
		return;
	}

	if (IapFlash_Write(IAP_FLASH_OWNER_CAN, s_can_iap.written, s_can_iap.block, block_len) == 0U)
	{
		can_iap_enter_error(CAN_IAP_ERR_FLASH);
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_FLASH);
		return;
	}

	s_can_iap.running_crc = CanIap_Crc16Update(s_can_iap.running_crc, s_can_iap.block, block_len);
	s_can_iap.written = next_written;
	s_can_iap.last_commit_valid = 1U;
	s_can_iap.last_commit_seq = block_seq;
	s_can_iap.last_commit_len = block_len;
	s_can_iap.last_commit_crc = block_crc;
	s_can_iap.block_seq++;
	s_can_iap.block_bytes = 0U;
	s_can_iap.idle_10ms = 0U;
	(void)can_iap_send_ack(CAN_IAP_CMD_COMMIT, s_can_iap.state, CAN_IAP_ERR_OK);
}

static void can_iap_handle_end(const CanRxMsg *rx)
{
	UINT16 frame_count;
	UINT16 crc;

	if ((s_can_iap.state != CAN_IAP_STATE_RECEIVING) || (rx->DLC != 8U))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_END, CAN_IAP_ERR_BAD_STATE);
		return;
	}

	frame_count = CanIap_ReadBe16(&rx->Data[1]);
	crc = CanIap_ReadBe16(&rx->Data[3]);
	if ((frame_count != s_can_iap.expect_seq) || (s_can_iap.written != s_can_iap.fw_size) ||
		(crc != s_can_iap.fw_crc) || (s_can_iap.running_crc != s_can_iap.fw_crc))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_END, CAN_IAP_ERR_CRC);
		return;
	}

	if (IapFlash_Finish(IAP_FLASH_OWNER_CAN, s_can_iap.fw_size) == 0U)
	{
		can_iap_enter_error(CAN_IAP_ERR_APP_INVALID);
		(void)can_iap_send_nack(CAN_IAP_CMD_END, CAN_IAP_ERR_APP_INVALID);
		return;
	}

	s_can_iap.state = CAN_IAP_STATE_DONE;
	(void)can_iap_send_ack(CAN_IAP_CMD_END, s_can_iap.state, CAN_IAP_ERR_OK);
	__delay_ms(20);
	MCU_RESET();
}

static void can_iap_handle_abort(void)
{
	IapFlash_Abort(IAP_FLASH_OWNER_CAN);
	can_iap_reset_runtime(CAN_IAP_STATE_IDLE);
	(void)can_iap_send_ack(CAN_IAP_CMD_ABORT, s_can_iap.state, CAN_IAP_ERR_OK);
}

static void can_iap_handle_ctrl(const CanRxMsg *rx)
{
	UINT8 cmd;

	if (rx->DLC == 0U)
	{
		return;
	}

	cmd = rx->Data[0];
	s_can_iap.last_cmd = cmd;
	switch (cmd)
	{
	case CAN_IAP_CMD_HELLO:
		can_iap_handle_hello(rx);
		break;
	case CAN_IAP_CMD_START:
		can_iap_handle_start(rx);
		break;
	case CAN_IAP_CMD_COMMIT:
		can_iap_handle_commit(rx);
		break;
	case CAN_IAP_CMD_END:
		can_iap_handle_end(rx);
		break;
	case CAN_IAP_CMD_ABORT:
		can_iap_handle_abort();
		break;
	default:
		(void)can_iap_send_nack(cmd, CAN_IAP_ERR_BAD_CMD);
		break;
	}
}

static void can_iap_handle_rx(const CanRxMsg *rx)
{
	UINT8 node;

	if ((rx->IDE != CAN_ID_EXT) || (rx->RTR != CAN_RTR_DATA))
	{
		return;
	}

	node = (UINT8)(rx->ExtId & 0xFFU);
	if ((node != s_can_iap.node) && (node != CAN_IAP_NODE_DEFAULT))
	{
		return;
	}

	if (rx->ExtId == CanIap_CtrlId(node))
	{
		s_can_iap.node = node;
		can_iap_handle_ctrl(rx);
	}
	else if ((rx->ExtId & CAN_IAP_DATA_ID_MASK) == CAN_IAP_DATA_BASE_ID)
	{
		s_can_iap.node = node;
		can_iap_handle_data(rx);
	}
}

static void can_iap_gpio_init(void)
{
	GPIO_InitTypeDef gpio;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_ResetBits(GPIO_CMNT_EN, PIN_CMNT_EN);
	gpio.GPIO_Pin = PIN_CMNT_EN;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIO_CMNT_EN, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_11;
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_12;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);
}

static void can_iap_can_init(void)
{
	CAN_InitTypeDef can;
	CAN_FilterInitTypeDef filter;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
	CAN_DeInit(CAN1);
	CAN_StructInit(&can);
	can.CAN_TTCM = DISABLE;
	can.CAN_ABOM = ENABLE;
	can.CAN_AWUM = DISABLE;
	can.CAN_NART = DISABLE;
	can.CAN_RFLM = DISABLE;
	can.CAN_TXFP = DISABLE;
	can.CAN_Mode = CAN_Mode_Normal;
	can.CAN_SJW = CAN_SJW_1tq;
	can.CAN_BS1 = CAN_BS1_5tq;
	can.CAN_BS2 = CAN_BS2_2tq;
	can.CAN_Prescaler = CAN_IAP_CAN_PRESCALER_250K;
	(void)CAN_Init(CAN1, &can);

	/* Keep the existing accept-all filter so deployed tools and node-selection
	 * behavior stay byte-for-byte compatible. Filtering is performed in software. */
	filter.CAN_FilterNumber = 0U;
	filter.CAN_FilterMode = CAN_FilterMode_IdMask;
	filter.CAN_FilterScale = CAN_FilterScale_32bit;
	filter.CAN_FilterIdHigh = 0U;
	filter.CAN_FilterIdLow = 0U;
	filter.CAN_FilterMaskIdHigh = 0U;
	filter.CAN_FilterMaskIdLow = 0U;
	filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
	filter.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&filter);
}

void CanIap_Init(void)
{
	can_iap_reset_runtime(CAN_IAP_STATE_IDLE);
	can_iap_gpio_init();
	can_iap_can_init();
}

void CanIap_Task(void)
{
	CanRxMsg rx;

	while (CAN_MessagePending(CAN1, CAN_FIFO0) > 0U)
	{
		CAN_Receive(CAN1, CAN_FIFO0, &rx);
		can_iap_handle_rx(&rx);
	}
}

void CanIap_10msTask(void)
{
	if (++s_can_iap_heartbeat_10ms >= CAN_IAP_HEARTBEAT_PERIOD_10MS)
	{
		s_can_iap_heartbeat_10ms = 0U;
		can_iap_send_heartbeat();
	}

	if (s_can_iap.state != CAN_IAP_STATE_RECEIVING)
	{
		return;
	}

	if (s_can_iap.idle_10ms < CAN_IAP_RX_TIMEOUT_10MS)
	{
		s_can_iap.idle_10ms++;
		return;
	}

	can_iap_enter_error(CAN_IAP_ERR_BAD_STATE);
}

UINT8 CanIap_GetState(void)
{
	return s_can_iap.state;
}
