#ifndef CONF_H
#define CONF_H

#define  BMS_HARDWARE_VERDION_DEFAULT   "LiTech"
#define  BMS_SOFTWARE_VERDION_DEFAULT   "a009-240826-c073v1p0-16s"  //32
#define  BMS_SERIAL_NUMBER_DEFAULT  	  "LiTech"



#define SNum            16

// #define _DEBUG_
// #define __test__

// #define __new_soc_test__
// #define __SIF_FUNC__

#define __same_door__
#define __THIRD_PROTECT_FUNC_
// #define __VIRTURE_CURR__

// #define __FUNC__HEAT__
// #define __FUNC__LED__
#define __FUNC_RTC__

#define _DI_SWITCH_SYS_ONOFF	//DI??????
//#define _DI_SWITCH_DSG_ONOFF	//DI?????????????MOS
// #define _DI_SWITCH_longKEY_ONOFF

#define UART1_WAKEUP_ENABLE
// #define UART2_WAKEUP_ENABLE
#define RS485_WAKEUP_ENABLE

/*********************************************param conf*****************************************************/
// #define __lianxing__

/* 下面2行宏只能选择其一 */
#define PARAM_SAVE_TO_EEPROM			/* 参数存储到外部的EEPROM (AT24C128) */
//#define PARAM_SAVE_TO_FLASH		/* 参数存储到CPU内部Flash */

#ifdef PARAM_SAVE_TO_EEPROM
	#define PARAM_ADDR		E2P_ADDR_PARAM			/* 参数区地址 */
	#define PARAM_VER_ADD			(PARAM_ADDR - 2)							/* 参数版本 */
#endif

#ifdef PARAM_SAVE_TO_FLASH
	#define PARAM_ADDR		ADDR_FLASH_SECTOR_3			/* 0x0800C000 中间的16KB扇区用来存放参数 */
	//#define PARAM_ADDR	 ADDR_FLASH_SECTOR_11		/* 0x080E0000 Flash最后128K扇区用来存放参数 */
#endif

#define PARAM_VER			0x0832							/* 参数版本 */

#define max_cali_points		(10 + 2)
#define MAX_BATSNUM_LEN			11
/*********************************************end param conf*****************************************************/

#ifdef __test__
#define  ENTER_RTC_TIME     (7)
// #define RTC_DEEP_TIME      (3 * 10)
#define RTC_DEEP_TIME      (3 * 500 )
#define RTC_sleepTIME            3
#define RTC_SOC_OCV_TIME         5
#define APP_DEEP_TIME       20

#define SMOOTH_DELAY_COMPSEN    (5 * 1)

#define NORMAL_L3_SLEEP_UNIT  15
#define NORMAL_L2_SLEEP_UNIT  15

#define LITTLE_CURRENT_OCV_TIME     (5 * 10)
#else
// #define  ENTER_RTC_TIME     (30)
#define  ENTER_RTC_TIME     (20)
#define RTC_DEEP_TIME      (3 * 60 * 24 * 5)
// #define RTC_DEEP_TIME      (5)
#define RTC_sleepTIME            20
// #define RTC_sleepTIME            5
// #define RTC_SOC_OCV_TIME         (3 * 60 * 5)
#define RTC_SOC_OCV_TIME         (3)
#define APP_DEEP_TIME       (60 * 10)


#define SMOOTH_DELAY_COMPSEN    (5 * 60)
#define LITTLE_CURRENT_OCV_TIME     (5 * 60 * 10)
// #define LITTLE_CURRENT_OCV_TIME     (5 * 10)

#define NORMAL_L3_SLEEP_UNIT  60
#define NORMAL_L2_SLEEP_UNIT  60
#endif

/*********************************************************************************/

#define   CURR_80A      0
#define   CURR_100A     1
#define   CURR_150A     2
#define   CURR_200A     3
#define   CURR_250A     4
#define   CURR_DEFAULT     5
#define   CURR_TEST     6

#define bq76xx_afe  0
#define sh36xx      1


#define CODE_A002           0
#define CODE_A036           1
#define CODE_TEST           2
#define CODE_TEST2           3
#define CODE_A009           4
/*********************************************************************************/

#define   LEVEL_CURR      CURR_100A     
#define   AFE_TYPE        sh36xx
#define   FAC_CODE        CODE_A009    


#if (AFE_TYPE == sh36xx)

#if (LEVEL_CURR == CURR_80A)
#define CBC_DelayT		384
#define CBC_Cur_DSG		500
#else
// #define CBC_DelayT		3200
// #define CBC_Cur_DSG		11000
#define CBC_DelayT		1280
#define CBC_Cur_DSG		3000
// #define CBC_Cur_DSG		((CS_Cur_CHGmax<<2)/5)
#endif

#elif (AFE_TYPE == bq76xx_afe)

#if (LEVEL_CURR == CURR_80A)
#define CBC_DelayT		4000
#define CBC_Cur_DSG		5000
#else
// #define CBC_DelayT		4000
// #define CBC_Cur_DSG		11100
#define CBC_DelayT		500
#define CBC_Cur_DSG		670
// #define CBC_Cur_DSG		((CS_Cur_CHGmax<<2)/5)
#endif

#endif


#ifdef __FUNC__HEAT__
#define CHG_LOWTEMP_PARAM   120
#define HEAT_OPEN_CURR      50
#else
#define CHG_LOWTEMP_PARAM   380
#define HEAT_OPEN_CURR      500
#endif // DEBUG




/*********************************************************************************/

#ifdef __FUNC_RTC__
// #define __SLEEP_VNORMAL__             	2200
// #define	__SLEEP_TIMENORMAL__	          10080	
// #define __SLEEP_VLOW__     		          3000
// #define	__SLEEP_TIMEVLOW__		          1440
//天瀚参数
#define __SLEEP_VNORMAL__             	3000
#define	__SLEEP_TIMENORMAL__	          10	
#define __SLEEP_VLOW__     		          3000
#define	__SLEEP_TIMEVLOW__		          10
#else
#define __SLEEP_VNORMAL__             	4200
#define	__SLEEP_TIMENORMAL__	          10080	
#define __SLEEP_VLOW__     		          3000
#define	__SLEEP_TIMEVLOW__		          1440
#endif

#define AFE_COV_H       4200
#define AFE_CUV_H       2000
#define AFE_OCC_H       20
#define AFE_ODC_H       110

/********************************************protect conf***********************************************/

#if (FAC_CODE == CODE_A036)

#define COV_1           3550
#define COV_2           3550
#define COV_3           3600
#define COV_recover     3500
#define COV_filter1      100
#define COV_filter2     100
#define COV_filter3     200

#define CUV_1           3000
#define CUV_2           3000
#define CUV_3           3000
#define CUV_recover     3100
#define CUV_filter1      100
#define CUV_filter2     100
#define CUV_filter3     200


#define BOV_1           (355 * SNum)
#define BOV_2           (355 * SNum)
#define BOV_3           (360 * SNum)
#define BOV_recover     (350 * SNum)
#define BOV_filter1      100 
#define BOV_filter2     100 
#define BOV_filter3     100 

#define BUV_1           (300 * SNum)
#define BUV_2           (300 * SNum)
#define BUV_3           (300 * SNum)
#define BUV_recover     (310 * SNum)
#define BUV_filter1      100 
#define BUV_filter2     100 
#define BUV_filter3     100 

#define OTC_1           ((60 + 40) * 10)
#define OTC_2           ((60 + 40) * 10)
#define OTC_3           ((65 + 40) * 10)
#define OTC_recover     ((55 + 40) * 10)
#define OTC_filter1       100
#define OTC_filter2      100
#define OTC_filter3      100

#define UTC_1           ((0 + 40) * 10)
#define UTC_2           ((0 + 40) * 10)
#ifdef __FUNC__HEAT__
#if (AFE_TYPE == sh36xx)
#define UTC_3           ((-20 + 40) * 10)
#elif (AFE_TYPE == bq76xx_afe)
#define UTC_3           ((-28 + 40) * 10)
#endif
#else
#define UTC_3           ((0 + 40) * 10)
#endif // DEBUG
#define UTC_recover     ((0 + 40) * 10)
#define UTC_filter1      100
#define UTC_filter2      100
#define UTC_filter3      100

#define OTD_1           ((50 + 40) * 10)
#define OTD_2           ((60 + 40) * 10)
#define OTD_3           ((65 + 40) * 10)
#define OTD_recover     ((60 + 40) * 10)
#define OTD_filter1      100
#define OTD_filter2      100
#define OTD_filter3      100

#define UTD_1           ((-10 + 40) * 10)
#define UTD_2           ((-10 + 40) * 10)
#define UTD_3           ((-10 + 40) * 10)
#define UTD_recover     ((0 + 40) * 10)
#define UTD_filter1      100
#define UTD_filter2      100
#define UTD_filter3      100

