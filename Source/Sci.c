#include "main.h"
#include "iap_upgrade.h"

// 长度的问题，怎么解决APP和UPPER长度的问题，还有REG数量的问题
//  SCI_485 Message Structure
//  SCI_485 Message Structure
struct RS485MSG
{
	UINT16 ptr_no;			  // Word stating what state msg is in
	UINT8 csr;				  // I2C address of slave msg is intended for
	UINT16 u16RdRegStartAddr; // read reg start addr
	UINT16 u16RdRegStartAddrActure;
	UINT8 u16RdRegByteNum;					// read byte lenth
	UINT8 AckLenth;							// ack byte lenth
	UINT8 AckType;							// ack type
	UINT8 ErrorType;						// error type
	UINT8 u16Buffer[RS485_MAX_BUFFER_SIZE]; // Array holding msg data - max that
	enum RS485_CMD_E enRs485CmdType;
};

uint16_t g_u16CommuErrCnt_SCI1 = 0; // SCI通信异常计数
uint8_t g_u8TxEnable_SCI1 = 0;
uint8_t g_u8TxFinishFlag_SCI1 = 0;
struct RS485MSG g_stCurrentMsgPtr_SCI1;

uint16_t g_u16CommuErrCnt_SCI2 = 0; // SCI通信异常计数
uint8_t g_u8TxEnable_SCI2 = 0;
uint8_t g_u8TxFinishFlag_SCI2 = 0;
struct RS485MSG g_stCurrentMsgPtr_SCI2;

uint16_t g_u16CommuErrCnt_SCI3 = 0; // SCI通信异常计数
uint8_t g_u8TxEnable_SCI3 = 0;
uint8_t g_u8TxFinishFlag_SCI3 = 0;
struct RS485MSG g_stCurrentMsgPtr_SCI3;

volatile union SysStatusFlag g_st_SysStatusFlag;
uint8_t g_u8SCITxBuff[SCI_TX_BUF_LEN];
uint8_t u8FlashReceiveCnt = 0;
uint16_t falshcnt = 0;
UINT8 Flash_Faultcnt = 0;

FLASH_Status FlashWriteOneHalfWord(uint32_t StartAddr, uint16_t Buffer)
{
	FLASH_Status result;

	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	result = FLASH_ErasePage(StartAddr);
	if (result == FLASH_COMPLETE)
	{
		result = FLASH_ProgramHalfWord(StartAddr, Buffer);
		if ((result == FLASH_COMPLETE) && ((*(vu16 *)StartAddr) != Buffer))
		{
			result = FLASH_ERROR_PG;
		}
	}
	FLASH_Lock();
	return result;
}

/*
	函数功能:	读取指定地址的半字(16位数据)
	输入参数:	faddr:读地址(此地址必须为2的倍数!!)
	返回值: 	对应数据
*/
UINT16 FlashReadOneHalfWord(UINT32 faddr)
{
	return *(vu16 *)faddr;
}

// 设置FLASH_AR寄存器为要擦除页所在地址，选择要擦除的页。FLASH_AR的值在哪一页范围内，就表示要擦除哪一页。
void APPFlashErase(uint16_t Sector)
{
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	while (FLASH_ErasePage(FLASH_ADDR_APP_START + Sector * 1024) != FLASH_COMPLETE)
		;
	FLASH_Lock();
}

UINT8 FlashWrite(uint32_t StartAddr, uint8_t *Buffer, uint16_t Length)
{
	uint16_t TempBuffer = 0;
	uint16_t i;
	UINT8 result = 0;
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	for (i = 0; i < Length; i += 2)
	{
		TempBuffer = Buffer[i] | 0xFF00;
		if (i + 1 < Length)
		{
			TempBuffer &= ((Buffer[i + 1] << 8) | 0x00FF);
		}
		if (FLASH_COMPLETE != FLASH_ProgramHalfWord(StartAddr + i, TempBuffer))
		{
			result = 1;
		}
	}
	FLASH_Lock();
	return result;
}

uint16_t Sci_CRC16RTU(uint8_t *pszBuf, uint16_t unLength)
{
	uint16_t CRCC = 0XFFFF;
	uint32_t CRC_count;

	for (CRC_count = 0; CRC_count < unLength; CRC_count++)
	{
		int i;

		CRCC = CRCC ^ *(pszBuf + CRC_count);

		for (i = 0; i < 8; i++)
		{
			if (CRCC & 1)
			{
				CRCC >>= 1;
				CRCC ^= 0xA001;
			}
			else
			{
				CRCC >>= 1;
			}
		}
	}

	return CRCC;
}

void CRC_verify(struct RS485MSG *s)
{
	uint16_t u16SciVerify;
	uint16_t t_u16FrameLenth;
	uint16_t u16SciVerify2;
	t_u16FrameLenth = s->ptr_no - 2;
	u16SciVerify = s->u16Buffer[t_u16FrameLenth] + (s->u16Buffer[t_u16FrameLenth + 1] << 8);
	u16SciVerify2 = Sci_CRC16RTU((uint8_t *)s->u16Buffer, t_u16FrameLenth);
	if (u16SciVerify == u16SciVerify2)
	{
		s->AckType = RS485_ACK_POS;
	}
	else
	{
		s->u16RdRegByteNum = 0;
		s->AckType = RS485_ACK_NEG;
		s->ErrorType = RS485_ERROR_CRC_ERROR;
	}
}

