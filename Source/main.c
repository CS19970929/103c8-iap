#include "main.h"
#include "can_iap.h"
#include "can_iap_protocol.h"
#include "iap_upgrade.h"
#include "boot_control.h"

void App_UpgrateFaultMonitor(void);
void App_FlashUpgrate(void);
void App_UpdateFinishChk(void);

void IAP_To_APP_Jump(void);
void InitDelay(void);
void InitIO(void);
void InitSystemWakeUp(void);
void App_SysTime(void);
void InitTimer(void);
void FlashTest(void);
void InitNVIC(void);

volatile union SYS_TIME g_st_SysTimeFlag;
UINT8 g_u81msCnt = 0;
UINT8 g_u810msClockCnt = 0;
UINT8 g_u81msClockCnt = 0;

typedef void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;
uint8_t u8FlagUdFinishE2PROM = 0;
uint8_t u8FlagUdFinish = 0;
static INT8 fac_us = 0;	 // us
static INT16 fac_ms = 0; // ms

// 20-05-16对过一次引脚配置
int main(void)
{
	SystemInit();
	if (BootCtrl_ShouldJumpToApp() != 0U)
	{
		IAP_To_APP_Jump(); // 跳回去不能开各种中断或者初始化，也即下面的初始化不能放上来
	}
	else
	{
		InitDelay();
		InitNVIC();
		InitIO();
		InitTimer();
		// InitSystemWakeUp(); // 少了这个初始化函数导致写出错，miss，让硬件整体正常工作
							// 这个到底放哪里，是取决于硬件是否需要执行一些操作菜正常工作？
							// 不过既然能执行这个函数，说明单片机已经正常工作
		// FlashTest();
		InitUSART1();
		InitUSART2();
		IapUpgrade_Init();
		CanIap_Init();
		// InitUSART3();
		while (1)
		{
			App_SysTime();
			CanIap_Task();
			if (g_st_SysTimeFlag.bits.b1Sys10msFlag)
			{
				CanIap_10msTask();
			}
			App_UpgrateFaultMonitor();
			App_FlashUpgrate();
			App_UpdateFinishChk(); // 升级结束判断
		}
	}

	return 0;
}

void jtag_disableAndConfIO(void)
{
#if 1
	/* 禁用 JTAG，PB3、PB4、PA15重定义为普通IO */
	GPIO_InitTypeDef GPIO_InitStructure;
	(void)GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE); // 使能PA和PB端口时钟

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);	 // 配置复用时钟
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // 启用SW，禁用JTAG，PA15、PB3、PB4可用

#if 0
	GPIO_ResetBits(GPIOB, GPIO_Pin_4);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_3; // 端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	   // 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	   // IO口速度为50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);				   // 根据设定参数初始化

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;		  // 端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // IO口速度为50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);			  // 根据设定参数初始化
#endif

