/*!
 * \file controls.h
 * \date 2026-10-27
 * \authors Thomas Moretti [majorfrost173@gmail.com]
 *
 * \brief Type definitions of the controls module.
 */

#ifndef CONTROLS_H
#define CONTROLS_H

#include <stdint.h>

#include "ephorus.h"

/*!
 * \brief The maximum level allowed by the controls
 */
constexpr uint8_t controls_api_max_level = 10U;

/*!
 * \brief The possible return codes of the controls module functions
 */
enum ControlsReturnCode {
    CONTROLS_RC_OK,
    CONTROLS_RC_ERROR,
};

/*!
 * \brief The name of the individual controls
 */
enum ControlsName : uint8_t {
    CONTROLS_NAME_POWER,            /*!< The level of the power control set from the steering wheel (0..=10) */
    CONTROLS_NAME_REGEN,            /*!< The level of the regenerative braking set from the steering wheel (0..=10) */
    CONTROLS_NAME_TORQUE_VECTORING, /*!< The level of the torque vectoring set from the steering wheel (0..=10) */
    CONTROLS_NAME_LAUNCH_CONTROL,   /*!< The toggle for the launch control */
    CONTROLS_NAME_COUNT,
};

/*!
 * \brief The status of the controls
 */
enum ControlsStatus : uint8_t {
    CONTROLS_STATUS_OK,    /*!< The controls are usable */
    CONTROLS_STATUS_ERROR, /*!< The controls are in an error state and shouldn't be used */
    CONTROLS_STATUS_COUNT,
};

/*!
 * \brief The handler of the module
 */
struct ControlsHandler {
    uint8_t values[CONTROLS_NAME_COUNT];            /*!< The values of the controls */
    float requested_torque_nm[EPHORUS_WHEEL_COUNT]; /*!< Raw torque requested from the controls */
    enum ControlsStatus status;                     /*!< The status of the controls */
    uint32_t last_tick_ms_can_send;                 /*!< The last tick in milliseconds when the control levels were sent */
};

#endif // CONTROLS_H
