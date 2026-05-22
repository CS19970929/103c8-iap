#include "main.h"
#include "can_iap.h"
#include "can_iap_protocol.h"
#include <string.h>

#define CAN_IAP_BLOCK_BYTES             ((UINT16)256U)
#define CAN_IAP_FLASH_PAGE_SIZE         ((UINT32)1024U)
#define CAN_IAP_RX_TIMEOUT_10MS         ((UINT16)500U)
#define CAN_IAP_TX_WAIT_LOOP            ((UINT32)60000U)
#define CAN_IAP_CAN_PRESCALER_250K      ((UINT16)4U)

typedef struct
{
	UINT8 state;
	UINT8 node;
	UINT8 last_cmd;
	UINT8 last_error;
	UINT16 expect_seq;
	UINT16 block_seq;
	UINT16 block_bytes;
	UINT16 fw_crc;
	UINT16 running_crc;
	UINT16 idle_10ms;
	UINT32 fw_size;
	UINT32 written;
	UINT8 block[CAN_IAP_BLOCK_BYTES];
} CAN_IAP_RUNTIME;

static CAN_IAP_RUNTIME s_can_iap;

static void can_iap_reset_runtime(UINT8 state)
{
	memset(&s_can_iap, 0, sizeof(s_can_iap));
	s_can_iap.state = state;
	s_can_iap.node = CAN_IAP_NODE_DEFAULT;
	s_can_iap.running_crc = 0xFFFFU;
}

static UINT8 can_iap_send_ack(UINT8 cmd, UINT8 status, UINT8 code)
{
	CanTxMsg tx;
	UINT32 wait;
	UINT8 mailbox;

	memset(&tx, 0, sizeof(tx));
	tx.ExtId = CanIap_AckId(s_can_iap.node);
	tx.IDE = CAN_ID_EXT;
	tx.RTR = CAN_RTR_DATA;
	tx.DLC = 8U;
	CanIap_BuildAck(cmd, status, s_can_iap.expect_seq, code, tx.Data);

	mailbox = CAN_Transmit(CAN1, &tx);
	if (mailbox >= 3U)
	{
		return 0U;
	}

	wait = CAN_IAP_TX_WAIT_LOOP;
	while ((wait > 0U) && (CAN_TransmitStatus(CAN1, mailbox) == CAN_TxStatus_Pending))
	{
		--wait;
	}

	return (CAN_TransmitStatus(CAN1, mailbox) == CAN_TxStatus_Ok) ? 1U : 0U;
}

static UINT8 can_iap_send_nack(UINT8 cmd, UINT8 code)
{
	CanTxMsg tx;
	UINT32 wait;
	UINT8 mailbox;

	memset(&tx, 0, sizeof(tx));
	tx.ExtId = CanIap_AckId(s_can_iap.node);
	tx.IDE = CAN_ID_EXT;
	tx.RTR = CAN_RTR_DATA;
	tx.DLC = 8U;
	CanIap_BuildNack(cmd, s_can_iap.expect_seq, code, tx.Data);

	mailbox = CAN_Transmit(CAN1, &tx);
	if (mailbox >= 3U)
	{
		return 0U;
	}

	wait = CAN_IAP_TX_WAIT_LOOP;
	while ((wait > 0U) && (CAN_TransmitStatus(CAN1, mailbox) == CAN_TxStatus_Pending))
	{
		--wait;
	}

	return (CAN_TransmitStatus(CAN1, mailbox) == CAN_TxStatus_Ok) ? 1U : 0U;
}

static UINT8 can_iap_is_range_valid(UINT32 offset, UINT16 length)
{
	UINT32 start = CAN_IAP_APP_BASE_ADDR + offset;
	UINT32 end = start + (UINT32)length;

	if ((length == 0U) || (start < CAN_IAP_APP_BASE_ADDR) || (end > CAN_IAP_APP_LIMIT_ADDR) || (end < start))
	{
		return 0U;
	}
	return 1U;
}

static UINT8 can_iap_erase_page(UINT32 addr)
{
	FLASH_Status status;

	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	status = FLASH_ErasePage(addr);
	return (status == FLASH_COMPLETE) ? 1U : 0U;
}

