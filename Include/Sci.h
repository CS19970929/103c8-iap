#ifndef SCI_H
#define SCI_H



void IAP_Init(void);
uint8_t IAPDownloadCheck(void);
int8_t DownloadFirmware(void);

#define	SCI_TX_BUF_LEN			130   //原来130和100
#define RS485_MAX_BUFFER_SIZE 	1200

#define	RS485_BROADCAST_ADDR		(( uint8_t ) 0x00 )
#define	RS485_SLAVE_ADDR			(( uint8_t ) 0x01 )

//RS485状态机状态
#define	RS485_STA_IDLE				0
#define	RS485_STA_RX_COMPLETE		1
#define	RS485_STA_RX_OK				2
#define	RS485_STA_TX_COMPLETE		3

#define RS485_ADDR_RO_START				0xD000


#define	RS485_ACK_POS			        0x00	// 正响应
#define	RS485_ACK_NEG			        0x01	// 负响应
//Error type
#define	RS485_ERROR_ADDR_INVALID	    0x01	// 地址不合法
#define	RS485_ERROR_CRC_ERROR			0x02	// CRC校验错误
#define	RS485_ERROR_DATA_INVALID	    0x03	// 参数不合法
#define	RS485_ERROR_CMD_INVALID			0x04	// 当前状态下命令无效
#define	RS485_ERROR_RONLY_NO_W			0x05	// 只读参数拒绝写入
#define	RS485_ERROR_WONLY_NO_R			0x06	// 只写参数拒绝读取
#define	RS485_ERROR_NO_PERMISSION		0x07	// 无权限
#define	RS485_ERROR_NULL			    0x08	// 未知错误

#define RS485_CMD_ADDR_FLASH_CONNECT		0xFFFD	//MCU连接检测
//#define RS485_CMD_ADDR_FLASH_SIZE			0xFFFE	//MCU HEX文件大小
#define RS485_CMD_ADDR_FLASH_UPGRATE		0xFFFE	//代码升级命令
#define RS485_CMD_ADDR_FLASH_COMPLETE		0xFFFF	//代码升级命令

//485 cmd type
enum RS485_CMD_E {
	RS485_CMD_READ_REGS = 3,
	RS485_CMD_WRITE_REG = 6,
	RS485_CMD_WRITE_REGS = 16
};
 
union SysStatusFlag{
    uint8_t   all;
    struct structSysStatusFlag
    {
        uint8_t b1SCI1CommuErr       	:1; //SCI1通讯异常
        uint8_t b1SCI2CommuErr       	:1; //SCI2通讯异常
        uint8_t b1CanTxDelayFlag      	:1; //CAN发送延时
        uint8_t b1BUSOFF              	:1; //BUS OFF状态标志位

    	uint8_t b1CANOffTestSd        	:1;
		uint8_t b1EepromErr           	:1; //EEPROM异常
        uint8_t b2Rcved             	:2; 
     }bits;
};


extern struct RS485MSG g_stCurrentMsgPtr_SCI1;
extern struct RS485MSG g_stCurrentMsgPtr_SCI2;
extern struct RS485MSG g_stCurrentMsgPtr_SCI3;

extern uint16_t falshcnt; 
extern UINT8 Flash_Faultcnt;
extern uint8_t u8FlashReceiveCnt;

void Sci_DataInit( struct RS485MSG *s);

void InitUSART1(void);
void Sci1_Updata(struct RS485MSG *s);

void InitUSART2(void);
void Sci2_Updata(struct RS485MSG *s);

void InitUSART3(void);
void Sci3_Updata(struct RS485MSG *s);

FLASH_Status FlashWriteOneHalfWord(uint32_t StartAddr,uint16_t Buffer);
UINT16 FlashReadOneHalfWord(UINT32 faddr);

#endif
