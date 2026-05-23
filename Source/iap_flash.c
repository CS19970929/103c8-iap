#include "main.h"
#include "iap_flash.h"
#include "can_iap_protocol.h"
#include <string.h>

typedef struct
{
	UINT8 owner;
	UINT16 first_page_len;
	UINT8 first_page[IAP_FLASH_PAGE_SIZE];
} IAP_FLASH_CONTEXT;

static IAP_FLASH_CONTEXT s_iap_flash;

static UINT8 iap_flash_range_valid(UINT32 offset, UINT16 length)
{
	UINT32 start;
	UINT32 end;

	if (length == 0U)
	{
		return 0U;
	}

	start = CAN_IAP_APP_BASE_ADDR + offset;
	end = start + (UINT32)length;
	if ((start < CAN_IAP_APP_BASE_ADDR) || (end > CAN_IAP_APP_LIMIT_ADDR) || (end < start))
	{
		return 0U;
	}

	return 1U;
}

static UINT8 iap_flash_erase_page(UINT32 addr)
{
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	return (FLASH_ErasePage(addr) == FLASH_COMPLETE) ? 1U : 0U;
}

static UINT8 iap_flash_program_halfword(UINT32 addr, UINT16 halfword)
{
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	if (FLASH_ProgramHalfWord(addr, halfword) != FLASH_COMPLETE)
	{
		return 0U;
	}
	return (*(__IO UINT16 *)addr == halfword) ? 1U : 0U;
}

static UINT8 iap_flash_program_bytes(UINT32 addr, const UINT8 *data, UINT16 length)
{
	UINT16 i;
	UINT16 halfword;

	for (i = 0U; i < length; i = (UINT16)(i + 2U))
	{
		halfword = (UINT16)data[i] | 0xFF00U;
		if ((UINT16)(i + 1U) < length)
		{
			halfword = (UINT16)(data[i] | ((UINT16)data[i + 1U] << 8));
		}
		if (iap_flash_program_halfword(addr + i, halfword) == 0U)
		{
			return 0U;
		}
	}

	return 1U;
}

static UINT8 iap_flash_program_direct(UINT32 offset, const UINT8 *data, UINT16 length)
{
	UINT32 addr;

	if ((data == 0) || (iap_flash_range_valid(offset, length) == 0U))
	{
		return 0U;
	}

	addr = CAN_IAP_APP_BASE_ADDR + offset;
	FLASH_Unlock();
	if ((addr & (IAP_FLASH_PAGE_SIZE - 1U)) == 0U)
	{
		if (iap_flash_erase_page(addr) == 0U)
		{
			FLASH_Lock();
			return 0U;
		}
	}

	if (iap_flash_program_bytes(addr, data, length) == 0U)
	{
		FLASH_Lock();
		return 0U;
	}
	FLASH_Lock();
	return 1U;
}

static UINT8 iap_flash_vector_valid_in_buffer(UINT32 image_size)
{
	UINT32 msp;
	UINT32 reset;

	if ((image_size < 8U) || (s_iap_flash.first_page_len < 8U))
	{
		return 0U;
	}

	msp = ((UINT32)s_iap_flash.first_page[0]) |
		  ((UINT32)s_iap_flash.first_page[1] << 8) |
		  ((UINT32)s_iap_flash.first_page[2] << 16) |
		  ((UINT32)s_iap_flash.first_page[3] << 24);
	reset = ((UINT32)s_iap_flash.first_page[4]) |
			((UINT32)s_iap_flash.first_page[5] << 8) |
			((UINT32)s_iap_flash.first_page[6] << 16) |
			((UINT32)s_iap_flash.first_page[7] << 24);

	if ((msp < CAN_IAP_SRAM_BASE_ADDR) || (msp >= CAN_IAP_SRAM_LIMIT_ADDR))
	{
		return 0U;
	}
	if ((reset < CAN_IAP_APP_BASE_ADDR) ||
		(reset >= (CAN_IAP_APP_BASE_ADDR + image_size)) ||
		((reset & 0x1U) == 0U))
	{
		return 0U;
	}

	return 1U;
}

