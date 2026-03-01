/*!
 * \file buzzer-api.h
 * \author Dorijan Di Zepp
 * \date 26-02-2026
 * \brief Hardware-agnostic module for buzzer timing logic.
 * This file defines the buzzer module's API to operate on the buzzer.
 */

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
 * \return BUZZER_RC_OK on success, BUZZER_RC_ERROR if critical pointers are NULL.
 */
enum BuzzerReturnCode buzzer_api_init(
    buzzer_on_callback buzzer_on,
    buzzer_off_callback buzzer_off,
    buzzer_delay_callback buzzer_play_sync,
    buzzer_tick_callback buzzer_get_tick,
    uint32_t duration);

/*!
 * \brief Plays the buzzer and blocks the CPU until duration elapses.
 * \warning Uses buzzer_delay. No other code will run.
 * \return BUZZER_RC_OK or BUZZER_RC_ERROR if not initialized.
 */
enum BuzzerReturnCode buzzer_api_play_sync();

/*!
 * \brief Starts the buzzer and returns control to the caller immediately.
 * \note Must call buzzer_async_update() periodically to stop the sound.
 * \return BUZZER_RC_OK or BUZZER_RC_ERROR if not initialized.
 */
enum BuzzerReturnCode buzzer_api_start_async();

/*!
 * \brief Manages the non-blocking countdown.
 * \note Should be called in the main loop. If the duration has passed, it 
 * automatically calls the buzzer off callback.
 * \return BUZZER_RC_PLAYING if still active, BUZZER_RC_OK if idle or just finished.
 */
enum BuzzerReturnCode buzzer_api_async_update();

/*!
 * \brief Forces the buzzer OFF and resets the buzzer state.
 * \return BUZZER_RC_OK.
 */
enum BuzzerReturnCode buzzer_api_reset();

/*!
 * \brief Updates the duration for the next play command.
 * \param duration Time in milliseconds.
 * \return BUZZER_RC_OK if duration has been changed, BUZZER_RC_ERROR otherwise.
 */
enum BuzzerReturnCode buzzer_api_set_duration(uint32_t duration);

/*!
 * \brief Sets the buzzer frequency.
 * \param frequency Frequency in Hz.
 * \return BUZZER_RC_OK if frequency has been changed, BUZZER_RC_ERROR otherwise.
 */
enum BuzzerReturnCode buzzer_api_set_frequency(uint32_t frequency);

/*!
 * \brief Sets the buzzer amplitude.
 * \param amplitude Amplitude as a percentage (0-1).
 * \return BUZZER_RC_OK if amplitude has been changed, BUZZER_RC_ERROR otherwise.
 */
enum BuzzerReturnCode buzzer_api_set_amplitude(float amplitude);

/*!
 * \brief Retrieves the current configured duration.
 * \return Duration in milliseconds.
 */
uint32_t buzzer_api_get_duration();

/*!
 * \brief Gets the currently set buzzer frequency.
 * \return Frequency in Hz.
 */
uint32_t buzzer_api_get_frequency();

/*!
 * \brief Gets the currently set buzzer amplitude.
 * \return Amplitude percentage.
 */
float buzzer_api_get_amplitude();

/*!
 * \brief Check if the buzzer is playing a sound.
 * \return Boolean, true if the buzzer is playing, false otherwise
 */
bool buzzer_api_is_playing();

#endif