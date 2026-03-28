/*!
 * \file buzzer.h
 * \author Dorijan Di Zepp
 * \date 2026-03-20
 * \brief Hardware-agnostic module for buzzer timing logic.
 *
 * This module defines the buzzer handler, the return codes and the callbacks signatures
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

// R2D defines
#define BUZZER_RD2_SOUND_DURATION_MS (3000) // Sound duration in ms for the RD2

// ASSI defines
#define BUZZER_ASSI_SOUND_DURATION_MS (8000) // Sound duration in ms for the ASSI emergency
#define BUZZER_ASSI_SOUND_DUTY_CYCLE (0.5f)  // Duty cycle for the ASSI emergency sound

/*!
 * \brief Return codes for the buzzer module APIs.
 */
enum BuzzerReturnCode {
    BUZZER_RC_OK,     /*!< Operation completed successfully */
    BUZZER_RC_ERROR,  /*!< Invalid parameters or uninitialized hardware pointers */
    BUZZER_RC_PLAYING /*!< The buzzer is currently active in async mode */
};

/*!
 * \brief Selection of the buzzer to be used to play the sound.
 * This enum serves as the index for the internal handler array. 
 * \warning BUZZER_TYPE_COUNT must always remain the final element in this enum. 
 * It is used by the compiler to automatically size the internal handler array 
 * and by the module to perform boundary checks.
 * \note Each entry in this list allocates a dedicated `BuzzerHandler` structure 
 * in memory. To optimize memory usage, ensure that any unused buzzer types are 
 * commented out or removed so that BUZZER_TYPE_COUNT correctly reflects 
 * only the active buzzer instances.
 */
enum BuzzerType {
    BUZZER_TYPE_R2D,  /*!< Buzzer used for R2D sound generation */
    BUZZER_TYPE_ASSI, /*!< Buzzer used for ASSI sound generation */
    BUZZER_TYPE_COUNT /*!< Sentinel value: total number of supported buzzers */
};

/*! 
 * \brief Signature for enabling the buzzer with specific characteristics.
 * \param frequency The target frequency in Hz.
 * \param amplitude The amplitude (volume) value between 0 and 1.
 * \note Regardless of the peripheral used, the two parameters have to be defined in the
 * callback signature but in the function implementation these can be ignored (e.g. GPIO).
 */
typedef enum BuzzerReturnCode (*buzzer_on_callback)(uint32_t frequency, float amplitude);

/*!
 * \brief Signature for disabling the buzzer. 
 * \note No parameters needed for the 'Off' callback.
 */
typedef enum BuzzerReturnCode (*buzzer_off_callback)(void);

/*!
 * \brief Signature for playing the buzzer in blocking way.
 * \param frequency The target frequency in Hz.
 * \param amplitude The amplitude (volume) value between 0 and 1.
 * \param duration_ms Time to stall execution in milliseconds.
 * \note Regardless of the peripheral used, the two parameters have to be defined in the
 * callback signature but in the function implementation these can be ignored (e.g. GPIO).
 */
typedef enum BuzzerReturnCode (*buzzer_delay_callback)(uint32_t frequency, float amplitude, uint32_t duration_ms);

/*!
 * \brief Signature for fetching system uptime (e.g., HAL_GetTick).
 * \return Current system time.
 */
typedef uint32_t (*buzzer_tick_callback)(void);

/*!
 * \brief Internal state and hardware interface for the buzzer.
 * It stores the function pointers used to bridge the logic to the physical hardware.
 */
struct BuzzerHandler {
    buzzer_on_callback buzzer_on;           /*!< Callback to enable the buzzer pin */
    buzzer_off_callback buzzer_off;         /*!< Callback to disable the buzzer pin */
    buzzer_delay_callback buzzer_play_sync; /*!< Callback for synchronous blocking playing */
    buzzer_tick_callback buzzer_get_tick;   /*!< Callback to retrieve elapsed time for async logic */

    uint32_t frequency;  /*!< Desired buzzer frequency in Hz */
    float amplitude;     /*!< Desired volume as a percentage (0-1) */
    uint32_t duration;   /*!< Target sound duration in milliseconds */
    uint32_t start_time; /*!< Timestamp (ms) when async play started */
    bool is_playing;     /*!< Flag indicating if buzzer is physically active */
};

#endif