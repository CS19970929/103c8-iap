#ifndef CONF_GPIO_H
#define CONF_GPIO_H

// #include "System_Init.h"

//#define __STM32F0__
#define __STM32F1__

#ifdef __STM32F0__
#include "stm32f0xx.h"
#endif // __STM32F0__
#ifdef __STM32F1__
#include "stm32f10x.h"
#endif // __STM32F1__

#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
//IO�ڵ�ַӳ��
#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C
#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C
#define GPIOD_ODR_Addr    (GPIOD_BASE+12) //0x4001140C
#define GPIOE_ODR_Addr    (GPIOE_BASE+12) //0x4001180C
#define GPIOF_ODR_Addr    (GPIOF_BASE+12) //0x40011A0C
#define GPIOG_ODR_Addr    (GPIOG_BASE+12) //0x40011E0C

#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808
#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08
#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008
#define GPIOD_IDR_Addr    (GPIOD_BASE+8) //0x40011408
#define GPIOE_IDR_Addr    (GPIOE_BASE+8) //0x40011808
#define GPIOF_IDR_Addr    (GPIOF_BASE+8) //0x40011A08
#define GPIOG_IDR_Addr    (GPIOG_BASE+8) //0x40011E08
 
//IO�ڲ���,ֻ�Ե�һ��IO��!
//ȷ��n��ֵС��16!
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //���
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //����

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //���
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //����

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //���
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //����

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //���
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //����

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //���
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //����

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //���
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //����

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //���
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //����

/**********************************************/
#define MCUO_DEBUG_LED1 	PBout(15)		//LED1

#define MCUO_AFE_VPRO 		PBout(0)		//AFE_VPRO
#define MCUO_AFE_CTLC 		PBout(14)		//��������


#define GPIO_SCI1_TX	     GPIOB
#define PIN_SCI1_TX	     GPIO_Pin_6

#define GPIO_SCI1_RX	     GPIOB
#define PIN_SCI1_RX	     GPIO_Pin_7
/**********************************************/
//todo 注意rtc休眠唤醒前后，外设、io状态，power
#define GPIO_CHG_IN                 GPIOA                  //5v充电识别信号；充电唤醒？什么沿；充电关放电；
#define PIN_CHG_IN                  GPIO_Pin_0

#define GPIO_INT_WK_CMNT            GPIOB                  //todo can唤醒
#define PIN_INT_WK_CMNT             GPIO_Pin_12

//todo 待确认逻辑
#define GPIO_SW                     GPIOA
#define PIN_SW                      GPIO_Pin_9

#define GPIO_AFE1_CTL        GPIOB
#define PIN_AFE1_CTL         GPIO_Pin_14
//todo afe ship pin待确认
//todo afe alm pin待确认
//todo afe mode pin待确认

#define GPIO_DBG_LED        GPIOB
#define PIN_DBG_LED         GPIO_Pin_15

//todo afe mode c073 看是否影响，输出低就行了，应该不用管，后面测试没问题直接 删掉
#define GPIO_RF_EN                  GPIOA                   //todo !!!熔断保险丝控制io
#define PIN_RF_EN                   GPIO_Pin_7

#define GPIO_AFE1_PRO_EN        GPIOB
#define PIN_AFE1_PRO_EN         GPIO_Pin_0

//todo 待确认 测温度？命名修改没问题直接 
#define GPIO_ADC_VBUS               GPIOA                   //adc输入采样，总压
#define PIN_ADC_VBUS                GPIO_Pin_1

//todo 供电 给谁的？？？
#define GPIO_CMNT_EN                GPIOA                   //(power) can供电？？？
#define PIN_CMNT_EN                 GPIO_Pin_4

#define GPIO_HT_CHG                GPIOA                   //(power) can供电？？？
#define PIN_HT_CHG                 GPIO_Pin_5

#define GPIO_ADC_NMOS        GPIOB
#define PIN_ADC_NMOS         GPIO_Pin_1

#define GPIO_M_STB                  GPIOA                   //(power)
#define PIN_M_STB                   GPIO_Pin_15

#define GPIO_AD_EN                  GPIOB                   //todo (power) ???
#define PIN_AD_EN                   GPIO_Pin_3

#define GPIO_BLE_EN                  GPIOB                   //todo (power) ???
#define PIN_BLE_EN                   GPIO_Pin_4


#define GPIO_SW_EN             GPIOB                   //(power)
#define PIN_SW_EN              GPIO_Pin_5

#endif