static UINT8 iap_flash_program_first_page_last(UINT32 image_size)
{
	UINT16 length;
	UINT16 tail_len;

	if ((image_size == 0U) || (image_size > (CAN_IAP_APP_LIMIT_ADDR - CAN_IAP_APP_BASE_ADDR)))
	{
		return 0U;
	}

	length = (image_size > IAP_FLASH_PAGE_SIZE) ? (UINT16)IAP_FLASH_PAGE_SIZE : (UINT16)image_size;
	if ((length < 8U) || (s_iap_flash.first_page_len < length) ||
		(iap_flash_vector_valid_in_buffer(image_size) == 0U))
	{
		return 0U;
	}

	FLASH_Unlock();
	if (iap_flash_erase_page(CAN_IAP_APP_BASE_ADDR) == 0U)
	{
		FLASH_Lock();
		return 0U;
	}

	tail_len = (UINT16)(length - 8U);
	if ((tail_len > 0U) &&
		(iap_flash_program_bytes(CAN_IAP_APP_BASE_ADDR + 8U, &s_iap_flash.first_page[8], tail_len) == 0U))
	{
		FLASH_Lock();
		return 0U;
	}

	if ((iap_flash_program_bytes(CAN_IAP_APP_BASE_ADDR + 4U, &s_iap_flash.first_page[4], 4U) == 0U) ||
		(iap_flash_program_bytes(CAN_IAP_APP_BASE_ADDR, &s_iap_flash.first_page[0], 4U) == 0U))
	{
		FLASH_Lock();
		return 0U;
	}

	FLASH_Lock();
	return 1U;
}

UINT8 IapFlash_Begin(UINT8 owner)
{
	if ((owner == 0U) || ((s_iap_flash.owner != 0U) && (s_iap_flash.owner != owner)))
	{
		return 0U;
	}

	s_iap_flash.owner = owner;
	s_iap_flash.first_page_len = 0U;
	memset(s_iap_flash.first_page, 0xFF, sizeof(s_iap_flash.first_page));

	FLASH_Unlock();
	if (iap_flash_erase_page(CAN_IAP_APP_BASE_ADDR) == 0U)
	{
		FLASH_Lock();
		s_iap_flash.owner = 0U;
		return 0U;
	}
	FLASH_Lock();
	return 1U;
}

UINT8 IapFlash_Write(UINT8 owner, UINT32 offset, const UINT8 *data, UINT16 length)
{
	UINT16 first_len;
	UINT32 first_end;

	if ((s_iap_flash.owner != owner) || (data == 0) || (iap_flash_range_valid(offset, length) == 0U))
	{
		return 0U;
	}

	if (offset < IAP_FLASH_PAGE_SIZE)
	{
		first_len = (UINT16)(IAP_FLASH_PAGE_SIZE - offset);
		if (first_len > length)
		{
			first_len = length;
		}
		memcpy(&s_iap_flash.first_page[offset], data, first_len);
		first_end = offset + (UINT32)first_len;
		if (first_end > s_iap_flash.first_page_len)
		{
			s_iap_flash.first_page_len = (UINT16)first_end;
		}

		if (first_len == length)
		{
			return 1U;
		}

		return iap_flash_program_direct(offset + first_len, &data[first_len], (UINT16)(length - first_len));
	}

	return iap_flash_program_direct(offset, data, length);
}

UINT8 IapFlash_Finish(UINT8 owner, UINT32 image_size)
{
	if (s_iap_flash.owner != owner)
	{
		return 0U;
	}

	if (iap_flash_program_first_page_last(image_size) == 0U)
	{
		return 0U;
	}

	if (CanIap_IsValidAppVector(CAN_IAP_APP_BASE_ADDR, CAN_IAP_APP_BASE_ADDR + image_size) == 0U)
	{
		return 0U;
	}

	s_iap_flash.owner = 0U;
	return 1U;
}

void IapFlash_Abort(UINT8 owner)
{
	if (s_iap_flash.owner == owner)
	{
		s_iap_flash.owner = 0U;
		s_iap_flash.first_page_len = 0U;
	}
}
