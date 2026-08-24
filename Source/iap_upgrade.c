#include "main.h"
#include "iap_upgrade.h"
#include "can_iap_protocol.h"
#include "boot_control.h"
#include "iap_flash.h"

#define IAP_UPGRADE_LEGACY_BLOCK_BYTES  ((UINT16)1024U)
#define IAP_UPGRADE_TIMEOUT_10MS        ((UINT16)6000U)
#define IAP_UPGRADE_MAX_BLOCKS          ((UINT16)((CAN_IAP_APP_LIMIT_ADDR - CAN_IAP_APP_BASE_ADDR) / IAP_UPGRADE_LEGACY_BLOCK_BYTES))

typedef struct
{
	UINT8 state;
	UINT8 last_error;
	UINT16 block_count;
	UINT16 idle_10ms;
	UINT16 running_crc;
	UINT32 written_bytes;
} IAP_UPGRADE_RUNTIME;

static IAP_UPGRADE_RUNTIME s_iap_upgrade;

static void iap_upgrade_set_error(UINT8 error)
{
	IapFlash_Abort(IAP_FLASH_OWNER_SERIAL);
	s_iap_upgrade.state = IAP_UPGRADE_STATE_ERROR;
	s_iap_upgrade.last_error = error;
	if (Flash_Faultcnt < (UINT8)0xFFU)
	{
		Flash_Faultcnt++;
	}
}

static void iap_upgrade_reset(UINT8 state)
{
	s_iap_upgrade.state = state;
	s_iap_upgrade.last_error = IAP_UPGRADE_ERR_OK;
	s_iap_upgrade.block_count = 0U;
	s_iap_upgrade.idle_10ms = 0U;
	s_iap_upgrade.running_crc = 0xFFFFU;
	s_iap_upgrade.written_bytes = 0U;
	u8FlashReceiveCnt = 0U;
}

static UINT16 iap_upgrade_serial_payload_length(UINT16 declared_length, UINT8 byte_count)
{
	if (byte_count != 0U)
	{
		if (declared_length > ((UINT16)0xFFFFU / 2U))
		{
			return 0U;
		}
		if ((UINT16)(declared_length * 2U) != (UINT16)byte_count)
		{
			return 0U;
		}
		return (UINT16)byte_count;
	}

	return declared_length;
}

void IapUpgrade_Init(void)
{
	iap_upgrade_reset(IAP_UPGRADE_STATE_IDLE);
}

UINT8 IapUpgrade_SerialConnect(UINT16 value_count, const UINT8 *payload)
{
	(void)payload;

	if (value_count != 1U)
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_BAD_PARAM);
		return 0U;
	}

	IapFlash_Abort(IAP_FLASH_OWNER_SERIAL);
	iap_upgrade_reset(IAP_UPGRADE_STATE_READY);
	BootCtrl_ClearRequest();
	if (IapFlash_Begin(IAP_FLASH_OWNER_SERIAL) == 0U)
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_FLASH);
		return 0U;
	}

	return 1U;
}

UINT8 IapUpgrade_SerialWriteBlock(UINT16 declared_length, UINT8 byte_count, const UINT8 *payload)
{
	UINT16 length;
	UINT32 offset;

	length = iap_upgrade_serial_payload_length(declared_length, byte_count);
	if ((length == 0U) || (length > IAP_UPGRADE_LEGACY_BLOCK_BYTES) || (payload == 0))
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_BAD_PARAM);
		return 0U;
	}

	if (s_iap_upgrade.state == IAP_UPGRADE_STATE_IDLE)
	{
		if (IapUpgrade_SerialConnect(1U, 0) == 0U)
		{
			return 0U;
		}
	}
	if ((s_iap_upgrade.state != IAP_UPGRADE_STATE_READY) &&
		(s_iap_upgrade.state != IAP_UPGRADE_STATE_RECEIVING))
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_BAD_STATE);
		return 0U;
	}
	if (s_iap_upgrade.block_count >= IAP_UPGRADE_MAX_BLOCKS)
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_RANGE);
		return 0U;
	}

	offset = (UINT32)s_iap_upgrade.block_count * (UINT32)IAP_UPGRADE_LEGACY_BLOCK_BYTES;
	if (IapFlash_Write(IAP_FLASH_OWNER_SERIAL, offset, payload, length) == 0U)
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_FLASH);
		return 0U;
	}

	s_iap_upgrade.state = IAP_UPGRADE_STATE_RECEIVING;
	s_iap_upgrade.running_crc = CanIap_Crc16Update(s_iap_upgrade.running_crc, payload, length);
	s_iap_upgrade.written_bytes = offset + (UINT32)length;
	s_iap_upgrade.block_count++;
	s_iap_upgrade.idle_10ms = 0U;
	u8FlashReceiveCnt = (UINT8)s_iap_upgrade.block_count;
	return 1U;
}

UINT8 IapUpgrade_SerialComplete(UINT16 value_count, const UINT8 *payload)
{
	(void)payload;

	if (value_count != 1U)
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_BAD_PARAM);
		return 0U;
	}
	if ((s_iap_upgrade.state != IAP_UPGRADE_STATE_RECEIVING) || (s_iap_upgrade.block_count == 0U) ||
		(s_iap_upgrade.written_bytes < 8U))
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_BAD_STATE);
		return 0U;
	}
	if (IapFlash_Finish(IAP_FLASH_OWNER_SERIAL, s_iap_upgrade.written_bytes) == 0U)
	{
		iap_upgrade_set_error(IAP_UPGRADE_ERR_APP_INVALID);
		return 0U;
	}

	s_iap_upgrade.state = IAP_UPGRADE_STATE_DONE;
	s_iap_upgrade.idle_10ms = 0U;
	return 1U;
}

void IapUpgrade_10msTask(void)
{
	if ((s_iap_upgrade.state != IAP_UPGRADE_STATE_READY) &&
		(s_iap_upgrade.state != IAP_UPGRADE_STATE_RECEIVING))
	{
		return;
	}

	if (s_iap_upgrade.idle_10ms < IAP_UPGRADE_TIMEOUT_10MS)
	{
		s_iap_upgrade.idle_10ms++;
		return;
	}

	iap_upgrade_set_error(IAP_UPGRADE_ERR_TIMEOUT);
	u8FlashReceiveCnt = 0U;
}

UINT8 IapUpgrade_GetState(void)
{
	return s_iap_upgrade.state;
}

UINT8 IapUpgrade_GetLastError(void)
{
	return s_iap_upgrade.last_error;
}

UINT16 IapUpgrade_GetBlockCount(void)
{
	return s_iap_upgrade.block_count;
}

UINT32 IapUpgrade_GetWrittenBytes(void)
{
	return s_iap_upgrade.written_bytes;
}