#define mos_1           ((80 + 40) * 10)
#define mos_2           ((80 + 40) * 10)
#define mos_3           ((80 + 40) * 10)
#define mos_recover     ((60 + 40) * 10)
#define mos_filter1      100
#define mos_filter2      100
#define mos_filter3      100

#define VDELTER_1       1000
#define VDELTER_2       1000
#define VDELTER_3       1000
#define VDELTER_recover 900
#define VDELTER_filter1  100
#define VDELTER_filter2  100
#define VDELTER_filter3  100

#define socLow_1        3
#define socLow_2        2
#define socLow_3        1
#define socLow_recover  2
#define socLow_filter1   100
#define socLow_filter2   100
#define socLow_filter3   100

#if (LEVEL_CURR == CURR_80A)
#define _CURR_		80
#elif (LEVEL_CURR == CURR_100A)
#define _CURR_		100
#elif (LEVEL_CURR == CURR_150A)
#define _CURR_		150
#elif (LEVEL_CURR == CURR_200A)
#define _CURR_		200
#elif (LEVEL_CURR == CURR_250A)
#define _CURR_		250
#elif (LEVEL_CURR == CURR_TEST)
#define _CURR_		1000
#elif (LEVEL_CURR == CURR_DEFAULT)
#define _CURR_		1000
#endif

#define CURR_1C		(10 * _CURR_)
#define CURR_5C		(5 * 10 * _CURR_)

#if (LEVEL_CURR == CURR_80A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (0.7 * CURR_5C) 
#define ODC_2       (0.8 * CURR_5C) 
#define ODC_3       (CURR_5C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_100A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (500 * 10) 
#define ODC_2       (600 * 10) 
#define ODC_3       (700 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_150A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (600 * 10) 
#define ODC_2       (700 * 10) 
#define ODC_3       (800 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_200A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_250A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_TEST)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  100 

#define ODC_1       (700 * 10) 
#define ODC_2       (CURR_1C) 
#define ODC_3       (CURR_1C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  20 
#elif (LEVEL_CURR == CURR_DEFAULT)
#define OCC_1       (75) 
#define OCC_2       (75) 
#define OCC_3       (80) 
#define OCC_recover (70) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  100 

#define ODC_1       (75) 
#define ODC_2       (75) 
#define ODC_3       (80) 
#define ODC_recover (70) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  100 
#endif
// #endif

#elif (FAC_CODE == CODE_A002)

#define COV_1           3550
#define COV_2           3650
#define COV_3           3750
#define COV_recover     3650
#define COV_filter1      100
#define COV_filter2     100
#define COV_filter3     100

#define CUV_1           2700
#define CUV_2           2600
#define CUV_3           2500
#define CUV_recover     2600
#define CUV_filter1      100
#define CUV_filter2     100
#define CUV_filter3     100


#define BOV_1           (355 * SNum)
#define BOV_2           (360 * SNum)
#define BOV_3           (365 * SNum)
#define BOV_recover     (360 * SNum)
#define BOV_filter1      100 
#define BOV_filter2     100 
#define BOV_filter3     100 

#define BUV_1           (270 * SNum)
#define BUV_2           (260 * SNum)
#define BUV_3           (250 * SNum)
#define BUV_recover     (260 * SNum)
#define BUV_filter1      100 
#define BUV_filter2     100 
#define BUV_filter3     100 

#define OTC_1           ((50 + 40) * 10)
#define OTC_2           ((55 + 40) * 10)
#define OTC_3           ((65 + 40) * 10)
#define OTC_recover     ((55 + 40) * 10)
#define OTC_filter1       100
#define OTC_filter2      100
#define OTC_filter3      100

#define UTC_1           ((3 + 40) * 10)
#define UTC_2           ((0 + 40) * 10)
#ifdef __FUNC__HEAT__
#if (AFE_TYPE == sh36xx)
#define UTC_3           ((-20 + 40) * 10)
#elif (AFE_TYPE == bq76xx_afe)
#define UTC_3           ((-28 + 40) * 10)
#endif
#else
#define UTC_3           ((-2 + 40) * 10)
#endif // DEBUG
#define UTC_recover     ((0 + 40) * 10)
#define UTC_filter1      100
#define UTC_filter2      100
#define UTC_filter3      100

#define OTD_1           ((50 + 40) * 10)
#define OTD_2           ((55 + 40) * 10)
#define OTD_3           ((65 + 40) * 10)
#define OTD_recover     ((55 + 40) * 10)
#define OTD_filter1      100
#define OTD_filter2      100
#define OTD_filter3      100

