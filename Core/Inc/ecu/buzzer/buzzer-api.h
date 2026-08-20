/*!
 * \file buzzer-api.h
 * \author Dorijan Di Zepp
 * \date 2026-04-07
 * \brief Hardware-agnostic module for buzzer timing logic.
 * This file defines the buzzer module's API to operate on the buzzer.
 */

#ifndef BUZZER_API_H
#define BUZZER_API_H

#include "buzzer.h"

/*!
 * \brief Bulk initializes the entire buzzer subsystem and links logic to hardware.
 * This function performs an "all-or-nothing" initialization. It first validates that
 * all provided callback arrays contain non-NULL pointers for every buzzer defined
 * by \c BUZZER_TYPE_COUNT. If valid, it initializes the internal handlers and attempts to
 * put all hardware into a safe (OFF) state.
 *
 * \note The order of callbacks in each array MUST correspond to the integer values 
 * of the \ref BuzzerType enum (e.g. index 0 must be the callback for the 
 * buzzer type with value 0).
 * 
 * \note If any hardware "off" call fails, the function will still attempt to 
 * initialize the remaining buzzers but will return \c BUZZER_RC_ERROR to 
 * indicate a hardware safe-state failure.
 * 
 * \note Re-calling this function will reset all internal states (duration, 
 * frequency, etc.) to zero for all buzzers.
 *
 * \param[in] on_ptrs Array of functions to turn hardware ON (size \c BUZZER_TYPE_COUNT).
 * \param[in] off_ptrs Array of functions to turn hardware OFF (size \c BUZZER_TYPE_COUNT).
 * \param[in] play_sync_ptrs Array of blocking delay functions for sync mode (size \c BUZZER_TYPE_COUNT).
 * \param[in] get_tick_ptrs Array of system uptime functions for async mode (size \c BUZZER_TYPE_COUNT).
 * \retval BUZZER_RC_OK All handlers initialized and hardware successfully put to OFF state.
 * \retval BUZZER_RC_ERROR If any pointer was NULL or if at least one hardware OFF command failed.
 */
enum BuzzerReturnCode buzzer_api_init(
    buzzer_on_callback on_ptrs[BUZZER_TYPE_COUNT],
    buzzer_off_callback off_ptrs[BUZZER_TYPE_COUNT],
    buzzer_delay_callback play_sync_ptrs[BUZZER_TYPE_COUNT],
    buzzer_tick_callback get_tick_ptrs[BUZZER_TYPE_COUNT]);

/*!
 * \brief Plays the buzzer and blocks the CPU until duration elapses.
 * \warning No other code will run until the buzzer stops.
 * \param[in] buzzer_type The buzzer type on which to operate
 * \retval BUZZER_RC_OK if the buzzer played in sync correctly.
 * \retval BUZZER_RC_ERROR if it was not possible to play the buzzer.
 */
enum BuzzerReturnCode buzzer_api_play_sync(enum BuzzerType buzzer_type);

/*!
 * \brief Requests a one-shot buzzer play, serviced by \ref buzzer_api_poll.
 *
 * \details Moves the buzzer to \c BUZZER_PLAY_STATE_REQUESTED so the next
 * \ref buzzer_api_poll starts it and sounds it for the currently configured
 * \c duration. Call \ref buzzer_api_set_duration (and frequency/amplitude if
 * needed) before requesting. Requesting again always restarts the sound, even
 * if it is currently playing or already finished.
 *
 * \note This is the trigger half of the "request once, poll every loop" model and
 * uses no caller-side timing state. It does not touch the hardware itself; the
 * following \ref buzzer_api_poll does.
 *
 * \param[in] buzzer_type The buzzer type on which to operate.
 * \retval BUZZER_RC_OK if the play was requested.
 * \retval BUZZER_RC_ERROR if \c buzzer_type is unknown.
 */
enum BuzzerReturnCode buzzer_api_request(enum BuzzerType buzzer_type);

/*!
 * \brief Services a requested one-shot play in a non-blocking, non-restarting way.
 *
 * \details Meant to be called every loop iteration (e.g. once per main-loop cycle),
 * even when nothing was requested. It advances the play lifecycle: it starts the
 * buzzer on the first poll after a request, keeps it on until \c duration elapses,
 * then turns it off and latches \c BUZZER_PLAY_STATE_DONE. Unlike
 * \ref buzzer_api_play_async, a finished sound is NOT restarted by further polls.
 *
 * \param[in] buzzer_type The buzzer type on which to operate.
 * \retval BUZZER_RC_PLAYING if the buzzer is currently sounding.
 * \retval BUZZER_RC_OK if nothing is playing (idle/finished) or the sound just stopped.
 * \retval BUZZER_RC_ERROR if the hardware callbacks are missing or \c buzzer_type is unknown.
 */
enum BuzzerReturnCode buzzer_api_poll(enum BuzzerType buzzer_type);

/*!
 * \brief Returns the current lifecycle state of the request/poll play model.
 * \param[in] buzzer_type The buzzer type on which to operate.
 * \return The current \ref BuzzerPlayState, or \c BUZZER_PLAY_STATE_IDLE if \c buzzer_type is invalid.
 */
enum BuzzerPlayState buzzer_api_get_play_state(enum BuzzerType buzzer_type);

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

#endif