void Sci_DataInit(struct RS485MSG *s)
{
	uint16_t i;

	s->ptr_no = 0;
	s->csr = RS485_STA_IDLE;
	s->enRs485CmdType = RS485_CMD_READ_REGS;
	for (i = 0; i < RS485_MAX_BUFFER_SIZE; i++)
	{
		s->u16Buffer[i] = 0;
	}
}

void Sci_ReadRegs_Data(struct RS485MSG *s, UINT8 t_u8BuffTemp[])
{
	UINT16 u16SciTemp;
	UINT16 i, j;

	i = 0;
	for (j = 0; j < 34; j++)
	{
		u16SciTemp = 0;
		t_u8BuffTemp[i++] = (u16SciTemp >> 8) & 0x00FF;
		t_u8BuffTemp[i++] = u16SciTemp & 0x00FF;
	}
	u16SciTemp = falshcnt;
	t_u8BuffTemp[0] = (u16SciTemp >> 8) & 0x00FF;
	t_u8BuffTemp[1] = u16SciTemp & 0x00FF;

	u16SciTemp = 0;
	t_u8BuffTemp[i++] = (u16SciTemp >> 8) & 0x00FF;
	t_u8BuffTemp[i++] = u16SciTemp & 0x00FF;

	u16SciTemp = 0;
	t_u8BuffTemp[i++] = (u16SciTemp >> 8) & 0x00FF;
	t_u8BuffTemp[i++] = u16SciTemp & 0x00FF;

	u16SciTemp = 0;
	t_u8BuffTemp[i++] = (u16SciTemp >> 8) & 0x00FF;
	t_u8BuffTemp[i++] = u16SciTemp & 0x00FF;

	// sEE_I2CFaultcnt++;I2CFaultcnt= I2CFaultcnt+2;
	u16SciTemp = (0) | (Flash_Faultcnt << 8); // EEPROM和AFE的通讯错误//顺序就是这个
	t_u8BuffTemp[i++] = (u16SciTemp >> 8) & 0x00FF;
	t_u8BuffTemp[i++] = u16SciTemp & 0x00FF;
}

void Sci_ReadRegs_ACK(struct RS485MSG *s)
{
	UINT8 i;
	UINT16 u16SciTemp;
	if (s->AckType == RS485_ACK_POS)
	{
		if (s->u16RdRegStartAddrActure >= RS485_ADDR_RO_START)
		{
			Sci_ReadRegs_Data(s, g_u8SCITxBuff);
		}
		// 头码，前三个字节保持不变
		s->u16Buffer[0] = (s->u16Buffer[0] != 0) ? RS485_SLAVE_ADDR : s->u16Buffer[0];
		s->u16Buffer[1] = s->enRs485CmdType;
		s->u16Buffer[2] = s->u16RdRegByteNum;
		// 数据
		for (i = 0; i < (s->u16RdRegByteNum); i++)
		{
			s->u16Buffer[i + 3] = g_u8SCITxBuff[i + ((s->u16RdRegStartAddr) << 1)];
		}
		i = s->u16RdRegByteNum + 3;
	}
	else
	{
		i = 1;
		s->u16Buffer[i++] = s->enRs485CmdType | 0x80;
		s->u16Buffer[i++] = s->ErrorType;
	}
	u16SciTemp = Sci_CRC16RTU((UINT8 *)s->u16Buffer, i);
	s->u16Buffer[i++] = u16SciTemp & 0x00FF;
	s->u16Buffer[i++] = u16SciTemp >> 8;
	s->AckLenth = i;

	s->ptr_no = 0;
	s->csr = RS485_STA_TX_COMPLETE;
}

void Sci_ReadRegsDecode(struct RS485MSG *s)
{
	UINT16 t_u16Temp;

	t_u16Temp = s->u16Buffer[3] + (s->u16Buffer[2] << 8);
	s->u16RdRegStartAddrActure = t_u16Temp;

	if (t_u16Temp == (RS485_ADDR_RO_START + 0x0050))
	{ // 13个
		t_u16Temp -= (RS485_ADDR_RO_START + 80 - 22 - 10 - 2);
	}
	else
	{
		t_u16Temp = 0;
	}
	s->u16RdRegStartAddr = t_u16Temp;
	s->u16RdRegByteNum = (s->u16Buffer[5] + (s->u16Buffer[4] << 8)) << 1;
}

