/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "logger-api.h"
#include "eagletrt-api.h"
/* USER CODE END Includes */

extern UART_HandleTypeDef huart4;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_UART4_Init(void);

/* USER CODE BEGIN Prototypes */
/*!
 * \brief Production hardware bridge to transmit messages over physical UART2 using the ST HAL.
 * \param[in] message Pointer to the structured PAL message containing the data payload.
 * \retval PAL_RC_OK on success.
 * \retval PAL_RC_NULL_POINTER if message or its inner payload pointer is NULL.
 * \retval PAL_RC_IO_ERROR if the underlying HAL layer reports a timeout or bus failure.
 */
enum PalReturnCode usart_logger_transmit(const struct PalMessage *message);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */
