/*!
 * \file buzzer.h
 * \author Dorijan Di Zepp
 * \date 14-02-2026
 * \brief Hardware-agnostic module for buzzer timing logic.
 *
 * This module manages synchronous and asynchronous timing.
 * \note To switch hardware behaviors (e.g., from GPIO to PWM), re-initialize the module with new callbacks.
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

// R2D defines
#define BUZZER_RD2_SOUND_DURATION (3000) // Sound duration in ms for the RD2

// ASSI defines
#define BUZZER_ASSI_SOUND_DURATION (8000) // Sound duration in ms for the ASSI emergency
#define BUZZER_ASSI_SOUND_DT (50)         // Duty cycle (%) for the ASSI emergency sound

/*!
 * \brief Return codes for the buzzer module APIs.
 */
enum BuzzerReturnCode {
    BUZZER_RC_OK,     /*!< Operation completed successfully */
    BUZZER_RC_ERROR,  /*!< Invalid parameters or uninitialized hardware pointers */
    BUZZER_RC_PLAYING /*!< The buzzer is currently active in async mode */
};

/*!
 * \brief Internal state and hardware interface for the buzzer.

 * It stores the function pointers used to bridge the logic to the physical hardware.
 */
struct BuzzerHandler {
    /*!< \brief Function to physically enable the buzzer */
    enum BuzzerReturnCode (*buzzer_on)(void);

    /*!< \brief Function to physically disable the buzzer */
    enum BuzzerReturnCode (*buzzer_off)(void);

    /*!< \brief Function for blocking delay (e.g., HAL_Delay) */
    void (*buzzer_delay)(uint32_t ms);

    /*!< \brief Function to get system uptime (e.g., HAL_GetTick) */
    uint32_t (*buzzer_get_tick)(void);

    uint32_t duration;   /*!< Target sound duration in milliseconds */
    uint32_t start_time; /*!< Timestamp (ms) when async play started */
    bool is_playing;     /*!< Flag indicating if buzzer is physically active */
};

/*!
 * \brief Links the logic to hardware and sets initial duration.
 * \note Re-calling this function while a buzzer is playing will stop the 
 * previous hardware action before linking the new ones.
 * \param on_ptr Function to turn hardware ON.
 * \param off_ptr Function to turn hardware OFF.
 * \param delay_ptr Pointer to blocking delay (required for sync mode).
 * \param tick_ptr Pointer to get system uptime (required for async mode).
 * \param duration Sound duration in milliseconds.
 * \return BUZZER_RC_OK on success, BUZZER_RC_ERROR if critical pointers are NULL.
 */
enum BuzzerReturnCode buzzer_init(
    enum BuzzerReturnCode (*on_ptr)(void),
    enum BuzzerReturnCode (*off_ptr)(void),
    void (*delay_ptr)(uint32_t),
    uint32_t (*tick_ptr)(void),
    uint32_t duration);

/*!
 * \brief Plays the buzzer and blocks the CPU until duration elapses.
 * \warning Uses buzzer_delay. No other code will run.
 * \return BUZZER_RC_OK or BUZZER_RC_ERROR if not initialized.
 */
enum BuzzerReturnCode buzzer_play_sync();

/*!
 * \brief Starts the buzzer and returns control to the caller immediately.
 * \note Must call buzzer_async_update() periodically to stop the sound.
 * \return BUZZER_RC_OK or BUZZER_RC_ERROR if not initialized.
 */
enum BuzzerReturnCode buzzer_start_async();

/*!
 * \brief Manages the non-blocking countdown.
 * Should be called in the main loop. If the duration has passed, it 
 * automatically calls the buzzer off callback.
 * \return BUZZER_RC_PLAYING if still active, BUZZER_RC_OK if idle or just finished.
 */
enum BuzzerReturnCode buzzer_async_update();

/*!
 * \brief Forces the buzzer OFF and resets the internal "playing" state.
 * \return BUZZER_RC_OK.
 */
enum BuzzerReturnCode buzzer_clear();

/*!
 * \brief Updates the duration for the next play command.
 * \param duration Time in milliseconds.
 */
void buzzer_set_duration(uint32_t duration);

/*!
 * \brief Retrieves the current configured duration.
 * \return Duration in milliseconds.
 */
uint32_t buzzer_get_duration();

/**
 * \brief Check if the buzzer is playing a sound.
 * \return Boolean, true if the buzzer is playing, false otherwise
 */
bool buzzer_is_playing();

#endif