void Sci_WrRegsDecode(struct RS485MSG *s)
{
	uint16_t u16WrRegNum;
	uint16_t u16SciRegStartAddr;
	UINT8 ok;

	u16SciRegStartAddr = s->u16Buffer[3] + (s->u16Buffer[2] << 8);
	u16WrRegNum = s->u16Buffer[5] + (s->u16Buffer[4] << 8);
	ok = 0U;

	switch (u16SciRegStartAddr)
	{
	case RS485_CMD_ADDR_FLASH_CONNECT:
		ok = IapUpgrade_SerialConnect(u16WrRegNum, &s->u16Buffer[7]);
		break;

	case RS485_CMD_ADDR_FLASH_UPGRATE:
		ok = IapUpgrade_SerialWriteBlock(u16WrRegNum, s->u16Buffer[6], &s->u16Buffer[7]);
		break;

	case RS485_CMD_ADDR_FLASH_COMPLETE:
		ok = IapUpgrade_SerialComplete(u16WrRegNum, &s->u16Buffer[7]);
		if (ok != 0U)
		{
			u8FlagUdFinishE2PROM = 1U;
		}
		break;

	default:
		s->AckType = RS485_ACK_NEG;
		s->ErrorType = RS485_ERROR_CMD_INVALID;
		return;
	}

	if (ok == 0U)
	{
		s->AckType = RS485_ACK_NEG;
		s->ErrorType = RS485_ERROR_CMD_INVALID;
	}
}
void Sci_WrReg_s_Decode_ACK(struct RS485MSG *s)
{
	uint8_t i;
	uint16_t u16SciTemp;

	if (s->AckType == RS485_ACK_POS)
	{
		i = 6;
	}
	else
	{
		i = 1;
		s->u16Buffer[i++] = s->enRs485CmdType | 0x80;
		s->u16Buffer[i++] = s->ErrorType;
	}

	u16SciTemp = Sci_CRC16RTU((uint8_t *)s->u16Buffer, (uint16_t)i);
	s->u16Buffer[i++] = u16SciTemp & 0x00FF;
	s->u16Buffer[i++] = u16SciTemp >> 8;
	s->AckLenth = i;

	s->ptr_no = 0;
	s->csr = RS485_STA_TX_COMPLETE;
}

// 串口初始化函数
void InitUSART1(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

	// Enable the USART1 Interrupt(使能USART1中断)
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// USART1_TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出，按道理是开漏输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// USART1_RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入，做RX
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// 串口初始化
	USART_InitStructure.USART_BaudRate = 19200;										// 设置串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 设置数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 设置停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 设置效验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 设置流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 设置工作模式
	USART_Init(USART1, &USART_InitStructure);

	USART1->CR3 |= 1 << 0; // EIE，开帧错误中断，同时开启噪声中断

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 使能接收中断
	USART_Cmd(USART1, ENABLE);					   // 使能串口1

	Sci_DataInit(&g_stCurrentMsgPtr_SCI1);
}

void Sci1_FaultChk(void)
{
	UINT8 FaultCnt = 0;
	if (USART1->SR & 0x08)
	{ // 接收溢出错误，RXNEIE或EIE使能产生中断，开
		USART1->SR &= ~0x08;
		FaultCnt++;
	}

	if (USART1->SR & 0x04)
	{ // 检测到噪声，默认开，不开的话CR3的ONEBIT置1，不开
	  // USART_CR3的EIE使能中断
		USART1->SR &= ~0x04;
		FaultCnt++;
	}

	if (USART1->SR & 0x02)
	{						 // 帧错误，USART_CR3的EIE使能中断，开
		USART1->SR &= ~0x02; // 清除
		FaultCnt++;
	}

	if (USART1->SR & 0x01)
	{						 // 奇偶校验错误标志 USART_CR1的PEIE使能该中断，不开
		USART1->SR &= ~0x01; // 清除
		FaultCnt++;
	}

	if (FaultCnt)
	{
		g_u16CommuErrCnt_SCI1++;
	}
}

/*=================================================================
 * FUNCTION: Sci2_Tx_Deal
 * PURPOSE : 串口数据发送
 * INPUT:    void
 *
 * RETURN:   void
 *
 * CALLS:    void
 *
 * CALLED BY:SCI_TX_isr()
 *
 *=================================================================*/
void Sci1_Tx_Deal(struct RS485MSG *s)
{
	if (0 == g_u8TxEnable_SCI1)
	{
		return;
	}

	if (g_u16CommuErrCnt_SCI1)
	{ // 出现错误也得把数据全部接收完，然后不回复
		s->ptr_no = 0;
		s->csr = RS485_STA_TX_COMPLETE;
		g_u8TxFinishFlag_SCI1 = 1;
		g_u8TxEnable_SCI1 = 0;
		g_u16CommuErrCnt_SCI1 = 0;
		return;
	}

	while (!((USART1->SR) & (1 << 7)))
		; // Waitting Transmit data register empty
	if (s->ptr_no < s->AckLenth)
	{
		USART1->DR = s->u16Buffer[s->ptr_no]; // load data
		s->ptr_no++;
	}
	else
	{
		s->ptr_no = 0;
		s->csr = RS485_STA_TX_COMPLETE;
		g_u8TxFinishFlag_SCI1 = 1;
		g_u8TxEnable_SCI1 = 0;
		if (u8FlagUdFinishE2PROM)
		{
			u8FlagUdFinishE2PROM = 0;
			u8FlagUdFinish = 1;
			u8FlashReceiveCnt = 0;
		}
	}
}

