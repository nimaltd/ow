
/*
 * @file        ow_config.h
 * @brief       OneWire communication driver
 * @author      Nima Askari
 * @version     1.0.0
 * @license     See the LICENSE file in the root folder.
 *
 * @note        All my libraries are dual-licensed. 
 *              Please review the licensing terms before using them.
 *              For any inquiries, feel free to contact me.
 *
 * @github      https://www.github.com/nimaltd
 * @linkedin    https://www.linkedin.com/in/nimaltd
 * @youtube     https://www.youtube.com/@nimaltd
 * @instagram   https://instagram.com/github.nimaltd
 *
 * Copyright (C) 2025 Nima Askari - NimaLTD. All rights reserved.
 */

#ifndef _OW_CONFIG_H_
#define _OW_CONFIG_H_

/*************************************************************************************************/
/** Includes **/
/*************************************************************************************************/

/* USER CODE BEGIN OW_INCLUDES */

/* USER CODE END OW_INCLUDES */

/*************************************************************************************************/
/** Configurations **/
/*************************************************************************************************/

/* USER CODE BEGIN OW_CONFIGURATION */

#define OW_MAX_DATA_LEN     16
#define OW_MAX_DEVICE       5

#define OW_TIM_RST          500
#define OW_TIM_RST_DET      100
#define OW_TIM_WRITE_HIGH   70
#define OW_TIM_WRITE_LOW    10
#define OW_TIM_READ_LOW     10
#define OW_TIM_READ_SAMPLE  10
#define OW_TIM_READ_HIGH    60

/* USER CODE END OW_CONFIGURATION */

#if (OW_MAX_DATA_LEN < 8)
#error  OW_MAX_DATA_LEN should be bigger than 8!
#endif

#if ((OW_MAX_DEVICE <= 0) || (OW_MAX_DEVICE >= 255))
#error  OW_MAX_DEVICE should be btween 1 and 255!
#endif


/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

#endif /* _OW_CONFIG_H_ */
