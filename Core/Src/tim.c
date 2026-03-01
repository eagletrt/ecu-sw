/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
#include "tim.h"

/* USER CODE BEGIN 0 */
#include "eagletrt-api.h"

// ASSI tim peripheral
#define ASSI_BUZZER_TIM htim2
#define ASSI_BUZZER_CHANNEL TIM_CHANNEL_2
/* USER CODE END 0 */

TIM_HandleTypeDef htim2;

/* TIM2 init function */
void MX_TIM2_Init(void) {

    /* USER CODE BEGIN TIM2_Init 0 */

    /* USER CODE END TIM2_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
    TIM_MasterConfigTypeDef sMasterConfig = { 0 };
    TIM_OC_InitTypeDef sConfigOC = { 0 };

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 4294967295;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM2_Init 2 */

    /* USER CODE END TIM2_Init 2 */
    HAL_TIM_MspPostInit(&htim2);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle) {

    if (tim_baseHandle->Instance == TIM2) {
        /* USER CODE BEGIN TIM2_MspInit 0 */

        /* USER CODE END TIM2_MspInit 0 */
        /* TIM2 clock enable */
        __HAL_RCC_TIM2_CLK_ENABLE();

        /* TIM2 interrupt Init */
        HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
        /* USER CODE BEGIN TIM2_MspInit 1 */

        /* USER CODE END TIM2_MspInit 1 */
    }
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle) {

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (timHandle->Instance == TIM2) {
        /* USER CODE BEGIN TIM2_MspPostInit 0 */

        /* USER CODE END TIM2_MspPostInit 0 */

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM2 GPIO Configuration
    PB3     ------> TIM2_CH2
    */
        GPIO_InitStruct.Pin = AUX_BUZZER_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        HAL_GPIO_Init(AUX_BUZZER_GPIO_Port, &GPIO_InitStruct);

        /* USER CODE BEGIN TIM2_MspPostInit 1 */

        /* USER CODE END TIM2_MspPostInit 1 */
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle) {

    if (tim_baseHandle->Instance == TIM2) {
        /* USER CODE BEGIN TIM2_MspDeInit 0 */

        /* USER CODE END TIM2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM2_CLK_DISABLE();

        /* TIM2 interrupt Deinit */
        HAL_NVIC_DisableIRQ(TIM2_IRQn);
        /* USER CODE BEGIN TIM2_MspDeInit 1 */

        /* USER CODE END TIM2_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
enum BuzzerReturnCode tim_buzzer_on(uint32_t frequency, float amplitude) {
    //NOTE: because the buzzer characteristics are still not known
    // there is no check about the requested frequency and amplitude

    // calculate ARR based on target frequency
    // ARR = (f_clk / (f_pwm * (PSC + 1))) - 1
    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq();
    uint32_t psc = ASSI_BUZZER_TIM.Instance->PSC;

    // ensure no division by zero
    uint32_t arr = (timer_clk / (frequency * (psc + 1))) - 1;
    __HAL_TIM_SET_AUTORELOAD(&ASSI_BUZZER_TIM, arr);

    // calculate pulse (CCR) for Duty Cycle
    // since amplitude is [0, 1], pulse = ARR * amplitude
    uint32_t pulse = (uint32_t)((float)arr * amplitude);
    __HAL_TIM_SET_COMPARE(&ASSI_BUZZER_TIM, ASSI_BUZZER_CHANNEL, pulse);

    // "flush" ARR and CCR registers
    ASSI_BUZZER_TIM.Instance->EGR = TIM_EGR_UG;

    if (HAL_TIM_PWM_Start(&ASSI_BUZZER_TIM, ASSI_BUZZER_CHANNEL) != HAL_OK) {
        return BUZZER_RC_ERROR;
    }

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode tim_buzzer_off() {
    if (HAL_TIM_PWM_Stop(&ASSI_BUZZER_TIM, ASSI_BUZZER_CHANNEL) != HAL_OK) {
        return BUZZER_RC_ERROR;
    }
    return BUZZER_RC_OK;
}
/* USER CODE END 1 */
