#include "main.h"
#include "iap_flash.h"
#include "can_iap_protocol.h"
#include <string.h>

#define IAP_FLASH_APP_BYTES              (CAN_IAP_APP_LIMIT_ADDR - CAN_IAP_APP_BASE_ADDR)
#define IAP_FLASH_APP_PAGE_COUNT         ((IAP_FLASH_APP_BYTES + IAP_FLASH_PAGE_SIZE - 1U) / IAP_FLASH_PAGE_SIZE)
#define IAP_FLASH_ERASE_MAP_BYTES        ((IAP_FLASH_APP_PAGE_COUNT + 7U) / 8U)

typedef struct
{
	UINT8 owner;
	UINT8 pending_valid;
	UINT8 pending_byte;
	UINT16 first_page_len;
	UINT32 pending_offset;
	UINT8 erased_map[IAP_FLASH_ERASE_MAP_BYTES];
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

static UINT8 iap_flash_page_is_erased(UINT32 page_index)
{
	UINT32 byte_index;
	UINT8 bit_mask;

	if (page_index >= IAP_FLASH_APP_PAGE_COUNT)
	{
		return 0U;
	}

	byte_index = page_index >> 3;
	bit_mask = (UINT8)(1U << (page_index & 7U));
	return ((s_iap_flash.erased_map[byte_index] & bit_mask) != 0U) ? 1U : 0U;
}

static void iap_flash_mark_page_erased(UINT32 page_index)
{
	UINT32 byte_index;
	UINT8 bit_mask;

	if (page_index >= IAP_FLASH_APP_PAGE_COUNT)
	{
		return;
	}

	byte_index = page_index >> 3;
	bit_mask = (UINT8)(1U << (page_index & 7U));
	s_iap_flash.erased_map[byte_index] |= bit_mask;
}

static UINT8 iap_flash_erase_page(UINT32 addr)
{
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	return (FLASH_ErasePage(addr) == FLASH_COMPLETE) ? 1U : 0U;
}

static UINT8 iap_flash_ensure_page_erased(UINT32 offset)
{
	UINT32 page_index;
	UINT32 page_addr;
	UINT8 ok;

	page_index = offset / IAP_FLASH_PAGE_SIZE;
	if (page_index >= IAP_FLASH_APP_PAGE_COUNT)
	{
		return 0U;
	}
	if (iap_flash_page_is_erased(page_index) != 0U)
	{
		return 1U;
	}

	page_addr = CAN_IAP_APP_BASE_ADDR + page_index * IAP_FLASH_PAGE_SIZE;
	FLASH_Unlock();
	ok = iap_flash_erase_page(page_addr);
	FLASH_Lock();
	if (ok != 0U)
	{
		iap_flash_mark_page_erased(page_index);
	}
	return ok;
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

	if ((data == 0) || ((addr & 1U) != 0U))
	{
		return 0U;
	}

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

static UINT8 iap_flash_program_even_range(UINT32 offset, const UINT8 *data, UINT16 length)
{
	UINT32 page_offset;
	UINT16 chunk;

	if ((data == 0) || (length == 0U) || ((offset & 1U) != 0U) || ((length & 1U) != 0U))
	{
		return 0U;
	}

	while (length > 0U)
	{
		page_offset = offset & (IAP_FLASH_PAGE_SIZE - 1U);
		chunk = (UINT16)(IAP_FLASH_PAGE_SIZE - page_offset);
		if (chunk > length)
		{
			chunk = length;
		}

		if ((chunk & 1U) != 0U)
		{
			return 0U;
		}
		if (iap_flash_ensure_page_erased(offset) == 0U)
		{
			return 0U;
		}

		FLASH_Unlock();
		if (iap_flash_program_bytes(CAN_IAP_APP_BASE_ADDR + offset, data, chunk) == 0U)
		{
			FLASH_Lock();
			return 0U;
		}
		FLASH_Lock();

		offset += (UINT32)chunk;
		data += chunk;
		length = (UINT16)(length - chunk);
	}

	return 1U;
}

static UINT8 iap_flash_flush_pending(void)
{
	UINT16 halfword;

	if (s_iap_flash.pending_valid == 0U)
	{
		return 1U;
	}
	if (iap_flash_ensure_page_erased(s_iap_flash.pending_offset) == 0U)
	{
		return 0U;
	}

	halfword = (UINT16)s_iap_flash.pending_byte | 0xFF00U;
	FLASH_Unlock();
	if (iap_flash_program_halfword(CAN_IAP_APP_BASE_ADDR + s_iap_flash.pending_offset, halfword) == 0U)
	{
		FLASH_Lock();
		return 0U;
	}
	FLASH_Lock();
	s_iap_flash.pending_valid = 0U;
	return 1U;
}

static UINT8 iap_flash_program_stream(UINT32 offset, const UINT8 *data, UINT16 length)
{
	UINT16 even_len;
	UINT16 halfword;

	if ((data == 0) || (length == 0U))
	{
		return 0U;
	}

	if (s_iap_flash.pending_valid != 0U)
	{
		if (offset == (s_iap_flash.pending_offset + 1U))
		{
			if (iap_flash_ensure_page_erased(s_iap_flash.pending_offset) == 0U)
			{
				return 0U;
			}

			halfword = (UINT16)s_iap_flash.pending_byte | ((UINT16)data[0] << 8);
			FLASH_Unlock();
			if (iap_flash_program_halfword(CAN_IAP_APP_BASE_ADDR + s_iap_flash.pending_offset, halfword) == 0U)
			{
				FLASH_Lock();
				return 0U;
			}
			FLASH_Lock();

			s_iap_flash.pending_valid = 0U;
			offset++;
			data++;
			length--;
		}
		else if (iap_flash_flush_pending() == 0U)
		{
			return 0U;
		}
	}

	if (length == 0U)
	{
		return 1U;
	}
	if ((offset & 1U) != 0U)
	{
		return 0U;
	}

	even_len = (UINT16)(length & (UINT16)~1U);
	if (even_len > 0U)
	{
		if (iap_flash_program_even_range(offset, data, even_len) == 0U)
		{
			return 0U;
		}
		offset += (UINT32)even_len;
		data += even_len;
		length = (UINT16)(length - even_len);
	}

	if (length != 0U)
	{
		s_iap_flash.pending_valid = 1U;
		s_iap_flash.pending_byte = data[0];
		s_iap_flash.pending_offset = offset;
	}

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

	if ((image_size == 0U) || (image_size > IAP_FLASH_APP_BYTES))
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

	memset(&s_iap_flash, 0, sizeof(s_iap_flash));
	s_iap_flash.owner = owner;
	memset(s_iap_flash.first_page, 0xFF, sizeof(s_iap_flash.first_page));

	FLASH_Unlock();
	if (iap_flash_erase_page(CAN_IAP_APP_BASE_ADDR) == 0U)
	{
		FLASH_Lock();
		memset(&s_iap_flash, 0, sizeof(s_iap_flash));
		return 0U;
	}
	FLASH_Lock();
	iap_flash_mark_page_erased(0U);
	return 1U;
}

UINT8 IapFlash_Write(UINT8 owner, UINT32 offset, const UINT8 *data, UINT16 length)
{
	UINT16 first_len;
	UINT32 first_end;
	UINT32 write_offset;
	const UINT8 *write_data;
	UINT16 write_length;

	if ((s_iap_flash.owner != owner) || (data == 0) || (iap_flash_range_valid(offset, length) == 0U))
	{
		return 0U;
	}

	write_offset = offset;
	write_data = data;
	write_length = length;

	if (write_offset < IAP_FLASH_PAGE_SIZE)
	{
		first_len = (UINT16)(IAP_FLASH_PAGE_SIZE - write_offset);
		if (first_len > write_length)
		{
			first_len = write_length;
		}
		memcpy(&s_iap_flash.first_page[write_offset], write_data, first_len);
		first_end = write_offset + (UINT32)first_len;
		if (first_end > s_iap_flash.first_page_len)
		{
			s_iap_flash.first_page_len = (UINT16)first_end;
		}

		write_offset += (UINT32)first_len;
		write_data += first_len;
		write_length = (UINT16)(write_length - first_len);
	}

	if ((write_length > 0U) && (iap_flash_program_stream(write_offset, write_data, write_length) == 0U))
	{
		return 0U;
	}

	return 1U;
}

UINT8 IapFlash_Finish(UINT8 owner, UINT32 image_size)
{
	if ((s_iap_flash.owner != owner) || (image_size == 0U) || (image_size > IAP_FLASH_APP_BYTES))
	{
		return 0U;
	}

	if (iap_flash_flush_pending() == 0U)
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

	memset(&s_iap_flash, 0, sizeof(s_iap_flash));
	return 1U;
}

void IapFlash_Abort(UINT8 owner)
{
	if (s_iap_flash.owner == owner)
	{
		memset(&s_iap_flash, 0, sizeof(s_iap_flash));
	}
}