#define UTD_1           ((-10 + 40) * 10)
#define UTD_2           ((-15 + 40) * 10)
#define UTD_3           ((-20 + 40) * 10)
#define UTD_recover     ((-10 + 40) * 10)
#define UTD_filter1      100
#define UTD_filter2      100
#define UTD_filter3      100

#define mos_1           ((80 + 40) * 10)
#define mos_2           ((85 + 40) * 10)
#define mos_3           ((95 + 40) * 10)
#define mos_recover     ((80 + 40) * 10)
#define mos_filter1      100
#define mos_filter2      100
#define mos_filter3      100

#define VDELTER_1       1000
#define VDELTER_2       1000
#define VDELTER_3       1000
#define VDELTER_recover 900
#define VDELTER_filter1  100
#define VDELTER_filter2  100
#define VDELTER_filter3  100

#define socLow_1        3
#define socLow_2        2
#define socLow_3        1
#define socLow_recover  2
#define socLow_filter1   100
#define socLow_filter2   100
#define socLow_filter3   100

#if (LEVEL_CURR == CURR_80A)
#define _CURR_		60
#elif (LEVEL_CURR == CURR_100A)
#define _CURR_		100
#elif (LEVEL_CURR == CURR_150A)
#define _CURR_		150
#elif (LEVEL_CURR == CURR_200A)
#define _CURR_		200
#elif (LEVEL_CURR == CURR_250A)
#define _CURR_		250
#elif (LEVEL_CURR == CURR_TEST)
#define _CURR_		1000
#endif

#define CURR_1C		(10 * _CURR_)
#define CURR_5C		(5 * CURR_1C)

#if (LEVEL_CURR == CURR_80A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (0.7 * CURR_5C) 
#define ODC_2       (0.8 * CURR_5C) 
#define ODC_3       (CURR_5C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_100A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (500 * 10) 
#define ODC_2       (600 * 10) 
#define ODC_3       (700 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_150A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (600 * 10) 
#define ODC_2       (700 * 10) 
#define ODC_3       (800 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_200A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_250A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_TEST)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  100 

#define ODC_1       (700 * 10) 
#define ODC_2       (CURR_1C) 
#define ODC_3       (CURR_1C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  20 
// #endif
#endif

#elif (FAC_CODE == CODE_A009)

#define COV_1           3500
#define COV_2           3650
#define COV_3           3750
#define COV_recover     3500
#define COV_filter1      100
#define COV_filter2     100
#define COV_filter3     100

#define CUV_1           3000
#define CUV_2           2800
#define CUV_3           2500
#define CUV_recover     2800
#define CUV_filter1      100
#define CUV_filter2     100
#define CUV_filter3     100


#define BOV_1           (350 * SNum)
#define BOV_2           (365 * SNum)
#define BOV_3           (375 * SNum)
#define BOV_recover     (345 * SNum)
// #define BOV_1           (2800)
// #define BOV_2           (2920)
// #define BOV_3           (3000)
// #define BOV_recover     (2800)
#define BOV_filter1      100 
#define BOV_filter2     100 
#define BOV_filter3     100 


#define BUV_1           (300 * SNum)
#define BUV_2           (280 * SNum)
#define BUV_3           (250 * SNum)
#define BUV_recover     (280 * SNum)
#define BUV_filter1      100 
#define BUV_filter2     100 
#define BUV_filter3     100 

#define OTC_1           ((40 + 40) * 10)
#define OTC_2           ((43 + 40) * 10)
#define OTC_3           ((45 + 40) * 10)
#define OTC_recover     ((40 + 40) * 10)
#define OTC_filter1       100
#define OTC_filter2      100
#define OTC_filter3      100

#define UTC_1           ((5 + 40) * 10)
#define UTC_2           ((3 + 40) * 10)
#ifdef __FUNC__HEAT__
#if (AFE_TYPE == sh36xx)
#define UTC_3           ((-20 + 40) * 10)
#elif (AFE_TYPE == bq76xx_afe)
#define UTC_3           ((-28 + 40) * 10)
#endif
#else
#define UTC_3           ((0 + 40) * 10)
#endif // DEBUG
#define UTC_recover     ((5 + 40) * 10)
#define UTC_filter1      100
#define UTC_filter2      100
#define UTC_filter3      100

#define OTD_1           ((60 + 40) * 10)
#define OTD_2           ((70 + 40) * 10)
#define OTD_3           ((75 + 40) * 10)
#define OTD_recover     ((60 + 40) * 10)
#define OTD_filter1      100
#define OTD_filter2      100
#define OTD_filter3      100

#define UTD_1           ((-5 + 40) * 10)
#define UTD_2           ((-10 + 40) * 10)
#define UTD_3           ((-20 + 40) * 10)
#define UTD_recover     ((0 + 40) * 10)
#define UTD_filter1      100
#define UTD_filter2      100
#define UTD_filter3      100

#define mos_1           ((75 + 40) * 10)
#define mos_2           ((80 + 40) * 10)
#define mos_3           ((85 + 40) * 10)
#define mos_recover     ((80 + 40) * 10)
#define mos_filter1      100
#define mos_filter2      100
#define mos_filter3      100

#define VDELTER_1       600
#define VDELTER_2       800
#define VDELTER_3       1000
#define VDELTER_recover 800
#define VDELTER_filter1  100
#define VDELTER_filter2  100
#define VDELTER_filter3  100

#define socLow_1        20
#define socLow_2        10
#define socLow_3        5
#define socLow_recover  11
#define socLow_filter1   100
#define socLow_filter2   100
#define socLow_filter3   100

#if (LEVEL_CURR == CURR_80A)
#define _CURR_		60
#elif (LEVEL_CURR == CURR_100A)
#define _CURR_		100
#elif (LEVEL_CURR == CURR_150A)
#define _CURR_		150
#elif (LEVEL_CURR == CURR_200A)
#define _CURR_		200
#elif (LEVEL_CURR == CURR_250A)
#define _CURR_		250
#elif (LEVEL_CURR == CURR_TEST)
#define _CURR_		1000
#endif

#define CURR_1C		(10 * _CURR_)
#define CURR_5C		(5 * CURR_1C)

#if (LEVEL_CURR == CURR_80A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (0.7 * CURR_5C) 
#define ODC_2       (0.8 * CURR_5C) 
#define ODC_3       (CURR_5C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_100A)
// #define OCC_1       (0.6 * CURR_1C) 
// #define OCC_2       (0.8 * CURR_1C) 
// #define OCC_3       (CURR_1C) 
// #define OCC_recover (0.5 * CURR_1C) 
// #define OCC_filter1  300 
// #define OCC_filter2  300 
// #define OCC_filter3  300 

// #define ODC_1       (500 * 10) 
// #define ODC_2       (600 * 10) 
// #define ODC_3       (700 * 10) 
// #define ODC_recover (0.5 * ODC_3) 
// #define ODC_filter1  30 
// #define ODC_filter2  30 
// #define ODC_filter3  30 

#define OCC_1       (200) 
#define OCC_2       (300) 
#define OCC_3       (450) 
#define OCC_recover (100) 
#define OCC_filter1  (100 * 5) 
#define OCC_filter2  (100 * 5) 
#define OCC_filter3  10 

#define ODC_1       (200) 
#define ODC_2       (300) 
#define ODC_3       (450) 
#define ODC_recover (100) 
#define ODC_filter1  (100 * 5) 
#define ODC_filter2  (100 * 5) 
#define ODC_filter3  10 

#elif (LEVEL_CURR == CURR_150A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (600 * 10) 
#define ODC_2       (700 * 10) 
#define ODC_3       (800 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_200A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_250A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_TEST)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  100 

#define ODC_1       (700 * 10) 
#define ODC_2       (CURR_1C) 
#define ODC_3       (CURR_1C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  20 
// #endif
#endif

#elif (FAC_CODE == CODE_TEST)

#define COV_1           3333
#define COV_2           3555
#define COV_3           3888
#define COV_recover     3650
#define COV_filter1     100
#define COV_filter2     100
#define COV_filter3     100

#define CUV_1           2700
#define CUV_2           2600
#define CUV_3           2580
#define CUV_recover     2600
#define CUV_filter1      100
#define CUV_filter2     100
#define CUV_filter3     100


#define BOV_1           (355 * SNum)
#define BOV_2           (360 * SNum)
#define BOV_3           (365 * SNum)
#define BOV_recover     (360 * SNum)
#define BOV_filter1      100 
#define BOV_filter2     100 
#define BOV_filter3     100 

#define BUV_1           (270 * SNum)
#define BUV_2           (260 * SNum)
#define BUV_3           (220 * SNum)
#define BUV_recover     (250 * SNum)
#define BUV_filter1      100 
#define BUV_filter2     100 
#define BUV_filter3     100 

#define OTC_1           ((50 + 40) * 10)
#define OTC_2           ((55 + 40) * 10)
#define OTC_3           ((60 + 40) * 10)
#define OTC_recover     ((55 + 40) * 10)
#define OTC_filter1       100
#define OTC_filter2      100
#define OTC_filter3      100

#define UTC_1           ((3 + 40) * 10)
#define UTC_2           ((0 + 40) * 10)
#ifdef __FUNC__HEAT__
#if (AFE_TYPE == sh36xx)
#define UTC_3           ((-20 + 40) * 10)
#elif (AFE_TYPE == bq76xx_afe)
#define UTC_3           ((-28 + 40) * 10)
#endif
#else
#define UTC_3           ((-10 + 40) * 10)
#endif // DEBUG
#define UTC_recover     ((0 + 40) * 10)
#define UTC_filter1      100
#define UTC_filter2      100
#define UTC_filter3      100

#define OTD_1           ((50 + 40) * 10)
#define OTD_2           ((55 + 40) * 10)
#define OTD_3           ((60 + 40) * 10)
#define OTD_recover     ((55 + 40) * 10)
#define OTD_filter1      100
#define OTD_filter2      100
#define OTD_filter3      100

#define UTD_1           ((-10 + 40) * 10)
#define UTD_2           ((-15 + 40) * 10)
#define UTD_3           ((-20 + 40) * 10)
#define UTD_recover     ((-10 + 40) * 10)
#define UTD_filter1      100
#define UTD_filter2      100
#define UTD_filter3      100

#define mos_1           ((70 + 40) * 10)
#define mos_2           ((70 + 40) * 10)
#define mos_3           ((70 + 40) * 10)
#define mos_recover     ((60 + 40) * 10)
#define mos_filter1      100
#define mos_filter2      100
#define mos_filter3      100

#define VDELTER_1       500
#define VDELTER_2       500
#define VDELTER_3       500
#define VDELTER_recover 400
#define VDELTER_filter1  100
#define VDELTER_filter2  100
#define VDELTER_filter3  100

#define socLow_1          10
#define socLow_2          10
#define socLow_3          10
#define socLow_recover    11
#define socLow_filter1   100
#define socLow_filter2   100
#define socLow_filter3   100

#if (LEVEL_CURR == CURR_80A)
#define _CURR_		60
#elif (LEVEL_CURR == CURR_100A)
#define _CURR_		100
#elif (LEVEL_CURR == CURR_150A)
#define _CURR_		150
#elif (LEVEL_CURR == CURR_200A)
#define _CURR_		200
#elif (LEVEL_CURR == CURR_250A)
#define _CURR_		250
#elif (LEVEL_CURR == CURR_TEST)
#define _CURR_		100
#endif

#define CURR_1C		(10 * _CURR_)
#define CURR_5C		(5 * CURR_1C)

#if (LEVEL_CURR == CURR_80A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (0.7 * CURR_5C) 
#define ODC_2       (0.8 * CURR_5C) 
#define ODC_3       (CURR_5C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_100A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (500 * 10) 
#define ODC_2       (600 * 10) 
#define ODC_3       (700 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_150A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (600 * 10) 
#define ODC_2       (700 * 10) 
#define ODC_3       (800 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_200A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_250A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_TEST)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  100 
#define OCC_filter2  100 
#define OCC_filter3  100 

#define ODC_1      (0.6 * CURR_1C)
#define ODC_2       (0.8 * CURR_1C) 
#define ODC_3       (CURR_1C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  100 
#define ODC_filter2  100
#define ODC_filter3  100 
#endif

#elif (FAC_CODE == CODE_TEST2)

#define COV_1           3600
#define COV_2           3700
#define COV_3           3800
#define COV_recover     3600
#define COV_filter1     300
#define COV_filter2     300
#define COV_filter3     300

#define CUV_1           2800
#define CUV_2           2700
#define CUV_3           2600
#define CUV_recover     2700
#define CUV_filter1     300
#define CUV_filter2     300
#define CUV_filter3     300


#define BOV_1           (355 * SNum)
#define BOV_2           (360 * SNum)
#define BOV_3           (365 * SNum)
#define BOV_recover     (360 * SNum)
#define BOV_filter1      300 
#define BOV_filter2     300 
#define BOV_filter3     300 

#define BUV_1           (270 * SNum)
#define BUV_2           (260 * SNum)
#define BUV_3           (250 * SNum)
#define BUV_recover     (260 * SNum)
#define BUV_filter1      300 
#define BUV_filter2     300 
#define BUV_filter3     300 

