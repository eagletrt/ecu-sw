/*!
 * \file controls-api.h
 * \date 2026-10-27
 * \authors Thomas Moretti [majorfrost173@gmail.com]
 *
 * \brief Function declarations of the controls module.
 */

#ifndef CONTROLS_API_H
#define CONTROLS_API_H

#include "controls.h"

/*!
 * \brief Initialize the module
 */
enum ControlsReturnCode controls_api_init(void);

/*!
 * \brief Get the value of the specified \p control
 *
 * \param[in] control The name of the control
 * \return The value associated to the wanted \p control, defaults to 0 if the \p control is not valid
 */
uint8_t controls_api_get_control_value(enum ControlsName control);

/*!
 * \brief Set the \p value of the specified \p control
 *
 * \param[in] control The name of the control
 * \param[in] value The value to set the specified \p control to
 */
enum ControlsReturnCode controls_api_set_control_value(enum ControlsName control, uint8_t value);

/*!
 * \brief Get the torque of the specified \p wheel
 *
 * \param[in] wheel The name of the wheel
 * \return The requested torque to the \p wheel
 */
float controls_api_get_torque(enum EphorusWheel wheel);

/*!
 * \brief Set the \p torque of the specified \p wheel
 *
 * \param[in] wheel The name of the wheel
 * \param[in] torque The wanted torque to the \p wheel
 */
enum ControlsReturnCode controls_api_set_torque(enum EphorusWheel wheel, float torque);

/*!
 * \brief Get the current status of the controls
 *
 * \return The current status
 */
enum ControlsStatus controls_api_get_status(void);

/*!
 * \brief Set the \p status of the controls
 *
 * \param[in] status The desired status
 */
enum ControlsReturnCode controls_api_set_status(enum ControlsStatus status);

/*!
 * \brief Periodically send the controls
 *
 * \param[in] tick_ms The current time tick in ms
 */
enum ControlsReturnCode controls_api_periodically_send_controls(uint32_t tick_ms);

#endif // CONTROLS_API_H