#endif
}
void InitIO(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);  // 使能IO复用功能模块时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // 使能GPIOC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE); // 使能GPIOD时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE); // 使能GPIOE时钟
	jtag_disableAndConfIO();

    {
        GPIO_InitStructure.GPIO_Pin = PIN_AFE1_ALM | PIN_AFE1_MODE | PIN_AFE1_SHIP;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = PIN_AFE1_PRO_EN | PIN_AFE1_CTL;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
        GPIO_Init(GPIOB, &GPIO_InitStructure);
    }

    // PB15_LED1
    // GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
    // GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
    GPIO_InitStructure.GPIO_Pin = PIN_DBG_LED;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
    GPIO_Init(GPIO_DBG_LED, &GPIO_InitStructure);
    // void GPIO_SetBits(GPIO_TypeDef * GPIOx, uint16_t GPIO_Pin);
    // void GPIO_ResetBits(GPIO_TypeDef * GPIOx, uint16_t GPIO_Pin);
    // GPIO_ResetBits(GPIOB, GPIO_Pin_15);
    // GPIO_SetBits(GPIOB, GPIO_Pin_15);

    GPIO_InitStructure.GPIO_Pin = PIN_KEY1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIO_KEY1, &GPIO_InitStructure);

    {
        //???这个函数没起作用
        // GPIO_WriteBit(GPIO_M_STB, PIN_M_STB, Bit_RESET);
        // GPIO_WriteBit(GPIO_AD_EN, PIN_AD_EN, Bit_RESET);
        // GPIO_WriteBit(GPIO_BLE_EN, PIN_BLE_EN, Bit_RESET);
        // GPIO_WriteBit(GPIO_SW_EN, PIN_SW_EN, Bit_RESET);

        GPIO_SetBits(GPIO_M_STB, PIN_M_STB);
        GPIO_ResetBits(GPIO_AD_EN, PIN_AD_EN);
        // GPIO_ResetBits(GPIO_BLE_EN, PIN_BLE_EN);
        GPIO_SetBits(GPIO_BLE_EN, PIN_BLE_EN);
        GPIO_ResetBits(GPIO_CMNT_EN, PIN_CMNT_EN);
        GPIO_SetBits(GPIO_SW_EN, PIN_SW_EN);

        GPIO_InitStructure.GPIO_Pin = PIN_M_STB;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
        GPIO_Init(GPIO_M_STB, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = PIN_AD_EN;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
        GPIO_Init(GPIO_AD_EN, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = PIN_BLE_EN;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
        GPIO_Init(GPIO_BLE_EN, &GPIO_InitStructure);
        GPIO_ResetBits(GPIO_BLE_EN, PIN_BLE_EN);

        GPIO_InitStructure.GPIO_Pin = PIN_SW_EN;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
        GPIO_Init(GPIO_SW_EN, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = PIN_CMNT_EN;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // IO口速度为2MHz
        GPIO_Init(GPIO_CMNT_EN, &GPIO_InitStructure);
    }

    MCUO_PWSV_STB = 1;
    MCUO_PWSV_CTR = 0;
    MCUO_DRV_CMNT = 0;
    MCUO_AFE_SHIP = 0;
    MCUO_AFE_MODE = 0;
}
void FlashTest(void)
{
	falshcnt = BootCtrl_IsIapRequestPending();
}

void InitSystemWakeUp(void)
{
	MCUO_PWSV_STB = 1;
	MCUO_PWSV_CTR = 0;
	MCUO_DRV_CMNT = 0;
	// MCUO_BLE_EN = 1;

	MCUO_AFE_SHIP = 0;
	MCUO_AFE_MODE = 0;
}

void InitDelay(void)
{
	SysTick->CTRL &= ~(1 << 2);			// 使用外部时钟
	fac_us = SystemCoreClock / 8000000; // 为系统时钟的1/8
	fac_ms = (INT16)fac_us * 1000;		// 每个ms需要的systick时钟数
}

// 关于NVIC_PriorityGroupConfig这个函数
// https://blog.csdn.net/zhuminzeng/article/details/8880138
// 第0组：所有4位用于指定响应优先级
// 第1组：最高1位用于指定抢占式优先级，最低3位用于指定响应优先级
void InitNVIC(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); // 中断嵌套设计
}

void __delay_ms(INT16 ms)
{
	INT32 temp;
	SysTick->LOAD = (INT32)ms * fac_ms;		  // 时间加载(SysTick->LOAD为24bit)
	SysTick->VAL = 0x00;					  // 清空计数器
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // 开始倒数
	do
	{
		temp = SysTick->CTRL;
	} while (temp & 0x01 && !(temp & (1 << 16))); // 等待时间到达

	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭计数器
	SysTick->VAL = 0X00;					   // 清空计数器
}

// 使用reset不需要deinit相关寄存器
void IAP_To_APP_Jump(void)
{

	if (CanIap_IsValidAppVector(FLASH_ADDR_APP_START, CAN_IAP_APP_LIMIT_ADDR) != 0U)
	{ // 判断APP区是否有代码

		JumpAddress = *(__IO uint32_t *)(FLASH_ADDR_APP_START + 4); // Jump to user application
		Jump_To_Application = (pFunction)JumpAddress;
		__disable_irq();
		SysTick->CTRL = 0U;
		TIM_Cmd(TIM3, DISABLE);
		USART_Cmd(USART1, DISABLE);
		USART_Cmd(USART2, DISABLE);
		USART_Cmd(USART3, DISABLE);
		CAN_DeInit(CAN1);
		NVIC_DisableIRQ(TIM3_IRQn);
		NVIC_DisableIRQ(USART1_IRQn);
		NVIC_DisableIRQ(USART2_IRQn);
		NVIC_DisableIRQ(USART3_IRQn);
		NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
		NVIC_ClearPendingIRQ(TIM3_IRQn);
		NVIC_ClearPendingIRQ(USART1_IRQn);
		NVIC_ClearPendingIRQ(USART2_IRQn);
		NVIC_ClearPendingIRQ(USART3_IRQn);
		NVIC_ClearPendingIRQ(USB_LP_CAN1_RX0_IRQn);
		SCB->VTOR = FLASH_ADDR_APP_START;
		__set_CONTROL(0U);
		__set_MSP(*(__IO uint32_t *)FLASH_ADDR_APP_START); // Initialize user application's Stack Pointer
		__enable_irq();
		Jump_To_Application();							   // Jump to application
	}
}

void App_UpgrateFaultMonitor(void)
{
	if (g_st_SysTimeFlag.bits.b1Sys10msFlag)
	{
		IapUpgrade_10msTask();
	}
}

void App_FlashUpgrate(void)
{
	Sci1_Updata(&g_stCurrentMsgPtr_SCI1);
	Sci2_Updata(&g_stCurrentMsgPtr_SCI2);
	Sci3_Updata(&g_stCurrentMsgPtr_SCI3);
}

void App_UpdateFinishChk(void)
{
	if (1 == u8FlagUdFinish)
	{
		__delay_ms(10); // 没这个延时reset的时候上位机报错，导致后续不行
		u8FlagUdFinish = 0;
		__disable_fault_irq();
		MCU_RESET();
	}
}

// 030所有时钟均为向上计数
void InitTimer(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); // 时钟3使能

	// 定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = 499;						// 设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = 72;					// 设置用来作为TIMx时钟频率除数的预分频值——计数分频
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		// 设置时钟分割:TDTS = Tck_tim——时钟分频
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // TIM向上计数模式
																// 我看了，向下计数是从自动装载值递减至0，向上计数是从0增加至装载值，也就是说在中断时间上没什么区别
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);				// 根据指定的参数初始化TIMx的时间基数单位
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);					// 使能指定的TIM3中断,允许更新中断

	/*	TIM3 中断嵌套设计*/
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  // 从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM3, ENABLE); // 使能TIMx
}

void App_SysTime(void)
{
	static UINT8 u8LEDcnt = 0;
	static UINT16 s_u16Cnt10ms = 0;
	static UINT16 s_u16Cnt20ms = 0;
	static UINT16 s_u16Cnt200ms = 0;
	static UINT16 s_u16Cnt200ms2 = 10;

	g_st_SysTimeFlag.bits.b1Sys10msFlag = 0;
	g_st_SysTimeFlag.bits.b1Sys10ms1Flag = 0;
	g_st_SysTimeFlag.bits.b1Sys10ms2Flag = 0;
	g_st_SysTimeFlag.bits.b1Sys10ms3Flag = 0;
	g_st_SysTimeFlag.bits.b1Sys10ms4Flag = 0;
	if (s_u16Cnt10ms != g_u810msClockCnt) // 10ms定时标志
	{
		s_u16Cnt10ms = g_u810msClockCnt;
		switch (g_u810msClockCnt)
		{
		case 0:
			// LATC1 = ~LATC1;
			s_u16Cnt20ms++;
			g_st_SysTimeFlag.bits.b1Sys10msFlag = 1;
			break;

		case 1:
			s_u16Cnt200ms++;
			g_st_SysTimeFlag.bits.b1Sys10ms1Flag = 1;
			break;

		case 2:
			s_u16Cnt200ms2++;
			g_st_SysTimeFlag.bits.b1Sys10ms2Flag = 1;
			break;

		case 3:
			g_st_SysTimeFlag.bits.b1Sys10ms3Flag = 1;
			break;

		case 4:
			g_st_SysTimeFlag.bits.b1Sys10ms4Flag = 1;
			break;

		default:
			break;
		}
	}

	g_st_SysTimeFlag.bits.b1Sys20msFlag = 0;
	if (s_u16Cnt20ms >= 2)
	{
		s_u16Cnt20ms = 0;
		g_st_SysTimeFlag.bits.b1Sys20msFlag = 1; // 20ms定时标志
	}

	g_st_SysTimeFlag.bits.b1Sys200msFlag = 0; // 屏蔽掉能使通讯异常
	g_st_SysTimeFlag.bits.b1Sys200ms1Flag = 0;
	if (s_u16Cnt200ms2 == 20)
	{
		s_u16Cnt200ms2 = 0;
		g_st_SysTimeFlag.bits.b1Sys200ms1Flag = 1; // 200ms定时标志
	}
	else if (s_u16Cnt200ms == 20)
	{
		s_u16Cnt200ms = 0;
		// MCUO_DEBUG_LED4 = ~MCUO_DEBUG_LED4;
		// MCUO_DEBUG_LED1
		g_st_SysTimeFlag.bits.b1Sys200msFlag = 1; // 200ms定时标志
	}

	if (1 == g_st_SysTimeFlag.bits.b1Sys200msFlag)
	{
		++u8LEDcnt;
		if (u8LEDcnt >= 5)
		{
			MCUO_DEBUG_LED1 = !MCUO_DEBUG_LED1;
			u8LEDcnt = 0;
		}
	}
}

// 定时器3中断服务程序
void TIM3_IRQHandler(void) // TIM3中断
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{												// 检查TIM3更新中断发生与否
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // 清除TIMx更新中断标志
		if ((++g_u81msCnt) >= 2)					// 1ms
		{
			g_u81msCnt = 0;
			g_u81msClockCnt++;

			if (g_u81msClockCnt >= 2) // 2ms
			{
				g_u81msClockCnt = 0;
				g_u810msClockCnt++;
				if (g_u810msClockCnt >= 5) // 10ms
				{
					g_u810msClockCnt = 0;
				}
			}
		}
		// LED1=~LED1;
	}
}
