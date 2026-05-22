#ifndef CAN_IAP_PROTOCOL_H
#define CAN_IAP_PROTOCOL_H

#include "stm32f10x.h"

#define CAN_IAP_PROTOCOL_VERSION        ((UINT8)1U)
#define CAN_IAP_NODE_DEFAULT            ((UINT8)1U)

#define CAN_IAP_BASE_ADDR               ((UINT32)0x08000000U)
#define CAN_IAP_APP_BASE_ADDR           ((UINT32)0x08004800U)
#define CAN_IAP_APP_LIMIT_ADDR          ((UINT32)0x0801F800U)
#define CAN_IAP_SRAM_BASE_ADDR          ((UINT32)0x20000000U)
#define CAN_IAP_SRAM_LIMIT_ADDR         ((UINT32)0x20005000U)

#define CAN_IAP_CTRL_BASE_ID            ((UINT32)0x14F8F000U)
#define CAN_IAP_ACK_BASE_ID             ((UINT32)0x14F8F100U)
#define CAN_IAP_DATA_BASE_ID            ((UINT32)0x14000000U)
#define CAN_IAP_DATA_ID_MASK            ((UINT32)0x1F000000U)

#define CAN_IAP_CMD_HELLO               ((UINT8)0x01U)
#define CAN_IAP_CMD_START               ((UINT8)0x02U)
#define CAN_IAP_CMD_COMMIT              ((UINT8)0x03U)
#define CAN_IAP_CMD_END                 ((UINT8)0x04U)
#define CAN_IAP_CMD_ABORT               ((UINT8)0x05U)
#define CAN_IAP_CMD_ACK                 ((UINT8)0x79U)
#define CAN_IAP_CMD_NACK                ((UINT8)0x1FU)

#define CAN_IAP_ERR_OK                  ((UINT8)0x00U)
#define CAN_IAP_ERR_BAD_CMD             ((UINT8)0x01U)
#define CAN_IAP_ERR_BAD_STATE           ((UINT8)0x02U)
#define CAN_IAP_ERR_BAD_PARAM           ((UINT8)0x03U)
#define CAN_IAP_ERR_BAD_SEQ             ((UINT8)0x04U)
#define CAN_IAP_ERR_CRC                 ((UINT8)0x05U)
#define CAN_IAP_ERR_FLASH               ((UINT8)0x06U)
#define CAN_IAP_ERR_APP_INVALID         ((UINT8)0x07U)

UINT16 CanIap_Crc16Update(UINT16 crc, const UINT8 *data, UINT16 length);
UINT16 CanIap_Crc16Calc(const UINT8 *data, UINT16 length);
UINT32 CanIap_CtrlId(UINT8 node);
UINT32 CanIap_AckId(UINT8 node);
UINT32 CanIap_DataId(UINT8 node, UINT16 seq);
UINT8 CanIap_IsValidAppVector(UINT32 app_addr, UINT32 app_limit);
void CanIap_BuildAck(UINT8 cmd, UINT8 status, UINT16 expect_seq, UINT8 code, UINT8 data[8]);
void CanIap_BuildNack(UINT8 cmd, UINT16 expect_seq, UINT8 code, UINT8 data[8]);
UINT16 CanIap_ReadBe16(const UINT8 *data);
UINT32 CanIap_ReadBe32(const UINT8 *data);
void CanIap_WriteBe16(UINT8 *data, UINT16 value);

#endif
