/*!
 * \file buzzer.c
 * \author Dorijan Di Zepp
 * \date 14-02-2026
 * \brief Hardware-agnostic module for buzzer timing logic.
 *
 * This module manages synchronous and asynchronous timing.
 * \note To switch hardware behaviors (e.g., from GPIO to PWM), re-initialize the module with new callbacks.
 */

#include "buzzer.h"
#include "eagletrt-api.h"

EAGLETRT_STATIC struct BuzzerHandler buzzer_handler;

enum BuzzerReturnCode buzzer_init(
    buzzer_action_callback buzzer_on,
    buzzer_action_callback buzzer_off,
    buzzer_delay_callback buzzer_delay,
    buzzer_tick_callback buzzer_get_tick,
    uint32_t duration) {
    // if we are already playing, stop the current hardware
    if (buzzer_handler.is_playing) {
        if (buzzer_handler.buzzer_off != NULL) {
            buzzer_handler.buzzer_off();
        }
    }

    // validate pointers
    if (buzzer_on == NULL || buzzer_off == NULL || buzzer_delay == NULL || buzzer_get_tick == NULL) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.buzzer_on = buzzer_on;
    buzzer_handler.buzzer_off = buzzer_off;
    buzzer_handler.buzzer_delay = buzzer_delay;
    buzzer_handler.buzzer_get_tick = buzzer_get_tick;

    // reset state
    buzzer_handler.duration = duration;
    buzzer_handler.is_playing = false;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_play_sync() {
    if (buzzer_handler.buzzer_on == NULL || buzzer_handler.buzzer_off == NULL || buzzer_handler.buzzer_delay == NULL) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.is_playing = true;
    if (buzzer_handler.buzzer_on() != BUZZER_RC_OK) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.buzzer_delay(buzzer_handler.duration);

    if (buzzer_handler.buzzer_off() != BUZZER_RC_OK) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.is_playing = false;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_start_async() {
    if (buzzer_handler.buzzer_on == NULL || buzzer_handler.buzzer_get_tick == NULL) {
        return BUZZER_RC_ERROR;
    }

    if (buzzer_handler.buzzer_on() != BUZZER_RC_OK) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.start_time = buzzer_handler.buzzer_get_tick();
    buzzer_handler.is_playing = true;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_async_update() {
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

enum BuzzerReturnCode buzzer_clear() {
    if (buzzer_handler.buzzer_off == NULL) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler.buzzer_off();
    buzzer_handler.is_playing = false;
    return BUZZER_RC_OK;
}

void buzzer_set_duration(uint32_t duration) {
    if (!buzzer_handler.is_playing)
        buzzer_handler.duration = duration;
}

uint32_t buzzer_get_duration() {
    return buzzer_handler.duration;
}

bool buzzer_is_playing() {
    return buzzer_handler.is_playing;
}