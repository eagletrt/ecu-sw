/*!
 * \file buzzer.h
 * \author Dorijan Di Zepp
 * \date 14-02-2026
 * \brief Module for controlling the buzzer.
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

#define BUZZER_RD2_SOUND_DURATION (3000)  // Sound duration in ms for the RD2
#define BUZZER_ASSI_SOUND_DURATION (8000) // Sound duration in ms for the ASSI emergency

/*!
 * \brief Operational modes for the buzzer.
 */
enum BuzzerMode {
    BUZZER_MODE_UNKNOWN, /*! Initial/Error state. No sound will be played. */
    /*!
     * \brief Ready-To-Drive (R2D) Mode.
     * \note Rule requirement: continuous sound for 1-3 seconds when the car 
     * transitions to the "Ready" state.
     */
    BUZZER_MODE_R2D,
    /*! 
     * \brief Autonomous System Status Indicator (ASSI) Emergency Mode.
     * \note Rule requirement: intermittent sound (50% duty cycle) at 1-5Hz 
     * for 8-10 seconds during an AS Emergency.
     */
    BUZZER_MODE_ASSI
};

/*!
 * \brief Internal state of the buzzer module.
 */
struct BuzzerHandler {
    enum BuzzerMode mode; /*!< For which system is the buzzer used*/
    uint64_t duration;    /*!< Sound duration in milliseconds */
    uint64_t start_time;  /*!< Timestamp (ms) when async play started */
    bool is_playing;      /*!< Flag indicating if buzzer is currently active */
};

/*!
 * \brief Initializes the buzzer internal handler.
 *
 * \param mode The system to which the buzzer will be used for
 * \param duration The duration in ms of the sound.
 */
void buzzer_init(enum BuzzerMode mode, uint64_t duration);

/*!
 * \brief Resets the handler and forces the buzzer to go silent.
 * 
 * \attention This function ensures the hardware is physically 
 * turned off and that all internal state is cleared.
 */
void buzzer_clear();

/*!
 * \brief Allow the setting of the mode of the sound.
 * 
 * \attention The new mode will be valid for the next play.
 * 
 * \param mode The mode between R2D and ASSI
 */
void buzzer_set_mode(enum BuzzerMode mode);

/*!
 * \brief Allow the setting of the duration of the sound.
 * 
 * \attention The new duration will be valid for the next play.
 * 
 * \param duration The duration in ms of the sound to be played
 */
void buzzer_set_duration(uint64_t duration);

/*!
 * \brief Starts the buzzer immediately and blocks execution.
 * 
 * \attention This function uses HAL_Delay(). It will stop all other logic 
 * until the duration elapses.
 */
void buzzer_play_sync();

/*!
 * \brief Starts the buzzer and returns immediately.
 * 
 * \attention The buzzer will remain ON until buzzer_async_callback() is called after the 
 * required duration has passed.
 */
void buzzer_start_async();

/*!
 * \brief Periodic function to manage asynchronous timing.
 * 
 * \note This should be called in the main loop or a periodic timer interrupt.
 * It checks if the duration has elapsed and toggles the hardware OFF.
 */
void buzzer_async_update();

#endif