static UINT8 can_iap_program_block(UINT32 offset, const UINT8 *data, UINT16 length)
{
	UINT32 addr;
	UINT16 i;
	UINT16 halfword;

	if (can_iap_is_range_valid(offset, length) == 0U)
	{
		return 0U;
	}

	addr = CAN_IAP_APP_BASE_ADDR + offset;
	FLASH_Unlock();

	if ((addr & (CAN_IAP_FLASH_PAGE_SIZE - 1U)) == 0U)
	{
		if (can_iap_erase_page(addr) == 0U)
		{
			FLASH_Lock();
			return 0U;
		}
	}

	for (i = 0U; i < length; i = (UINT16)(i + 2U))
	{
		halfword = data[i] | 0xFF00U;
		if ((UINT16)(i + 1U) < length)
		{
			halfword = (UINT16)(data[i] | ((UINT16)data[i + 1U] << 8));
		}

		FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
		if (FLASH_ProgramHalfWord(addr + i, halfword) != FLASH_COMPLETE)
		{
			FLASH_Lock();
			return 0U;
		}
		if (*(__IO UINT16 *)(addr + i) != halfword)
		{
			FLASH_Lock();
			return 0U;
		}
	}

	FLASH_Lock();
	return 1U;
}

static UINT8 can_iap_write_flag(UINT16 value)
{
	return (FlashWriteOneHalfWord(FLASH_ADDR_UPDATE_FLAG, value) == FLASH_COMPLETE) ? 1U : 0U;
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

	if (can_iap_write_flag(FLASH_TO_IAP_VALUE) == 0U)
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
	next_written = s_can_iap.written + (UINT32)block_len;

	if ((block_seq != s_can_iap.block_seq) || (block_len == 0U) ||
		(block_len > s_can_iap.block_bytes) || (next_written > s_can_iap.fw_size))
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_BAD_PARAM);
		return;
	}

	if (CanIap_Crc16Calc(s_can_iap.block, block_len) != block_crc)
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_CRC);
		return;
	}

	if (can_iap_program_block(s_can_iap.written, s_can_iap.block, block_len) == 0U)
	{
		s_can_iap.state = CAN_IAP_STATE_ERROR;
		s_can_iap.last_error = CAN_IAP_ERR_FLASH;
		(void)can_iap_send_nack(CAN_IAP_CMD_COMMIT, CAN_IAP_ERR_FLASH);
		return;
	}

	s_can_iap.running_crc = CanIap_Crc16Update(s_can_iap.running_crc, s_can_iap.block, block_len);
	s_can_iap.written = next_written;
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

	if (CanIap_IsValidAppVector(CAN_IAP_APP_BASE_ADDR, CAN_IAP_APP_BASE_ADDR + s_can_iap.fw_size) == 0U)
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_END, CAN_IAP_ERR_APP_INVALID);
		return;
	}

	if (can_iap_write_flag(FLASH_TO_APP_VALUE) == 0U)
	{
		(void)can_iap_send_nack(CAN_IAP_CMD_END, CAN_IAP_ERR_FLASH);
		return;
	}

	s_can_iap.state = CAN_IAP_STATE_DONE;
	(void)can_iap_send_ack(CAN_IAP_CMD_END, s_can_iap.state, CAN_IAP_ERR_OK);
	__delay_ms(20);
	MCU_RESET();
}

static void can_iap_handle_abort(void)
{
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
	if (s_can_iap.state != CAN_IAP_STATE_RECEIVING)
	{
		return;
	}

	if (s_can_iap.idle_10ms < CAN_IAP_RX_TIMEOUT_10MS)
	{
		s_can_iap.idle_10ms++;
		return;
	}

	s_can_iap.state = CAN_IAP_STATE_ERROR;
	s_can_iap.last_error = CAN_IAP_ERR_BAD_STATE;
	s_can_iap.block_bytes = 0U;
}

UINT8 CanIap_GetState(void)
{
	return s_can_iap.state;
}
