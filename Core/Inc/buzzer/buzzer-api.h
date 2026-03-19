/*!
 * \file buzzer-api.h
 * \author Dorijan Di Zepp
 * \date 26-02-2026
 * \brief Hardware-agnostic module for buzzer timing logic.
 * This file defines the buzzer module's API to operate on the buzzer.
 */

//TODO: update documentation for both setters and getters and is_playing

#ifndef BUZZER_API_H
#define BUZZER_API_H

#include "buzzer.h"

/*!
 * \brief Links the logic to hardware and sets initial duration.
 * \note Re-calling this function while a buzzer is playing will stop the 
 * previous hardware action before linking the new ones.
 * \note Frequency and amplitude are not required during initialization as 
 * they are hardware-dependent. For simple implementations (e.g. GPIO), 
 * they can be ignored, while for PWM they can be configured via the specific APIs.
 * \param buzzer_on Function to turn hardware ON.
 * \param buzzer_off Function to turn hardware OFF.
 * \param buzzer_play_sync Pointer to blocking delay function (required for sync mode).
 * \param buzzer_get_tick Pointer to get system uptime (required for async mode).
 * \param duration Sound duration in milliseconds.
 * \retval BUZZER_RC_OK on correct initialization.
 * \retval BUZZER_RC_ERROR if its was not possible to initialize the handler.
 */
enum BuzzerReturnCode buzzer_api_init(
    enum BuzzerType buzzer_type,
    buzzer_on_callback buzzer_on,
    buzzer_off_callback buzzer_off,
    buzzer_delay_callback buzzer_play_sync,
    buzzer_tick_callback buzzer_get_tick,
    uint32_t duration);

/*!
 * \brief Plays the buzzer and blocks the CPU until duration elapses.
 * \warning No other code will run until the buzzer stops.
 * \retval BUZZER_RC_OK if the buzzer played in sync correctly.
 * \retval BUZZER_RC_ERROR if it was not possible to play the buzzer.
 */
enum BuzzerReturnCode buzzer_api_play_sync(enum BuzzerType buzzer_mode);

/*!
 * \brief Starts the buzzer is not already playing and monitors the playing duration
 * in a non-blocking way.
 * \note Should be called in the main loop. If the duration has passed,
 * it automatically calls the buzzer off callback.
 * \retval BUZZER_RC_OK if the buzzer just finished playing.
 * \retval BUZZER_RC_PLAYING if the buzzer is currently playing.
 * \retval BUZZER_RC_ERROR if it was not possible to update the state or it was not possible
 * to start/stop the buzzer.
 */
enum BuzzerReturnCode buzzer_api_play_async(enum BuzzerType buzzer_mode);

/*!
 * \brief Forces the buzzer OFF and resets the buzzer state.
 * \retval BUZZER_RC_OK if the handler has been reset and the buzzer stopped.
 * \retval BUZZER_RC_ERROR if a reset could not be achieved.
 */
enum BuzzerReturnCode buzzer_api_reset(enum BuzzerType buzzer_mode);

/*!
 * \brief Updates the duration for the next play command.
 * \param duration Time in milliseconds.
 * \retval BUZZER_RC_OK if duration has been changed.
 */
enum BuzzerReturnCode buzzer_api_set_duration(enum BuzzerType buzzer_type, uint32_t duration);

/*!
 * \brief Sets the buzzer frequency.
 * \param frequency Frequency in Hz.
 * \retval BUZZER_RC_OK if frequency has been changed.
 */
enum BuzzerReturnCode buzzer_api_set_frequency(enum BuzzerType buzzer_type, uint32_t frequency);

/*!
 * \brief Sets the buzzer amplitude.
 * \param amplitude Amplitude as a percentage (0-1).
 * \retval BUZZER_RC_OK if amplitude has been changed.
 * \retval BUZZER_RC_ERROR if amplitude has not been changed.
 */
enum BuzzerReturnCode buzzer_api_set_amplitude(enum BuzzerType buzzer_type, float amplitude);

/*!
 * \brief Retrieves the current configured duration.
 * \return Duration in milliseconds.
 */
enum BuzzerReturnCode buzzer_api_get_duration(enum BuzzerType buzzer_type, uint32_t *out_duration);

/*!
 * \brief Gets the currently set buzzer frequency.
 * \return Frequency in Hz.
 */
enum BuzzerReturnCode buzzer_api_get_frequency(enum BuzzerType buzzer_type, uint32_t *out_frequency);

/*!
 * \brief Gets the currently set buzzer amplitude.
 * \return Amplitude percentage.
 */
enum BuzzerReturnCode buzzer_api_get_amplitude(enum BuzzerType buzzer_type, float *out_amplitude);

/*!
 * \brief Check if the buzzer is playing a sound.
 * \return Boolean, true if the buzzer is playing, false otherwise
 */
enum BuzzerReturnCode buzzer_api_is_playing(enum BuzzerType buzzer_type, bool *out_is_playing);

#endif