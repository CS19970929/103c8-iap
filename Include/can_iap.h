#ifndef CAN_IAP_H
#define CAN_IAP_H

#include "stm32f10x.h"

#define CAN_IAP_STATE_IDLE              ((UINT8)0U)
#define CAN_IAP_STATE_RECEIVING         ((UINT8)1U)
#define CAN_IAP_STATE_DONE              ((UINT8)2U)
#define CAN_IAP_STATE_ERROR             ((UINT8)3U)

void CanIap_Init(void);
void CanIap_Task(void);
void CanIap_10msTask(void);
UINT8 CanIap_GetState(void);

#endif
