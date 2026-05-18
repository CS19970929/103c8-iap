/**
  ******************************************************************************
  * @file    Project/STM32F0xx_StdPeriph_Templates/main.h 
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    05-December-2014
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

//#define __EEPROM_TEST

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "I2C.h"
#include "Sci.h"

#include "conf.h"

//ϵͳ�洢����������ST����������ڲ�Ԥ����һ��BootLoader��Ҳ����ISP��������һ��ROM
//С������Ʒ���洢��1-32KB��	 ÿҳ1KB��ϵͳ�洢��2KB��
//��������Ʒ���洢��64-128KB��	 ÿҳ1KB��ϵͳ�洢��2KB��
//��������Ʒ���洢��256KB���ϣ�  ÿҳ2KB��ϵͳ�洢��2KB��
//�����Ͳ�Ʒ���洢��256KB���ϣ�  ÿҳ2KB��ϵͳ�洢��18KB��
#define FLASH_ADDR_IAP_START 			0x08000000
#define FLASH_ADDR_APP_START 			0x08004800			//256-18-2-2=234K������ط�����һ�����⣬�޸ĺ����6K�����淴ӳ���������Zi-dataҲ��flash�Ķ���

//#define FLASH_ADDR_UPDATE_FLAG 		0x0803F000			//IAP=18K
//#define FLASH_ADDR_SLEEP_FLAG         0x0803F800			//���߹ؼ�ָ�2K
#define FLASH_ADDR_UPDATE_FLAG 			0x0801F800			//IAP=18K
#define FLASH_ADDR_SLEEP_FLAG           0x0801FC00			//���߹ؼ�ָ�2K

#define FLASH_TO_IAP_VALUE				((UINT16)0x00AB)
#define FLASH_TO_APP_VALUE				((UINT16)0xFFFF)

#define FLASH_NORMAL_SLEEP_VALUE    	((UINT16)0x1234)
#define FLASH_DEEP_SLEEP_VALUE    		((UINT16)0x1235)
#define FLASH_HICCUP_SLEEP_VALUE    	((UINT16)0x1236)
#define FLASH_SLEEP_RESET_VALUE    		((UINT16)0xFFFF)


//10msʱ��������
#define DELAYB10MS_0MS       ((UINT16)0)            //0ms
#define DELAYB10MS_30MS      ((UINT16)3)            //30ms
#define DELAYB10MS_50MS      ((UINT16)5)            //50ms
#define DELAYB10MS_100MS     ((UINT16)10)           //100ms
#define DELAYB10MS_200MS     ((UINT16)20)           //200ms
#define DELAYB10MS_250MS     ((UINT16)25)           //250ms
#define DELAYB10MS_500MS     ((UINT16)50)           //500ms
#define DELAYB10MS_1S        ((UINT16)100)          //1s
#define DELAYB10MS_1S5       ((UINT16)150)          //1.5s
#define DELAYB10MS_2S        ((UINT16)200)          //2s
#define DELAYB10MS_2S5       ((UINT16)250)          //2.5s
#define DELAYB10MS_3S        ((UINT16)300)          //3s
#define DELAYB10MS_4S        ((UINT16)400)          // 4s
#define DELAYB10MS_5S        ((UINT16)500)          // 5s
#define DELAYB10MS_10S       ((UINT16)1000)         //10s
#define DELAYB10MS_30S       ((UINT16)3000)         //30s






union SYS_TIME{
    UINT8   all;
    struct StatusSysTimeFlagBit
    {
        UINT8 b1Sys10msFlag         :1;
        UINT8 b1Sys20msFlag         :1;
		UINT8 b1Sys200ms1Flag       :1;
		UINT8 b1Sys200msFlag        :1;
		
        UINT8 b1Sys10ms1Flag        :1;
        UINT8 b1Sys10ms2Flag        :1;
        UINT8 b1Sys10ms3Flag        :1;
        UINT8 b1Sys10ms4Flag        :1;
     }bits;
};

#define MCU_RESET()	NVIC_SystemReset()
#define UPGRATER_TIMEOUT 40

void __delay_ms(INT16 nms);

extern uint8_t u8FlagUdFinishE2PROM;
extern uint8_t u8FlagUdFinish;
extern volatile union SYS_TIME g_st_SysTimeFlag;


#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

