#ifndef IAP_UPGRADE_H
#define IAP_UPGRADE_H

#include "stm32f10x.h"

#define IAP_UPGRADE_STATE_IDLE          ((UINT8)0U)
#define IAP_UPGRADE_STATE_READY         ((UINT8)1U)
#define IAP_UPGRADE_STATE_RECEIVING     ((UINT8)2U)
#define IAP_UPGRADE_STATE_DONE          ((UINT8)3U)
#define IAP_UPGRADE_STATE_ERROR         ((UINT8)4U)

#define IAP_UPGRADE_ERR_OK              ((UINT8)0U)
#define IAP_UPGRADE_ERR_BAD_PARAM       ((UINT8)1U)
#define IAP_UPGRADE_ERR_BAD_STATE       ((UINT8)2U)
#define IAP_UPGRADE_ERR_RANGE           ((UINT8)3U)
#define IAP_UPGRADE_ERR_FLASH           ((UINT8)4U)
#define IAP_UPGRADE_ERR_VERIFY          ((UINT8)5U)
#define IAP_UPGRADE_ERR_APP_INVALID     ((UINT8)6U)
#define IAP_UPGRADE_ERR_TIMEOUT         ((UINT8)7U)

void IapUpgrade_Init(void);
UINT8 IapUpgrade_SerialConnect(UINT16 value_count, const UINT8 *payload);
UINT8 IapUpgrade_SerialWriteBlock(UINT16 declared_length, UINT8 byte_count, const UINT8 *payload);
UINT8 IapUpgrade_SerialComplete(UINT16 value_count, const UINT8 *payload);
void IapUpgrade_10msTask(void);
UINT8 IapUpgrade_GetState(void);
UINT8 IapUpgrade_GetLastError(void);
UINT16 IapUpgrade_GetBlockCount(void);
UINT32 IapUpgrade_GetWrittenBytes(void);

#endif
