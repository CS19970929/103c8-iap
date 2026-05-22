#include "can_iap_protocol.h"

UINT16 CanIap_Crc16Update(UINT16 crc, const UINT8 *data, UINT16 length)
{
	UINT16 i;
	UINT8 bit;

	if (data == 0)
	{
		return crc;
	}

	for (i = 0U; i < length; ++i)
	{
		crc ^= data[i];
		for (bit = 0U; bit < 8U; ++bit)
		{
			if ((crc & 0x0001U) != 0U)
			{
				crc = (UINT16)((crc >> 1) ^ 0xA001U);
			}
			else
			{
				crc = (UINT16)(crc >> 1);
			}
		}
	}

	return crc;
}

UINT16 CanIap_Crc16Calc(const UINT8 *data, UINT16 length)
{
	return CanIap_Crc16Update(0xFFFFU, data, length);
}

UINT32 CanIap_CtrlId(UINT8 node)
{
	return CAN_IAP_CTRL_BASE_ID | (UINT32)node;
}

UINT32 CanIap_AckId(UINT8 node)
{
	return CAN_IAP_ACK_BASE_ID | (UINT32)node;
}

UINT32 CanIap_DataId(UINT8 node, UINT16 seq)
{
	return CAN_IAP_DATA_BASE_ID | ((UINT32)seq << 8) | (UINT32)node;
}

UINT16 CanIap_ReadBe16(const UINT8 *data)
{
	return (UINT16)(((UINT16)data[0] << 8) | data[1]);
}

UINT32 CanIap_ReadBe32(const UINT8 *data)
{
	return ((UINT32)data[0] << 24) |
		   ((UINT32)data[1] << 16) |
		   ((UINT32)data[2] << 8) |
		   (UINT32)data[3];
}

void CanIap_WriteBe16(UINT8 *data, UINT16 value)
{
	data[0] = (UINT8)(value >> 8);
	data[1] = (UINT8)value;
}

UINT8 CanIap_IsValidAppVector(UINT32 app_addr, UINT32 app_limit)
{
	UINT32 msp;
	UINT32 reset;

	if ((app_addr < CAN_IAP_APP_BASE_ADDR) || (app_addr >= app_limit))
	{
		return 0U;
	}

	msp = *(__IO UINT32 *)app_addr;
	reset = *(__IO UINT32 *)(app_addr + 4U);
	if ((msp < CAN_IAP_SRAM_BASE_ADDR) || (msp >= CAN_IAP_SRAM_LIMIT_ADDR))
	{
		return 0U;
	}
	if ((reset < app_addr) || (reset >= app_limit) || ((reset & 0x1U) == 0U))
	{
		return 0U;
	}
	return 1U;
}

void CanIap_BuildAck(UINT8 cmd, UINT8 status, UINT16 expect_seq, UINT8 code, UINT8 data[8])
{
	data[0] = CAN_IAP_CMD_ACK;
	data[1] = cmd;
	data[2] = status;
	CanIap_WriteBe16(&data[3], expect_seq);
	data[5] = code;
	data[6] = 0xFFU;
	data[7] = 0xFFU;
}

void CanIap_BuildNack(UINT8 cmd, UINT16 expect_seq, UINT8 code, UINT8 data[8])
{
	data[0] = CAN_IAP_CMD_NACK;
	data[1] = cmd;
	data[2] = CAN_IAP_ERR_BAD_STATE;
	CanIap_WriteBe16(&data[3], expect_seq);
	data[5] = code;
	data[6] = 0xFFU;
	data[7] = 0xFFU;
}

