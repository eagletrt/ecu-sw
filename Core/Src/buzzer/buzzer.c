/*!
 * \file buzzer.c
 * \author Dorijan Di Zepp
 * \date 14-02-2026
 * \brief Implementation of buzzer control logic and hardware wrappers.
 * \note The async_update function must be called periodically to ensure 
 * the buzzer stops after the defined duration.
 */

#include "buzzer.h"
#include "tim.h"
#include "main.h"
#include "eagletrt-api.h"

// Timer configuration for the Autonomous (ASSI) buzzer
#define AUX_BUZZER_TIM htim2
#define AUX_BUZZER_CHANNEL TIM_CHANNEL_2

#define BUZZER_ASSI_SOUND_DT (50) // Duty cycle (%) for the ASSI emergency sound

EAGLETRT_STATIC struct BuzzerHandler buzzer_handler;

/*!
 * \brief Direct hardware access to switch off the buzzer.
 */
EAGLETRT_STATIC void prv_buzzer_off(void) {
    switch (buzzer_handler.mode) {
        case BUZZER_MODE_R2D:
            HAL_GPIO_WritePin(RTD_BUZZER_GPIO_Port, RTD_BUZZER_Pin, GPIO_PIN_RESET);
            break;

        case BUZZER_MODE_ASSI:
            /* Note: Must use the Channel define (e.g., TIM_CHANNEL_1) */
            HAL_TIM_PWM_Stop(&AUX_BUZZER_TIM, AUX_BUZZER_CHANNEL);
            break;

        default:
            // if mode is unknown, attempt to kill both
            HAL_GPIO_WritePin(RTD_BUZZER_GPIO_Port, RTD_BUZZER_Pin, GPIO_PIN_RESET);
            HAL_TIM_PWM_Stop(&AUX_BUZZER_TIM, AUX_BUZZER_CHANNEL);
            break;
    }
}

/*!
 * \brief Direct hardware access to activate the buzzer with predefined duty cycles.
 */
EAGLETRT_STATIC void prv_buzzer_on() {
    switch (buzzer_handler.mode) {
        case BUZZER_MODE_R2D:
            HAL_GPIO_WritePin(RTD_BUZZER_GPIO_Port, RTD_BUZZER_Pin, GPIO_PIN_SET);
            break;

        case BUZZER_MODE_ASSI: {
            // brackets required for variable declaration inside switch
            uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&AUX_BUZZER_TIM);
            uint32_t pulse = (arr * BUZZER_ASSI_SOUND_DT) / 100;

            __HAL_TIM_SET_COMPARE(&AUX_BUZZER_TIM, AUX_BUZZER_CHANNEL, pulse);
            HAL_TIM_PWM_Start(&AUX_BUZZER_TIM, AUX_BUZZER_CHANNEL);
            break;
        }

        default:
            // BUZZER_MODE_UNKNOWN
            break;
    }
}

void buzzer_init(enum BuzzerMode mode, uint64_t duration) {
    buzzer_handler.mode = mode;
    buzzer_handler.duration = duration;
    buzzer_handler.start_time = 0;
    buzzer_handler.is_playing = false;
    prv_buzzer_off(); // in case the pin floated high
}

void buzzer_clear() {
    prv_buzzer_off();
    buzzer_handler.mode = BUZZER_MODE_UNKNOWN;
    buzzer_handler.is_playing = false;
    buzzer_handler.duration = 0;
    buzzer_handler.start_time = 0;
}

void buzzer_set_mode(enum BuzzerMode mode) {
    if (buzzer_handler.is_playing) {
        prv_buzzer_off(); // turn off current "buzzer" BEFORE changing the mode
    }
    buzzer_handler.mode = mode;
}

void buzzer_set_duration(uint64_t duration) {
    buzzer_handler.duration = duration;
}

void buzzer_play_sync() {
    if (buzzer_handler.duration == 0 || buzzer_handler.mode == BUZZER_MODE_UNKNOWN)
        return;

    prv_buzzer_on();

    HAL_Delay(buzzer_handler.duration);
    prv_buzzer_off();
}

void buzzer_start_async() {
    if (buzzer_handler.is_playing || buzzer_handler.duration == 0 ||
        buzzer_handler.mode == BUZZER_MODE_UNKNOWN)
        return;

    buzzer_handler.start_time = HAL_GetTick();
    buzzer_handler.is_playing = true;

    prv_buzzer_on();
}

void buzzer_async_update() {
    if (!buzzer_handler.is_playing)
        return;

    if ((HAL_GetTick() - buzzer_handler.start_time) >= buzzer_handler.duration) {
        prv_buzzer_off();
        buzzer_handler.is_playing = false;
    }
}