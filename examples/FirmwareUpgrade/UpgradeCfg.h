/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeCfg.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/09/03
@brief

**************************************************************************/
#ifndef _UPGRADE_CFG_H_
#define _UPGRADE_CFG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

extern int g_DebugPrintf;
#define DEBUG_PRINTF(Format,...) 		if(g_DebugPrintf > 0) printf((Format), ##__VA_ARGS__)

//#define DEBUG_PRINTF_ENABLE			1
//	
//#if (DEBUG_PRINTF_ENABLE != 0)
//#define DEBUG_PRINTF	printf
//#else
//#define DEBUG_PRINTF(...)
//#endif
	
	
#define PRINTF_PROGRESS_ENABLE		0
	
#if (PRINTF_PROGRESS_ENABLE != 0)
	#define PROGRESS_BAR_WIDTH 50
#endif

#define PRINTF_USETIME_ENABLE		1 /**< DEBUG_PRINTF_ENABLE must is 1 can use */

#ifdef __cplusplus
}
#endif

#endif  /** _UPGRADE_CFG_H_ */


