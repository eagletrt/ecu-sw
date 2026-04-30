/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "eagletrt-api.h"
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void) {

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, PTT_Pin | RTD_BUZZER_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : ENABLE_Pin */
    GPIO_InitStruct.Pin = ENABLE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ENABLE_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : PTT_Pin RTD_BUZZER_Pin */
    GPIO_InitStruct.Pin = PTT_Pin | RTD_BUZZER_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 2 */
enum BuzzerReturnCode gpio_buzzer_on(uint32_t frequency, float amplitude) {
    // gpio peripheral cannot use frequency and amplitude values
    // but the function signature has to be the same as the one defined in buzzer.h
    EAGLETRT_API_UNUSED(frequency);
    EAGLETRT_API_UNUSED(amplitude);

    HAL_GPIO_WritePin(RTD_BUZZER_GPIO_Port, RTD_BUZZER_Pin, GPIO_PIN_SET);
    // GPIO doesn't return any code
    return BUZZER_RC_OK;
}

enum BuzzerReturnCode gpio_buzzer_off() {
    HAL_GPIO_WritePin(RTD_BUZZER_GPIO_Port, RTD_BUZZER_Pin, GPIO_PIN_RESET);
    // GPIO doesn't return any code
    return BUZZER_RC_OK;
}

enum BuzzerReturnCode gpio_buzzer_play_sync(uint32_t frequency, float amplitude, uint32_t duration) {
    if (gpio_buzzer_on(frequency, amplitude) == BUZZER_RC_ERROR)
        return BUZZER_RC_ERROR;

    HAL_Delay(duration);

    return gpio_buzzer_off();
}

enum RaspReturnCode gpio_rasp_set_pin(enum RaspControlPinState pin_state) {
    switch (pin_state) {
        case RASP_CONTROL_PIN_STATE_ON:
            HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);
            break;

        case RASP_CONTROL_PIN_STATE_OFF:
            HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);
            break;

        default:
            // unknown state requested, return an error
            return RASP_RC_ERROR;
    }

    return RASP_RC_OK;
}
/* USER CODE END 2 */