void Sci1_Rx_Deal(struct RS485MSG *s)
{
	USART1->CR1 &= ~(1 << 5);
	s->u16Buffer[s->ptr_no] = USART1->DR; // 从RXFIFO 中读取接收到的数据
	if ((s->ptr_no == 0) && (s->u16Buffer[0] != RS485_SLAVE_ADDR) && (s->u16Buffer[0] != RS485_BROADCAST_ADDR))
	{
		s->ptr_no = 0;
		s->u16Buffer[0] = 0;
	}
	else
	{
		if (s->ptr_no == 1)
		{
			switch (s->u16Buffer[s->ptr_no])
			{
			case RS485_CMD_READ_REGS:
				s->enRs485CmdType = RS485_CMD_READ_REGS;
				s->ptr_no++;
				break;
			case RS485_CMD_WRITE_REG:
				s->enRs485CmdType = RS485_CMD_WRITE_REG;
				s->ptr_no++;
				break;
			case RS485_CMD_WRITE_REGS:
				s->enRs485CmdType = RS485_CMD_WRITE_REGS;
				s->ptr_no++;
				break;
			default:
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
				s->u16Buffer[1] = 0;
				break;
			}
		}
		else if (s->ptr_no > 2)
		{
			switch (s->enRs485CmdType)
			{
			case RS485_CMD_READ_REGS:
			case RS485_CMD_WRITE_REG:
				if (s->ptr_no == 7)
				{ //	receive complete
					s->csr = RS485_STA_RX_COMPLETE;
					// RCSTA1bits.CREN = 0;  //禁止接收
					// RC1IE = 0;			// 禁止EUSART2 接收中断
					USART1->CR1 &= ~(1 << 2);
					USART1->CR1 &= ~(1 << 5);
				}
				break;

			case RS485_CMD_WRITE_REGS:
				if (s->ptr_no >= 7)
				{
					if ((s->u16Buffer[6] != 0 && s->ptr_no == (s->u16Buffer[6] + 8)) || (s->u16Buffer[6] == 0 && s->ptr_no == s->u16Buffer[5] + (s->u16Buffer[4] << 8) + 8))
					{
						s->csr = RS485_STA_RX_COMPLETE;
						USART1->CR1 &= ~(1 << 2);
						USART1->CR1 &= ~(1 << 5);
					}
				}
				break;

			default:
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
				break;
			}

			s->ptr_no++;

			if (s->ptr_no == RS485_MAX_BUFFER_SIZE)
			{
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
			}
		}
		else
		{
			s->ptr_no++;
		}
	}
	USART1->CR1 |= (1 << 5);
}

/*=================================================================
 * FUNCTION: Sci1_Updata
 * PURPOSE : 收发数据状态机切换
 * INPUT:    void
 *
 * RETURN:   void
 *
 * CALLS:    void
 *
 * CALLED BY:main()
 *
 *=================================================================*/
void Sci1_Updata(struct RS485MSG *s)
{
	switch (s->csr)
	{
	// IDLE-空闲态，保持50ms后使能接收（物理层）receive set
	case RS485_STA_IDLE:
	{
		break;
	}
	// receive complete, to deal the receive data
	case RS485_STA_RX_COMPLETE:
	{
		// RC1IE = 0;// 禁止EUSART1 接收中断
		USART1->CR1 &= ~(1 << 5);
		CRC_verify(s);
		if (s->AckType == RS485_ACK_POS)
		{
			switch (s->enRs485CmdType)
			{
			case RS485_CMD_READ_REGS:
				Sci_ReadRegsDecode(s);
				break;
			case RS485_CMD_WRITE_REG:
				// Sci_WrRegDecode(s);
				break;
			case RS485_CMD_WRITE_REGS:
				Sci_WrRegsDecode(s);
				break;
			default:
				s->u16RdRegByteNum = 0;
				s->AckType = RS485_ACK_NEG;
				s->ErrorType = RS485_ERROR_NULL;
				break;
			}
		}
		s->csr = RS485_STA_RX_OK;
		break; // 下一轮再来
	}
	case RS485_STA_RX_OK:
	{
		switch (s->enRs485CmdType)
		{
		case RS485_CMD_READ_REGS:
			Sci_ReadRegs_ACK(s);
			USART1->CR1 |= (1 << 3);
			g_u8TxEnable_SCI1 = 1;
			g_u16CommuErrCnt_SCI1 = 0;
			break;
		case RS485_CMD_WRITE_REG:
		case RS485_CMD_WRITE_REGS:
			Sci_WrReg_s_Decode_ACK(s);
			// TXSTA1bits.TXEN = 1;    //使能发送
			USART1->CR1 |= (1 << 3);
			g_u8TxEnable_SCI1 = 1;
			break;
		default:
			break;
		}
	}
	// transmit complete, to switch receive wait 20ms
	case RS485_STA_TX_COMPLETE:
	{
		if (g_u8TxFinishFlag_SCI1)
		{
			s->csr = RS485_STA_IDLE;
			s->u16Buffer[0] = 0;
			s->u16Buffer[1] = 0;
			s->u16Buffer[2] = 0;
			s->u16Buffer[3] = 0;
			g_u8TxFinishFlag_SCI1 = 0;
			s->ptr_no = 0;

			// RCSTA1bits.CREN = 1;    //使能接收
			// RC1IE = 1;// 使能EUSART2 接收中断
			USART1->CR1 |= (1 << 2);
			USART1->CR1 |= (1 << 5);
			// TXSTA1bits.TXEN = 0;    //禁止发送，不能禁止，否则通信很容易断
			g_u8TxEnable_SCI1 = 0;
		}
		break;
	}

	default:
	{
		s->csr = RS485_STA_IDLE;
		break;
	}
	}
	Sci1_Tx_Deal(s);
	// Sci1_FaultChk();
}

// 串口初始化函数
void InitUSART2(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// Enable the USART1 Interrupt(使能USART1中断)
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// USART2_TX   PA2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出，按道理是开漏输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// USART2_RX	  PA3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入，做RX
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 串口初始化
	USART_InitStructure.USART_BaudRate = 19200;										// 设置串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 设置数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 设置停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 设置效验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 设置流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 设置工作模式
	USART_Init(USART2, &USART_InitStructure);

	USART2->CR3 |= 1 << 0; // EIE，开帧错误中断，同时开启噪声中断

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // 使能接收中断
	USART_Cmd(USART2, ENABLE);					   // 使能串口2

	Sci_DataInit(&g_stCurrentMsgPtr_SCI2);
}

void Sci2_FaultChk(void)
{
	UINT8 FaultCnt = 0;
	if (USART2->SR & 0x08)
	{ // 接收溢出错误，RXNEIE或EIE使能产生中断，开
		USART2->SR &= ~0x08;
		FaultCnt++;
	}

	if (USART2->SR & 0x04)
	{ // 检测到噪声，默认开，不开的话CR3的ONEBIT置1，不开
	  // USART_CR3的EIE使能中断
		USART2->SR &= ~0x04;
		FaultCnt++;
	}

	if (USART2->SR & 0x02)
	{						 // 帧错误，USART_CR3的EIE使能中断，开
		USART2->SR &= ~0x02; // 清除
		FaultCnt++;
	}

	if (USART2->SR & 0x01)
	{						 // 奇偶校验错误标志 USART_CR1的PEIE使能该中断，不开
		USART2->SR &= ~0x01; // 清除
		FaultCnt++;
	}

	if (FaultCnt)
	{
		g_u16CommuErrCnt_SCI2++;
	}
}

/*=================================================================
 * FUNCTION: Sci2_Tx_Deal
 * PURPOSE : 串口数据发送
 * INPUT:    void
 *
 * RETURN:   void
 *
 * CALLS:    void
 *
 * CALLED BY:SCI_TX_isr()
 *
 *=================================================================*/
void Sci2_Tx_Deal(struct RS485MSG *s)
{
	if (0 == g_u8TxEnable_SCI2)
	{
		return;
	}

	if (g_u16CommuErrCnt_SCI2)
	{ // 出现错误也得把数据全部接收完，然后不回复
		s->ptr_no = 0;
		s->csr = RS485_STA_TX_COMPLETE;
		g_u8TxFinishFlag_SCI2 = 1;
		g_u8TxEnable_SCI2 = 0;
		g_u16CommuErrCnt_SCI2 = 0;
		return;
	}

	while (!((USART2->SR) & (1 << 7)))
		; // Waitting Transmit data register empty
	if (s->ptr_no < s->AckLenth)
	{
		USART2->DR = s->u16Buffer[s->ptr_no]; // load data
		s->ptr_no++;
	}
	else
	{
		s->ptr_no = 0;
		s->csr = RS485_STA_TX_COMPLETE;
		g_u8TxFinishFlag_SCI2 = 1;
		g_u8TxEnable_SCI2 = 0;
		if (u8FlagUdFinishE2PROM)
		{
			u8FlagUdFinishE2PROM = 0;
			u8FlagUdFinish = 1;
			u8FlashReceiveCnt = 0;
		}
	}
}

void Sci2_Rx_Deal(struct RS485MSG *s)
{
	USART2->CR1 &= ~(1 << 5);
	s->u16Buffer[s->ptr_no] = USART2->DR; // 从RXFIFO 中读取接收到的数据
	if ((s->ptr_no == 0) && (s->u16Buffer[0] != RS485_SLAVE_ADDR) && (s->u16Buffer[0] != RS485_BROADCAST_ADDR))
	{
		s->ptr_no = 0;
		s->u16Buffer[0] = 0;
	}
	else
	{
		if (s->ptr_no == 1)
		{
			switch (s->u16Buffer[s->ptr_no])
			{
			case RS485_CMD_READ_REGS:
				s->enRs485CmdType = RS485_CMD_READ_REGS;
				s->ptr_no++;
				break;
			case RS485_CMD_WRITE_REG:
				s->enRs485CmdType = RS485_CMD_WRITE_REG;
				s->ptr_no++;
				break;
			case RS485_CMD_WRITE_REGS:
				s->enRs485CmdType = RS485_CMD_WRITE_REGS;
				s->ptr_no++;
				break;
			default:
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
				s->u16Buffer[1] = 0;
				break;
			}
		}
		else if (s->ptr_no > 2)
		{
			switch (s->enRs485CmdType)
			{
			case RS485_CMD_READ_REGS:
			case RS485_CMD_WRITE_REG:
				if (s->ptr_no == 7)
				{ //	receive complete
					s->csr = RS485_STA_RX_COMPLETE;
					// RCSTA1bits.CREN = 0;  //禁止接收
					// RC1IE = 0;			// 禁止EUSART2 接收中断
					USART2->CR1 &= ~(1 << 2);
					USART2->CR1 &= ~(1 << 5);
				}
				break;

			case RS485_CMD_WRITE_REGS:
				if (s->ptr_no >= 7)
				{
					if ((s->u16Buffer[6] != 0 && s->ptr_no == (s->u16Buffer[6] + 8)) || (s->u16Buffer[6] == 0 && s->ptr_no == s->u16Buffer[5] + (s->u16Buffer[4] << 8) + 8))
					{
						s->csr = RS485_STA_RX_COMPLETE;
						USART2->CR1 &= ~(1 << 2);
						USART2->CR1 &= ~(1 << 5);
					}
				}
				break;

			default:
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
				break;
			}

			s->ptr_no++;

			if (s->ptr_no == RS485_MAX_BUFFER_SIZE)
			{
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
			}
		}
		else
		{
			s->ptr_no++;
		}
	}
	USART2->CR1 |= (1 << 5);
}

/*=================================================================
 * FUNCTION: Sci1_Updata
 * PURPOSE : 收发数据状态机切换
 * INPUT:    void
 *
 * RETURN:   void
 *
 * CALLS:    void
 *
 * CALLED BY:main()
 *
 *=================================================================*/
void Sci2_Updata(struct RS485MSG *s)
{
	switch (s->csr)
	{
	// IDLE-空闲态，保持50ms后使能接收（物理层）receive set
	case RS485_STA_IDLE:
	{
		break;
	}
	// receive complete, to deal the receive data
	case RS485_STA_RX_COMPLETE:
	{
		// RC1IE = 0;// 禁止EUSART1 接收中断
		USART2->CR1 &= ~(1 << 5);
		CRC_verify(s);
		if (s->AckType == RS485_ACK_POS)
		{
			switch (s->enRs485CmdType)
			{
			case RS485_CMD_READ_REGS:
				Sci_ReadRegsDecode(s);
				break;
			case RS485_CMD_WRITE_REG:
				// Sci_WrRegDecode(s);
				break;
			case RS485_CMD_WRITE_REGS:
				Sci_WrRegsDecode(s);
				break;
			default:
				s->u16RdRegByteNum = 0;
				s->AckType = RS485_ACK_NEG;
				s->ErrorType = RS485_ERROR_NULL;
				break;
			}
		}
		s->csr = RS485_STA_RX_OK;
		break; // 下一轮再来
	}
	case RS485_STA_RX_OK:
	{
		switch (s->enRs485CmdType)
		{
		case RS485_CMD_READ_REGS:
			Sci_ReadRegs_ACK(s);
			USART2->CR1 |= (1 << 3);
			g_u8TxEnable_SCI2 = 1;
			g_u16CommuErrCnt_SCI2 = 0;
			break;
		case RS485_CMD_WRITE_REG:
		case RS485_CMD_WRITE_REGS:
			Sci_WrReg_s_Decode_ACK(s);
			// TXSTA1bits.TXEN = 1;    //使能发送
			USART2->CR1 |= (1 << 3);
			g_u8TxEnable_SCI2 = 1;
			break;
		default:
			break;
		}
	}
	// transmit complete, to switch receive wait 20ms
	case RS485_STA_TX_COMPLETE:
	{
		if (g_u8TxFinishFlag_SCI2)
		{
			s->csr = RS485_STA_IDLE;
			s->u16Buffer[0] = 0;
			s->u16Buffer[1] = 0;
			s->u16Buffer[2] = 0;
			s->u16Buffer[3] = 0;
			g_u8TxFinishFlag_SCI2 = 0;
			s->ptr_no = 0;

			// RCSTA1bits.CREN = 1;    //使能接收
			// RC1IE = 1;// 使能EUSART2 接收中断
			USART2->CR1 |= (1 << 2);
			USART2->CR1 |= (1 << 5);
			// TXSTA1bits.TXEN = 0;    //禁止发送，不能禁止，否则通信很容易断
			g_u8TxEnable_SCI2 = 0;
		}
		break;
	}

	default:
	{
		s->csr = RS485_STA_IDLE;
		break;
	}
	}
	Sci2_Tx_Deal(s);
	// Sci1_FaultChk();
}

// 串口初始化函数
void InitUSART3(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);
	// Enable the USART3 Interrupt(使能USART3中断)
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// USART3_TX   PD8
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出，按道理是开漏输出
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// USART3_RX	  PD9初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入，做RX
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// 串口初始化
	USART_InitStructure.USART_BaudRate = 19200;										// 设置串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 设置数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 设置停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 设置效验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 设置流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 设置工作模式
	USART_Init(USART3, &USART_InitStructure);

	USART3->CR3 |= 1 << 0; // EIE，开帧错误中断，同时开启噪声中断

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // 使能接收中断
	USART_Cmd(USART3, ENABLE);					   // 使能串口3

	Sci_DataInit(&g_stCurrentMsgPtr_SCI3);
}

void Sci3_FaultChk(void)
{
	UINT8 FaultCnt = 0;
	if (USART3->SR & 0x08)
	{ // 接收溢出错误，RXNEIE或EIE使能产生中断，开
		USART3->SR &= ~0x08;
		FaultCnt++;
	}

	if (USART3->SR & 0x04)
	{ // 检测到噪声，默认开，不开的话CR3的ONEBIT置1，不开
	  // USART_CR3的EIE使能中断
		USART3->SR &= ~0x04;
		FaultCnt++;
	}

	if (USART3->SR & 0x02)
	{						 // 帧错误，USART_CR3的EIE使能中断，开
		USART3->SR &= ~0x02; // 清除
		FaultCnt++;
	}

	if (USART3->SR & 0x01)
	{						 // 奇偶校验错误标志 USART_CR1的PEIE使能该中断，不开
		USART3->SR &= ~0x01; // 清除
		FaultCnt++;
	}

	if (FaultCnt)
	{
		g_u16CommuErrCnt_SCI3++;
	}
}

/*=================================================================
 * FUNCTION: Sci2_Tx_Deal
 * PURPOSE : 串口数据发送
 * INPUT:    void
 *
 * RETURN:   void
 *
 * CALLS:    void
 *
 * CALLED BY:SCI_TX_isr()
 *
 *=================================================================*/
void Sci3_Tx_Deal(struct RS485MSG *s)
{
	if (0 == g_u8TxEnable_SCI3)
	{
		return;
	}

	if (g_u16CommuErrCnt_SCI3)
	{ // 出现错误也得把数据全部接收完，然后不回复
		s->ptr_no = 0;
		s->csr = RS485_STA_TX_COMPLETE;
		g_u8TxFinishFlag_SCI3 = 1;
		g_u8TxEnable_SCI3 = 0;
		g_u16CommuErrCnt_SCI3 = 0;
		return;
	}

	while (!((USART3->SR) & (1 << 7)))
		; // Waitting Transmit data register empty
	if (s->ptr_no < s->AckLenth)
	{
		USART3->DR = s->u16Buffer[s->ptr_no]; // load data
		s->ptr_no++;
	}
	else
	{
		s->ptr_no = 0;
		s->csr = RS485_STA_TX_COMPLETE;
		g_u8TxFinishFlag_SCI3 = 1;
		g_u8TxEnable_SCI3 = 0;
		if (u8FlagUdFinishE2PROM)
		{
			u8FlagUdFinishE2PROM = 0;
			u8FlagUdFinish = 1;
			u8FlashReceiveCnt = 0;
		}
	}
}

void Sci3_Rx_Deal(struct RS485MSG *s)
{
	USART3->CR1 &= ~(1 << 5);
	s->u16Buffer[s->ptr_no] = USART3->DR; // 从RXFIFO 中读取接收到的数据
	if ((s->ptr_no == 0) && (s->u16Buffer[0] != RS485_SLAVE_ADDR) && (s->u16Buffer[0] != RS485_BROADCAST_ADDR))
	{
		s->ptr_no = 0;
		s->u16Buffer[0] = 0;
	}
	else
	{
		if (s->ptr_no == 1)
		{
			switch (s->u16Buffer[s->ptr_no])
			{
			case RS485_CMD_READ_REGS:
				s->enRs485CmdType = RS485_CMD_READ_REGS;
				s->ptr_no++;
				break;
			case RS485_CMD_WRITE_REG:
				s->enRs485CmdType = RS485_CMD_WRITE_REG;
				s->ptr_no++;
				break;
			case RS485_CMD_WRITE_REGS:
				s->enRs485CmdType = RS485_CMD_WRITE_REGS;
				s->ptr_no++;
				break;
			default:
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
				s->u16Buffer[1] = 0;
				break;
			}
		}
		else if (s->ptr_no > 2)
		{
			switch (s->enRs485CmdType)
			{
			case RS485_CMD_READ_REGS:
			case RS485_CMD_WRITE_REG:
				if (s->ptr_no == 7)
				{ //	receive complete
					s->csr = RS485_STA_RX_COMPLETE;
					// RCSTA1bits.CREN = 0;  //禁止接收
					// RC1IE = 0;			// 禁止EUSART2 接收中断
					USART3->CR1 &= ~(1 << 2);
					USART3->CR1 &= ~(1 << 5);
				}
				break;

			case RS485_CMD_WRITE_REGS:
				if (s->ptr_no >= 7)
				{
					if ((s->u16Buffer[6] != 0 && s->ptr_no == (s->u16Buffer[6] + 8)) || (s->u16Buffer[6] == 0 && s->ptr_no == s->u16Buffer[5] + (s->u16Buffer[4] << 8) + 8))
					{
						s->csr = RS485_STA_RX_COMPLETE;
						USART3->CR1 &= ~(1 << 2);
						USART3->CR1 &= ~(1 << 5);
					}
				}
				break;

			default:
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
				break;
			}

			s->ptr_no++;

			if (s->ptr_no == RS485_MAX_BUFFER_SIZE)
			{
				s->ptr_no = 0;
				s->u16Buffer[0] = 0;
			}
		}
		else
		{
			s->ptr_no++;
		}
	}
	USART3->CR1 |= (1 << 5);
}

/*=================================================================
 * FUNCTION: Sci1_Updata
 * PURPOSE : 收发数据状态机切换
 * INPUT:    void
 *
 * RETURN:   void
 *
 * CALLS:    void
 *
 * CALLED BY:main()
 *
 *=================================================================*/
void Sci3_Updata(struct RS485MSG *s)
{
	switch (s->csr)
	{
	// IDLE-空闲态，保持50ms后使能接收（物理层）receive set
	case RS485_STA_IDLE:
	{
		break;
	}
	// receive complete, to deal the receive data
	case RS485_STA_RX_COMPLETE:
	{
		// RC1IE = 0;// 禁止EUSART1 接收中断
		USART3->CR1 &= ~(1 << 5);
		CRC_verify(s);
		if (s->AckType == RS485_ACK_POS)
		{
			switch (s->enRs485CmdType)
			{
			case RS485_CMD_READ_REGS:
				Sci_ReadRegsDecode(s);
				break;
			case RS485_CMD_WRITE_REG:
				// Sci_WrRegDecode(s);
				break;
			case RS485_CMD_WRITE_REGS:
				Sci_WrRegsDecode(s);
				break;
			default:
				s->u16RdRegByteNum = 0;
				s->AckType = RS485_ACK_NEG;
				s->ErrorType = RS485_ERROR_NULL;
				break;
			}
		}
		s->csr = RS485_STA_RX_OK;
		break; // 下一轮再来
	}
	case RS485_STA_RX_OK:
	{
		switch (s->enRs485CmdType)
		{
		case RS485_CMD_READ_REGS:
			Sci_ReadRegs_ACK(s);
			USART3->CR1 |= (1 << 3);
			g_u8TxEnable_SCI3 = 1;
			g_u16CommuErrCnt_SCI3 = 0;
			break;
		case RS485_CMD_WRITE_REG:
		case RS485_CMD_WRITE_REGS:
			Sci_WrReg_s_Decode_ACK(s);
			// TXSTA1bits.TXEN = 1;    //使能发送
			USART3->CR1 |= (1 << 3);
			g_u8TxEnable_SCI3 = 1;
			break;
		default:
			break;
		}
	}
	// transmit complete, to switch receive wait 20ms
	case RS485_STA_TX_COMPLETE:
	{
		if (g_u8TxFinishFlag_SCI3)
		{
			s->csr = RS485_STA_IDLE;
			s->u16Buffer[0] = 0;
			s->u16Buffer[1] = 0;
			s->u16Buffer[2] = 0;
			s->u16Buffer[3] = 0;
			g_u8TxFinishFlag_SCI3 = 0;
			s->ptr_no = 0;

			// RCSTA1bits.CREN = 1;    //使能接收
			// RC1IE = 1;// 使能EUSART2 接收中断
			USART3->CR1 |= (1 << 2);
			USART3->CR1 |= (1 << 5);
			// TXSTA1bits.TXEN = 0;    //禁止发送，不能禁止，否则通信很容易断
			g_u8TxEnable_SCI3 = 0;
		}
		break;
	}

	default:
	{
		s->csr = RS485_STA_IDLE;
		break;
	}
	}
	Sci3_Tx_Deal(s);
	// Sci1_FaultChk();
}

void USART1_IRQHandler(void)
{
	Sci1_FaultChk();
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		Sci1_Rx_Deal(&g_stCurrentMsgPtr_SCI1);
	}
}

void USART2_IRQHandler(void)
{
	Sci2_FaultChk();
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		Sci2_Rx_Deal(&g_stCurrentMsgPtr_SCI2);
	}
}

void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		Sci3_FaultChk();
		Sci3_Rx_Deal(&g_stCurrentMsgPtr_SCI3);
	}
}
