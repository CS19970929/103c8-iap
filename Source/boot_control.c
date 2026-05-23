#include "main.h"
#include "boot_control.h"
#include "can_iap_protocol.h"

static volatile BOOT_CTRL_MAILBOX *boot_ctrl_mailbox(void)
{
	return (volatile BOOT_CTRL_MAILBOX *)BOOT_CTRL_MAILBOX_ADDR;
}

static UINT32 boot_ctrl_crc(UINT32 magic, UINT32 request)
{
	return magic ^ request ^ 0xA5A55A5AU;
}

UINT8 BootCtrl_IsIapRequestPending(void)
{
	volatile BOOT_CTRL_MAILBOX *mailbox;

	mailbox = boot_ctrl_mailbox();
	if ((mailbox->magic != BOOT_CTRL_MAILBOX_MAGIC) ||
		(mailbox->magic_inv != (UINT32)~BOOT_CTRL_MAILBOX_MAGIC) ||
		(mailbox->request != BOOT_CTRL_MAILBOX_REQUEST) ||
		(mailbox->request_inv != (UINT32)~BOOT_CTRL_MAILBOX_REQUEST) ||
		(mailbox->crc != boot_ctrl_crc(BOOT_CTRL_MAILBOX_MAGIC, BOOT_CTRL_MAILBOX_REQUEST)))
	{
		return 0U;
	}

	return 1U;
}

void BootCtrl_ClearRequest(void)
{
	volatile BOOT_CTRL_MAILBOX *mailbox;

	mailbox = boot_ctrl_mailbox();
	mailbox->magic = 0U;
	mailbox->magic_inv = 0U;
	mailbox->request = 0U;
	mailbox->request_inv = 0U;
	mailbox->crc = 0U;
}

UINT8 BootCtrl_RequestIap(void)
{
	volatile BOOT_CTRL_MAILBOX *mailbox;

	mailbox = boot_ctrl_mailbox();
	mailbox->magic = BOOT_CTRL_MAILBOX_MAGIC;
	mailbox->magic_inv = (UINT32)~BOOT_CTRL_MAILBOX_MAGIC;
	mailbox->request = BOOT_CTRL_MAILBOX_REQUEST;
	mailbox->request_inv = (UINT32)~BOOT_CTRL_MAILBOX_REQUEST;
	mailbox->crc = boot_ctrl_crc(BOOT_CTRL_MAILBOX_MAGIC, BOOT_CTRL_MAILBOX_REQUEST);

	return BootCtrl_IsIapRequestPending();
}

UINT8 BootCtrl_ConsumeIapRequest(void)
{
	if (BootCtrl_IsIapRequestPending() == 0U)
	{
		return 0U;
	}

	BootCtrl_ClearRequest();
	return 1U;
}

UINT8 BootCtrl_ShouldJumpToApp(void)
{
	if (BootCtrl_ConsumeIapRequest() != 0U)
	{
		return 0U;
	}

	return CanIap_IsValidAppVector(FLASH_ADDR_APP_START, CAN_IAP_APP_LIMIT_ADDR);
}
