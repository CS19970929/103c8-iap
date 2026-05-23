#ifndef IAP_FLASH_H
#define IAP_FLASH_H

#include "stm32f10x.h"

#define IAP_FLASH_OWNER_SERIAL           ((UINT8)1U)
#define IAP_FLASH_OWNER_CAN              ((UINT8)2U)
#define IAP_FLASH_PAGE_SIZE              ((UINT32)1024U)

UINT8 IapFlash_Begin(UINT8 owner);
UINT8 IapFlash_Write(UINT8 owner, UINT32 offset, const UINT8 *data, UINT16 length);
UINT8 IapFlash_Finish(UINT8 owner, UINT32 image_size);
void IapFlash_Abort(UINT8 owner);

#endif
