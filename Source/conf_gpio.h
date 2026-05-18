#ifndef CONF_GPIO_H
#define CONF_GPIO_H

//#include "System_Init.h"

//#define __STM32F0__
#define __STM32F1__

#ifdef __STM32F0__
#include "stm32f0xx.h"
#endif // __STM32F0__
#ifdef __STM32F1__
#include "stm32f10x.h"
#endif // __STM32F1__

//λ������,ʵ��51���Ƶ�GPIO���ƹ���
//����ʵ��˼��,�ο�<<CM3Ȩ��ָ��>>������(87ҳ~92ҳ).
//IO�ڲ����궨��
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



#define MCUO_DEBUG_LED1 	PBout(15)		//LED1


/**********************************************/
#define GPIO_INT_WK_MCU         GPIOA
#define PIN_INT_WK_MCU          GPIO_Pin_0


#define GPIO_M_STB          GPIOA
#define PIN_M_STB           GPIO_Pin_15

#define GPIO_AD_EN        GPIOB
#define PIN_AD_EN         GPIO_Pin_3

#define GPIO_BLE_EN        GPIOB
#define PIN_BLE_EN         GPIO_Pin_4

#define GPIO_SW_EN        GPIOB
#define PIN_SW_EN         GPIO_Pin_5


#define GPIO_CMNT_EN        GPIOA
#define PIN_CMNT_EN         GPIO_Pin_4

#define GPIO_KEY1        GPIOA
#define PIN_KEY1         GPIO_Pin_9

#define GPIO_AFE1_CTL        GPIOB
#define PIN_AFE1_CTL         GPIO_Pin_14

#define GPIO_AFE1_SHIP        GPIOA
#define PIN_AFE1_SHIP         GPIO_Pin_10

#define GPIO_AFE1_ALM        GPIOA
#define PIN_AFE1_ALM         GPIO_Pin_6

#define GPIO_AFE1_MODE        GPIOA
#define PIN_AFE1_MODE         GPIO_Pin_7

#define GPIO_AFE1_PRO_EN        GPIOB
#define PIN_AFE1_PRO_EN         GPIO_Pin_0

#define GPIO_DBG_LED        GPIOB
#define PIN_DBG_LED         GPIO_Pin_15

#define MCUO_DEBUG_LED1 	PBout(15)		//LED1

#define MCUO_DRV_CMNT		PCout(12)		//
#define MCUO_DRV_BLE		PBout(4)		//
//��Դģ��
#define MCUO_PWSV_CTR		PCout(13)		//
#define MCUO_PWSV_STB		PDout(2)		//

#define MCUO_AFE_SHIP 		PAout(10)		//AFE_SHIP
#define MCUO_AFE_MODE 		PAout(7)		//AFE_MODE
#define MCUO_AFE_VPRO 		PBout(0)		//AFE_VPRO
#define MCUO_AFE_CTLC 		PBout(14)		//��������


#define GPIO_AD_TTC_MOS1             GPIOA 
#define PIN_AD_TTC_MOS1              GPIO_Pin_1

#define GPIO_SCI1_TX	     GPIOB
#define PIN_SCI1_TX	     GPIO_Pin_6

#define GPIO_SCI1_RX	     GPIOB
#define PIN_SCI1_RX	     GPIO_Pin_7

#define GPIO_INT_WK_CMNT         GPIOB
#define PIN_INT_WK_CMNT          GPIO_Pin_12

#endif

