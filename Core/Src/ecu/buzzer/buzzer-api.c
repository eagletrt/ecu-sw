/*!
 * \file buzzer-api.c
 * \author Dorijan Di Zepp
 * \date 2026-04-08
 * \brief Hardware-agnostic module for buzzer timing logic.
 *
 * This module manages synchronous and asynchronous timing by implementing the 
 * module's API defined in buzzer-api.h.
 */

#include <string.h>
#include "buzzer-api.h"
#include "eagletrt-api.h"

/**
 * \brief Static array of buzzer instances.
 * \details This array based approach ensures deterministic memory allocation at compile-time, 
 * avoiding the risks of dynamic allocation. It allows for constant time access 
 * using the BuzzerType enum as a direct index, keeping the handler logic generic 
 * and scalable while ensuring that multiple buzzer instances operate independently 
 * without having to modify the entire module.
 */
EAGLETRT_STATIC struct BuzzerHandler buzzer_handlers[BUZZER_TYPE_COUNT];

/*!
 * \brief Private function to check that the buzzer type is a valid one and is
 * mapped to an handler instance.
 * \param buzzer_type Enum indicating the type of buzzer to be used
 * \retval True if the buzzer type is valid
 * \retval False if the buzzer type is invalid/unknown
 */
EAGLETRT_STATIC_INLINE bool prv_is_buzzer_type_valid(enum BuzzerType buzzer_type) {
    // verify that the buzzer type is valid and it doesn't exceed the handler size
    // casting to uint32_t makes negative values wrap to huge positive values
    return ((uint32_t)buzzer_type < (uint32_t)BUZZER_TYPE_COUNT);
}

enum BuzzerReturnCode buzzer_api_init(
    buzzer_on_callback on_ptrs[BUZZER_TYPE_COUNT],
    buzzer_off_callback off_ptrs[BUZZER_TYPE_COUNT],
    buzzer_delay_callback play_sync_ptrs[BUZZER_TYPE_COUNT],
    buzzer_tick_callback get_tick_ptrs[BUZZER_TYPE_COUNT]) {

    // check EVERY array before touching a single byte of buzzer_handlers
    // check array itself
    if (on_ptrs == NULL || off_ptrs == NULL ||
        play_sync_ptrs == NULL || get_tick_ptrs == NULL) {
        return BUZZER_RC_ERROR;
    }

    // check arrays content
    for (int i = 0; i < BUZZER_TYPE_COUNT; i++) {
        if (on_ptrs[i] == NULL ||
            off_ptrs[i] == NULL ||
            play_sync_ptrs[i] == NULL ||
            get_tick_ptrs[i] == NULL) {

            /* if even ONE pointer is missing, abort completely.
            No handlers are modified, and the system stays in its 
            previous state */
            return BUZZER_RC_ERROR;
        }
    }

    // If here, we know all BUZZER_TYPE_COUNT sets are valid
    bool hardware_error_occurred = false;

    for (int i = 0; i < BUZZER_TYPE_COUNT; i++) {
        // reset handler and set new handlers
        buzzer_handlers[i] = (struct BuzzerHandler){
            .buzzer_on = on_ptrs[i],
            .buzzer_off = off_ptrs[i],
            .buzzer_play_sync = play_sync_ptrs[i],
            .buzzer_get_tick = get_tick_ptrs[i]
        };

        // ensure hardware is off
        if (buzzer_handlers[i].buzzer_off() != BUZZER_RC_OK) {
            hardware_error_occurred = true;
        }
    }

    // if at least one off() call failed, we report it
    return (hardware_error_occurred) ? BUZZER_RC_ERROR : BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_play_sync(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];

    if (buzzer_handler->buzzer_play_sync == NULL) {
        return BUZZER_RC_ERROR;
    }

    return buzzer_handler->buzzer_play_sync(buzzer_handler->frequency, buzzer_handler->amplitude, buzzer_handler->duration);
    ;
}

enum BuzzerReturnCode buzzer_api_play_async(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];

    if (buzzer_handler->buzzer_on == NULL || buzzer_handler->buzzer_off == NULL || buzzer_handler->buzzer_get_tick == NULL) {
        return BUZZER_RC_ERROR;
    }

    // Check elapsed time
    uint32_t current_time = buzzer_handler->buzzer_get_tick();

    // If NOT playing, start the buzzer
    if (!buzzer_handler->is_playing) {
        if (buzzer_handler->buzzer_on(buzzer_handler->frequency, buzzer_handler->amplitude) == BUZZER_RC_ERROR) {
            return BUZZER_RC_ERROR;
        }

        buzzer_handler->start_time = current_time;
        buzzer_handler->is_playing = true;
        return BUZZER_RC_PLAYING;
    }

    // Buzzer is currently playing, check if duration has elapsed
    if ((current_time - buzzer_handler->start_time) >= buzzer_handler->duration) {
        if (buzzer_handler->buzzer_off() != BUZZER_RC_OK) {
            return BUZZER_RC_ERROR;
        }

        buzzer_handler->is_playing = false;
        return BUZZER_RC_OK;
    }

    // Buzzer is currently playing and duration hasn't expired yet
    return BUZZER_RC_PLAYING;
}

enum BuzzerReturnCode buzzer_api_reset(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];

    // stop buzzer and clear state
    if (buzzer_handler->buzzer_off != NULL) {
        if (buzzer_handler->buzzer_off() != BUZZER_RC_OK) {
            return BUZZER_RC_ERROR;
        }
    }

    buzzer_handler->is_playing = false;
    buzzer_handler->duration = 0;
    buzzer_handler->frequency = 0;
    buzzer_handler->amplitude = 0;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_set_duration(enum BuzzerType buzzer_type, uint32_t duration) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];
    buzzer_handler->duration = duration;
    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_set_frequency(enum BuzzerType buzzer_type, uint32_t frequency) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];
    buzzer_handler->frequency = frequency;
    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_set_amplitude(enum BuzzerType buzzer_type, float amplitude) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];

    if (amplitude >= 0.0F && amplitude <= 1.0F) {
        buzzer_handler->amplitude = amplitude;
        return BUZZER_RC_OK;
    }
    return BUZZER_RC_ERROR;
}

uint32_t buzzer_api_get_duration(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return 0U;
    }

    return buzzer_handlers[buzzer_type].duration;
}

uint32_t buzzer_api_get_frequency(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return 0U;
    }

    return buzzer_handlers[buzzer_type].frequency;
}

float buzzer_api_get_amplitude(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return 0.0F;
    }

    return buzzer_handlers[buzzer_type].amplitude;
}

bool buzzer_api_is_playing(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return false;
    }

    return buzzer_handlers[buzzer_type].is_playing;
}