#define OTC_1           ((50 + 40) * 10)
#define OTC_2           ((55 + 40) * 10)
#define OTC_3           ((65 + 40) * 10)
#define OTC_recover     ((55 + 40) * 10)
#define OTC_filter1       300
#define OTC_filter2      300
#define OTC_filter3      300

#define UTC_1           ((3 + 40) * 10)
#define UTC_2           ((0 + 40) * 10)
#ifdef __FUNC__HEAT__
#if (AFE_TYPE == sh36xx)
#define UTC_3           ((-20 + 40) * 10)
#elif (AFE_TYPE == bq76xx_afe)
#define UTC_3           ((-28 + 40) * 10)
#endif
#else
#define UTC_3           ((-2 + 40) * 10)
#endif // DEBUG
#define UTC_recover     ((0 + 40) * 10)
#define UTC_filter1      300
#define UTC_filter2      300
#define UTC_filter3      300

#define OTD_1           ((50 + 40) * 10)
#define OTD_2           ((55 + 40) * 10)
#define OTD_3           ((65 + 40) * 10)
#define OTD_recover     ((55 + 40) * 10)
#define OTD_filter1      300
#define OTD_filter2      300
#define OTD_filter3      300

#define UTD_1           ((-10 + 40) * 10)
#define UTD_2           ((-15 + 40) * 10)
#define UTD_3           ((-20 + 40) * 10)
#define UTD_recover     ((-10 + 40) * 10)
#define UTD_filter1      300
#define UTD_filter2      300
#define UTD_filter3      300

#define mos_1           ((80 + 40) * 10)
#define mos_2           ((80 + 40) * 10)
#define mos_3           ((100 + 40) * 10)
#define mos_recover     ((80 + 40) * 10)
#define mos_filter1      300
#define mos_filter2      300
#define mos_filter3      300

#define VDELTER_1       1000
#define VDELTER_2       1000
#define VDELTER_3       1000
#define VDELTER_recover 900
#define VDELTER_filter1  300
#define VDELTER_filter2  300
#define VDELTER_filter3  300

#define socLow_1        30
#define socLow_2        30
#define socLow_3        30
#define socLow_recover  31
#define socLow_filter1   300
#define socLow_filter2   300
#define socLow_filter3   300

#if (LEVEL_CURR == CURR_80A)
#define _CURR_		60
#elif (LEVEL_CURR == CURR_100A)
#define _CURR_		100
#elif (LEVEL_CURR == CURR_150A)
#define _CURR_		150
#elif (LEVEL_CURR == CURR_200A)
#define _CURR_		200
#elif (LEVEL_CURR == CURR_250A)
#define _CURR_		250
#elif (LEVEL_CURR == CURR_TEST)
#define _CURR_		150
#endif

#define CURR_1C		(10 * _CURR_)
#define CURR_5C		(5 * CURR_1C)

#if (LEVEL_CURR == CURR_80A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (0.7 * CURR_5C) 
#define ODC_2       (0.8 * CURR_5C) 
#define ODC_3       (CURR_5C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_100A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (500 * 10) 
#define ODC_2       (600 * 10) 
#define ODC_3       (700 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_150A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (600 * 10) 
#define ODC_2       (700 * 10) 
#define ODC_3       (800 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_200A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_250A)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  300 

#define ODC_1       (700 * 10) 
#define ODC_2       (800 * 10) 
#define ODC_3       (900 * 10) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  30 
#define ODC_filter2  30 
#define ODC_filter3  30 
#elif (LEVEL_CURR == CURR_TEST)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  1 

#define ODC_1       (0.6 * CURR_1C) 
#define ODC_2       (0.8 * CURR_1C)  
#define ODC_3       (CURR_1C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  300
#define ODC_filter2  300 
#define ODC_filter3  2 
#elif (LEVEL_CURR == CURR_TEST2)
#define OCC_1       (0.6 * CURR_1C) 
#define OCC_2       (0.8 * CURR_1C) 
#define OCC_3       (CURR_1C) 
#define OCC_recover (0.5 * CURR_1C) 
#define OCC_filter1  300 
#define OCC_filter2  300 
#define OCC_filter3  1 

#define ODC_1       (0.6 * CURR_1C) 
#define ODC_2       (0.8 * CURR_1C)  
#define ODC_3       (CURR_1C) 
#define ODC_recover (0.5 * ODC_3) 
#define ODC_filter1  300
#define ODC_filter2  300 
#define ODC_filter3  2 

#endif

#endif

/********************************************end protect conf***********************************************/




// #define _SLEEP_WITH_CURRENT

#define __CTLC__

