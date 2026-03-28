/*!
 * \file buzzer-api.h
 * \author Dorijan Di Zepp
 * \date 2026-03-25
 * \brief Hardware-agnostic module for buzzer timing logic.
 * This file defines the buzzer module's API to operate on the buzzer.
 */

#ifndef BUZZER_API_H
#define BUZZER_API_H

#include "buzzer.h"

/*!
 * \brief Links the logic to hardware. All playing "characteristics" (duration, frequency
  and aamplitude) can be set through the public api.
 * \note Re-calling this function while a buzzer is playing will stop the 
 * previous hardware action before linking the new ones.
 * \note Frequency and amplitude are not required during initialization as 
 * they are hardware-dependent. For simple implementations (e.g. GPIO), 
 * they can be ignored, while for PWM they can be configured via the specific APIs.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \param[in] buzzer_on Function to turn hardware ON.
 * \param[in] buzzer_off Function to turn hardware OFF.
 * \param[in] buzzer_play_sync Pointer to blocking delay function (required for sync mode).
 * \param[in] buzzer_get_tick Pointer to get system uptime (required for async mode).
 * \retval BUZZER_RC_OK on correct initialization.
 * \retval BUZZER_RC_ERROR if its was not possible to initialize the handler.
 */
enum BuzzerReturnCode buzzer_api_init(
    enum BuzzerType buzzer_type,
    buzzer_on_callback buzzer_on,
    buzzer_off_callback buzzer_off,
    buzzer_delay_callback buzzer_play_sync,
    buzzer_tick_callback buzzer_get_tick);

/*!
 * \brief Plays the buzzer and blocks the CPU until duration elapses.
 * \warning No other code will run until the buzzer stops.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \retval BUZZER_RC_OK if the buzzer played in sync correctly.
 * \retval BUZZER_RC_ERROR if it was not possible to play the buzzer.
 */
enum BuzzerReturnCode buzzer_api_play_sync(enum BuzzerType buzzer_type);

/*!
 * \brief Starts the buzzer is not already playing and monitors the playing duration
 * in a non-blocking way.
 * \note Should be called in the main loop. If the duration has passed,
 * it automatically calls the buzzer off callback.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \retval BUZZER_RC_OK if the buzzer just finished playing.
 * \retval BUZZER_RC_PLAYING if the buzzer is currently playing.
 * \retval BUZZER_RC_ERROR if it was not possible to update the state or it was not possible
 * to start/stop the buzzer.
 */
enum BuzzerReturnCode buzzer_api_play_async(enum BuzzerType buzzer_type);

/*!
 * \brief Forces the buzzer OFF and resets the buzzer state.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \retval BUZZER_RC_OK if the handler has been reset and the buzzer stopped.
 * \retval BUZZER_RC_ERROR if a reset could not be achieved or \c buzzer_type is unknown.
 */
enum BuzzerReturnCode buzzer_api_reset(enum BuzzerType buzzer_type);

/*!
 * \brief Updates the duration for the next play command.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \param[in] duration Time in milliseconds.
 * \retval BUZZER_RC_OK if \c duration has been changed.
 * \retval BUZZER_RC_ERROR if \c buzzer_type is unknownw
 */
enum BuzzerReturnCode buzzer_api_set_duration(enum BuzzerType buzzer_type, uint32_t duration);

/*!
 * \brief Sets the buzzer frequency.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \param[in] frequency Frequency in Hz.
 * \retval BUZZER_RC_OK if \c frequency has been changed.
 * \retval BUZZER_RC_ERROR if \c buzzer_type is unknownw
 */
enum BuzzerReturnCode buzzer_api_set_frequency(enum BuzzerType buzzer_type, uint32_t frequency);

/*!
 * \brief Sets the buzzer amplitude.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \param[in] amplitude Amplitude as a percentage (0-1).
 * \retval BUZZER_RC_OK if \c amplitude has been changed.
 * \retval BUZZER_RC_ERROR if \c buzzer_type is unknownw or the \c amplitude value was out of range.
 */
enum BuzzerReturnCode buzzer_api_set_amplitude(enum BuzzerType buzzer_type, float amplitude);

/*!
 * \brief Retrieves the current configured duration.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \return The play duration in milliseconds. 
 * \note Returns 0 if \c buzzer_type is invalid.
 */
uint32_t buzzer_api_get_duration(enum BuzzerType buzzer_type);

/*!
 * \brief Gets the currently set buzzer frequency.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \return The frequency in Hz. 
 * \note Returns 0 if \c buzzer_type is invalid.
 */
uint32_t buzzer_api_get_frequency(enum BuzzerType buzzer_type);

/*!
 * \brief Gets the currently set buzzer amplitude.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \return The amplitude percentage between 0.0 and 1.0. 
 * \note Returns 0 if \c buzzer_type is invalid.
 */
float buzzer_api_get_amplitude(enum BuzzerType buzzer_type);

/*!
 * \brief Check if the buzzer is playing a sound.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \retval \c true The buzzer is currently active/playing.
 * \retval \c false The buzzer is idle, or the \c buzzer_type is invalid.
 * 
 */
bool buzzer_api_is_playing(enum BuzzerType buzzer_type);

#endif