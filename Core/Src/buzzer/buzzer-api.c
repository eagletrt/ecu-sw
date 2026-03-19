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
bool prv_is_buzzer_type_valid(enum BuzzerType buzzer_type) {
    // verify that the buzzer type is valid and it doesn't exceed the handler size
    return (buzzer_type < BUZZER_TYPE_COUNT);
}

enum BuzzerReturnCode buzzer_api_init(
    enum BuzzerType buzzer_type,
    buzzer_on_callback buzzer_on,
    buzzer_off_callback buzzer_off,
    buzzer_delay_callback buzzer_play_sync,
    buzzer_tick_callback buzzer_get_tick,
    uint32_t duration) {

    // validate mode
    if (!prv_is_buzzer_type_valid(buzzer_type))
        return BUZZER_RC_ERROR;

    // validate pointers
    if (buzzer_on == NULL || buzzer_off == NULL || buzzer_play_sync == NULL || buzzer_get_tick == NULL)
        return BUZZER_RC_ERROR;

    // stop and clear the buzzer handler
    if (buzzer_api_reset(buzzer_type) == BUZZER_RC_ERROR)
        return BUZZER_RC_ERROR;

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];
    buzzer_handler->buzzer_on = buzzer_on;
    buzzer_handler->buzzer_off = buzzer_off;
    buzzer_handler->buzzer_play_sync = buzzer_play_sync;
    buzzer_handler->buzzer_get_tick = buzzer_get_tick;
    buzzer_handler->duration = duration;

    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_play_sync(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];

    if (buzzer_handler->buzzer_play_sync == NULL) {
        return BUZZER_RC_ERROR;
    }

    buzzer_handler->is_playing = true;
    enum BuzzerReturnCode rc = buzzer_handler->buzzer_play_sync(buzzer_handler->frequency, buzzer_handler->amplitude, buzzer_handler->duration);
    buzzer_handler->is_playing = false;

    return rc;
}

enum BuzzerReturnCode buzzer_api_play_async(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];

    if (buzzer_handler->buzzer_on == NULL || buzzer_handler->buzzer_off == NULL || buzzer_handler->buzzer_get_tick == NULL) {
        return BUZZER_RC_ERROR;
    }

    // check elapsed time
    uint32_t current_time = buzzer_handler->buzzer_get_tick();

    if (!buzzer_handler->is_playing) {
        // if NOT playing, start the buzzer
        if (buzzer_handler->buzzer_on(buzzer_handler->frequency, buzzer_handler->amplitude) == BUZZER_RC_ERROR)
            return BUZZER_RC_ERROR;

        buzzer_handler->start_time = current_time;
        buzzer_handler->is_playing = true;
        return BUZZER_RC_PLAYING;
    }

    // buzzer is currently playing
    if ((current_time - buzzer_handler->start_time) >= buzzer_handler->duration) {
        if (buzzer_handler->buzzer_off() == BUZZER_RC_ERROR)
            return BUZZER_RC_ERROR;

        buzzer_handler->is_playing = false;
        return BUZZER_RC_OK;
    }

    return BUZZER_RC_PLAYING;
}

enum BuzzerReturnCode buzzer_api_reset(enum BuzzerType buzzer_type) {
    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    struct BuzzerHandler *buzzer_handler = &buzzer_handlers[buzzer_type];

    if (buzzer_handler->buzzer_off != NULL) {
        if (buzzer_handler->buzzer_off() == BUZZER_RC_ERROR)
            return BUZZER_RC_ERROR;
    }

    // stop buzzer and clear state
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

    if (amplitude >= 0.0f && amplitude <= 1.0f) {
        buzzer_handler->amplitude = amplitude;
        return BUZZER_RC_OK;
    }
    return BUZZER_RC_ERROR;
}

enum BuzzerReturnCode buzzer_api_get_duration(enum BuzzerType buzzer_type, uint32_t *out_duration) {
    if (out_duration == NULL)
        return BUZZER_RC_ERROR;

    if (!prv_is_buzzer_type_valid(buzzer_type))
        return BUZZER_RC_ERROR;

    *out_duration = buzzer_handlers[buzzer_type].duration;
    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_get_frequency(enum BuzzerType buzzer_type, uint32_t *out_frequency) {
    if (out_frequency == NULL)
        return BUZZER_RC_ERROR;

    if (!prv_is_buzzer_type_valid(buzzer_type))
        return BUZZER_RC_ERROR;

    *out_frequency = buzzer_handlers[buzzer_type].frequency;
    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_get_amplitude(enum BuzzerType buzzer_type, float *out_amplitude) {
    if (out_amplitude == NULL)
        return BUZZER_RC_ERROR;

    if (!prv_is_buzzer_type_valid(buzzer_type))
        return BUZZER_RC_ERROR;

    *out_amplitude = buzzer_handlers[buzzer_type].amplitude;
    return BUZZER_RC_OK;
}

enum BuzzerReturnCode buzzer_api_is_playing(enum BuzzerType buzzer_type, bool *out_is_playing) {
    if (out_is_playing == NULL)
        return BUZZER_RC_ERROR;

    if (!prv_is_buzzer_type_valid(buzzer_type)) {
        return BUZZER_RC_ERROR;
    }

    *out_is_playing = buzzer_handlers[buzzer_type].is_playing;

    return BUZZER_RC_OK;
}