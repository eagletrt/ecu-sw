/*!
 * \file buzzer-api.c
 * \author Dorijan Di Zepp
 * \date 26-02-2026
 * \brief Hardware-agnostic module for buzzer timing logic.
 *
 * This module manages synchronous and asynchronous timing by implementing the 
 * module's API defined in buzzer-api.h.
 * \note To switch hardware behaviors (e.g., from GPIO to PWM), re-initialize the module with new callbacks.
 */

#include "buzzer-api.h"
#include "eagletrt-api.h"

EAGLETRT_STATIC struct BuzzerHandler buzzer_handler;

enum BuzzerReturnCode buzzer_api_init(
    buzzer_on_callback buzzer_on,
    buzzer_off_callback buzzer_off,
    buzzer_delay_callback buzzer_play_sync,
    buzzer_tick_callback buzzer_get_tick,
    uint32_t duration) {
    // if we are already playing, stop the current hardware
    if (buzzer_handler.is_playing) {
        if (buzzer_handler.buzzer_off != NULL) {
            buzzer_handler.buzzer_off();
        }
    }

    // validate pointers
    if (buzzer_on == NULL || buzzer_off == NULL || buzzer_play_sync == NULL || buzzer_get_tick == NULL) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.buzzer_on = buzzer_on;
    buzzer_handler.buzzer_off = buzzer_off;
    buzzer_handler.buzzer_play_sync = buzzer_play_sync;
    buzzer_handler.buzzer_get_tick = buzzer_get_tick;
    buzzer_handler.duration = duration;

    // reset state
    buzzer_handler.is_playing = false;
    buzzer_handler.frequency = 0;
    buzzer_handler.amplitude = 0;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_play_sync() {
    if (buzzer_handler.buzzer_on == NULL || buzzer_handler.buzzer_off == NULL || buzzer_handler.buzzer_play_sync == NULL) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.is_playing = true;
    if (buzzer_handler.buzzer_on(buzzer_handler.frequency, buzzer_handler.amplitude) != BUZZER_RC_OK) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.buzzer_play_sync(buzzer_handler.duration);

    if (buzzer_handler.buzzer_off() != BUZZER_RC_OK) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.is_playing = false;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_start_async() {
    if (buzzer_handler.buzzer_on == NULL || buzzer_handler.buzzer_get_tick == NULL) {
        return BUZZER_RC_ERROR;
    }

    if (buzzer_handler.buzzer_on(buzzer_handler.frequency, buzzer_handler.amplitude) != BUZZER_RC_OK) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.start_time = buzzer_handler.buzzer_get_tick();
    buzzer_handler.is_playing = true;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_async_update() {
    if (buzzer_handler.buzzer_off == NULL || buzzer_handler.buzzer_get_tick == NULL) {
        return BUZZER_RC_ERROR;
    }

    if (!buzzer_handler.is_playing) {
        return BUZZER_RC_OK;
    }

    // check elapsed time
    uint32_t current_time = buzzer_handler.buzzer_get_tick();

    if ((current_time - buzzer_handler.start_time) >= buzzer_handler.duration) {
        /* if turning off fails, leave unchanged the 'playing' state and
         report the error so the system knows the buzzer might be stuck */
        if (buzzer_handler.buzzer_off() != BUZZER_RC_OK) {
            return BUZZER_RC_ERROR;
        }

        buzzer_handler.is_playing = false;
        return BUZZER_RC_OK;
    }

    return BUZZER_RC_PLAYING;
}

enum BuzzerReturnCode buzzer_api_reset() {
    if (buzzer_handler.buzzer_off != NULL) {
        buzzer_handler.buzzer_off();
    }

    // stop buzzer and clear state
    buzzer_handler.is_playing = false;
    buzzer_handler.duration = 0;
    buzzer_handler.frequency = 0;
    buzzer_handler.amplitude = 0;

    // clear callbacks
    buzzer_handler.buzzer_on = NULL;
    buzzer_handler.buzzer_off = NULL;
    buzzer_handler.buzzer_play_sync = NULL;
    buzzer_handler.buzzer_get_tick = NULL;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_set_duration(uint32_t duration) {
    if (!buzzer_handler.is_playing){
        buzzer_handler.duration = duration;
        return BUZZER_RC_OK;
    }

    return BUZZER_RC_ERROR;
}

enum BuzzerReturnCode buzzer_api_set_frequency(uint32_t frequency) {
    if (!buzzer_handler.is_playing){
        buzzer_handler.frequency = frequency;
        return BUZZER_RC_OK;
    }

    return BUZZER_RC_ERROR;
}

enum BuzzerReturnCode buzzer_api_set_amplitude(float amplitude) {
    if(!buzzer_handler.is_playing){
        if(amplitude >= 0.0f && amplitude <= 1.0f){
            buzzer_handler.amplitude = amplitude;
            return BUZZER_RC_OK;
        }
    }

    return BUZZER_RC_ERROR;
}

uint32_t buzzer_api_get_duration() {
    return buzzer_handler.duration;
}

uint32_t buzzer_api_get_frequency() {
    return buzzer_handler.frequency;
}

float buzzer_api_get_amplitude() {
    return buzzer_handler.amplitude;
}

bool buzzer_api_is_playing() {
    return buzzer_handler.is_playing;
}