// #define BSP_Printf		printf
#define BSP_Printf(...)

#define DEBUG_LINE() 																												\
  BSP_Printf("Log: [%s:%s] line = %d\n", __FILE__, __func__, __LINE__)
#define DEBUG_INFO(fmt, ...)                                                \
  BSP_Printf("Log: [%s:%s] line = %d\n" fmt "\n", __FILE__, __func__, __LINE__, \
         ##__VA_ARGS__)

#define enumToStr(WEEEK)  #WEEEK 

enum irqWakeup
{
  uart1_irq = 1,
  uart2_irq,
  uart3_irq,
  PA0_irq,
  bms_keyirq,
  soc_key,
  CHG_IRQ,
  current_wake,
  chg_dsg_close,
  error_wake,
  cuv_wake,
  cov_wake,
  rs485_irq,
  NO_IRQ
};
extern enum irqWakeup g_irq_t;


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

// #define GPIO_M_CTR          GPIOA
// #define PIN_M_CTR           GPIO_Pin_8
#define GPIO_DBG_LED        GPIOB
#define PIN_DBG_LED         GPIO_Pin_15

#define GPIO_INT_WK_MCU        GPIOA
#define PIN_INT_WK_MCU         GPIO_Pin_0

#define GPIO_INT_WK_CMNT        GPIOB
#define PIN_INT_WK_CMNT         GPIO_Pin_12

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

#define GPIO_LOAD_RM        GPIOA
#define PIN_LOAD_RM         GPIO_Pin_8

#define GPIO_PORT_K1    GPIOC
#define GPIO_PIN_K1	    GPIO_Pin_13



#define MCUO_DEBUG_LED1 	PBout(15)		//LED1

//��Դģ��
#define MCUO_DRV_CMNT		PCout(12)		//
#define MCUO_PWSV_CTR		PCout(13)		//
#define MCUO_PWSV_STB		PDout(2)		//
#define MCUO_BLE_EN 		PBout(12)



//AFEģ��
#define MCUO_AFE_SHIP 		PAout(10)		//AFE_SHIP
#define MCUO_AFE_MODE 		PAout(7)		//AFE_MODE
#define MCUO_AFE_VPRO 		PBout(0)		//AFE_VPRO
#define MCUO_AFE_CTLC 		PBout(14)		//��������

//����ģ��
#define MCUO_E2PR_WP		PBout(13)	//EEPROMд����
#define MCUO_DRV_DET_CHG	PBout(3)	//
//#define MCUO_DRV_DET_LOAD	PAout(8)	//

#define MCUI_INT_WK_MCU		PAin(0)		//����MCU
#define MCUI_INT_WK_CHG		PAin(15)	//��绽��
//#define MCUI_INT_WK_LOAD	PDin(2)		//���ػ���
#define MCUI_CBC_DSG 		PBin(12)


//����ģ��
#define MCUO_MOS_PRE 		PCout(12)	//PC12�����MOS



#define MCUO_RELAY_HEAT 	PCout(6)		//���ȼ̵���
#define MCUO_RELAY_COOL 	PCout(6)		//�����̵���

#define MCUO_ENO_DO1		PCout(0)	//O��1
#define MCUO_ENO_DO2		PCout(1)	//O��2
#define MCUO_ENO_DO3		PCout(2)	//O��3
#define MCUO_ENO_DO4		PCout(3)	//O��4
#define MCUO_ENO_DO5		PBout(4)	//O��5
#define MCUO_ENO_DO6		PBout(1)	//O��6
#define MCUI_ENI_DI1		PCin(2)		//I��1
#define MCUI_ENI_DI2		PBin(3)		//I��1
#define MCUI_ENI_DI3		PAin(15)		//I��1


void InitIO(void);
void InitWakeUp_Base(void);
void InitWakeUp_NormalMode(void);
void InitWakeUp_RTCMode(void);
void InitWakeUp_DeepMode(void);
void IOstatus_Base(void);
void IOstatus_RTCMode(void);
void IOstatus_NormalMode(void);
void IOstatus_DeepMode(void);
void IORecover_RTCMode(void);
void IORecover_NormalMode(void);
void IORecover_DeepMode(void);
void Sys_StopMode(void);
void Init(void);
//todo define global enum event定义全局事件 配合enumtostr enum占用大的问题?

//fixme
//完善上位机 调试窗口 串口助手功能 常用调试指令
//自动工步 soc调试记录

//参数分区 可升级参数区、写死不可升级区、eg： erase_eeprom_times，record some important params
#endif
