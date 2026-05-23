#ifndef BOOT_CONTROL_H
#define BOOT_CONTROL_H

#include "stm32f10x.h"

#define BOOT_CTRL_MAILBOX_ADDR           ((UINT32)0x20004FE0U)
#define BOOT_CTRL_MAILBOX_MAGIC          ((UINT32)0x49415031U)
#define BOOT_CTRL_MAILBOX_REQUEST        ((UINT32)0x5AA55AA5U)

typedef struct
{
	UINT32 magic;
	UINT32 magic_inv;
	UINT32 request;
	UINT32 request_inv;
	UINT32 crc;
} BOOT_CTRL_MAILBOX;

UINT8 BootCtrl_RequestIap(void);
UINT8 BootCtrl_IsIapRequestPending(void);
UINT8 BootCtrl_ConsumeIapRequest(void);
UINT8 BootCtrl_ShouldJumpToApp(void);
void BootCtrl_ClearRequest(void);

#endif
