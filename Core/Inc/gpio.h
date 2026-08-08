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

#include "lights.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "buzzer.h"
#include "raspberry.h"
#include "shutdown.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */
/*!
 * \brief Direct hardware access to activate the buzzer.
 * \param frequency The target frequency in Hz.
 * \param amplitude The amplitude (volume) value between 0 and 1.
 * \note GPIO doesn't require such parameters but the function signature has to define them
 * in order to be compliant with the callback signature of the buzzer module
 */
enum BuzzerReturnCode gpio_buzzer_on(uint32_t frequency, float amplitude);

/*!
 * \brief Direct hardware access to switch off the buzzer.
 */
enum BuzzerReturnCode gpio_buzzer_off();

/*!
 * \brief Direct hardware access to activate the buzzer and play it in sync mode.
 * \param frequency The target frequency in Hz.
 * \param amplitude The amplitude (volume) value between 0 and 1.
 * \param duration The total duration of the play before stopping the buzzer.
 * \note GPIO doesn't require such parameters but the function signature has to define them
 * in order to be compliant with the callback signature of the buzzer module
 */
enum BuzzerReturnCode gpio_buzzer_play_sync(uint32_t frequency, float amplitude, uint32_t duration);

/*!
 * \brief Hardware-level implementation to control the raspberry signaling pin
 * \param[in] pin_state The desired logical state (ON/OFF).
 * \retval RASP_RC_OK if the GPIO was successfully toggled via the HAL.
 * \retval RASP_RC_ERROR if an undefined or UNKNOWN state was requested.
 */
enum RaspberryReturnCode gpio_raspberry_set_pin(enum RaspberryControlPinState pin_state);

/*!
 * \brief Hardware-level implementation to set the shutdown state of the system.
 *
 * \param[in] state The desired shutdown state (true for closed, false for open).
 *
 * \retval SHUTDOWN_RC_OK if the shutdown state was successfully set.
 * \retval SHUTDOWN_RC_ERROR if an undefined or UNKNOWN state was requested.
 */
enum ShutdownReturnCode gpio_shutdown_control_relay(bool state);

/*!
 * \brief Reads the state of the TSON button.
 *
 * \retval true if the TSON button is pressed (GPIO pin is HIGH).
 * \retval false if the TSON button is not pressed (GPIO pin is LOW).
 */
bool gpio_read_tson_button(void);

/*!
 * \brief Sets the state of a specific light.
 *
 * \param light_name The name of the light to set the state for.
 * \param state The desired state of the light (true for on, false for off).
 *
 * \retval LIGHTS_RC_OK if the light state was set successfully
 * \retval LIGHTS_RC_ERROR if there was an error setting the light state
 */
enum LightsReturnCode gpio_set_light_state(enum LightsName light_name, bool state);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */
