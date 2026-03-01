/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "buzzer.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */
/*!
 * \brief Direct hardware access to activate the buzzer.
 * \param frequency The target frequency in Hz.
 * \param ampltude THe amplitude (volume) value between 0 and 1.
 * \note GPIO doesn't require such parameters but the function signature has to define them
 * in order to be compliant with the callback signature of the buzzer module
 */
enum BuzzerReturnCode gpio_buzzer_on(uint32_t frequency, float amplitude);

/*!
 * \brief Direct hardware access to switch off the buzzer.
 */
enum BuzzerReturnCode gpio_buzzer_off();
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */
