/*!
 * \file pedals.h
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Module for managing the pedals states.
 */

#ifndef PEDALS_H
#define PEDALS_H

#include <stdint.h>
#include <stdbool.h>

#define PEDALS_MAX_BK_PRESSURE (100.0f)   // bars or PSI
#define PEDALS_MAX_TORQUE (91.0f)         // Nm
#define PEDALS_BRAKE_THRESHOLD_PCT (5.0f) // %

/*!
 * \brief Return codes for the pedals module APIs.
 */
enum PedalsReturnCode {
    PEDALS_RC_OK,   /*!< Operation completed successfully */
    PEDALS_RC_ERROR /*!< Operation NOt completed */
};

/*!
 * \brief Internal state of the pedals module.
 */
struct PedalsHandler {
    float throttle_pct;   /*!< Throttle in percentage 0.0 to 100.0 */
    float brake_pct;      /*!< Brake in percentage 0.0 to 100.0 */
    float brake_pressure; /*!< Brake pressure in bars or PSI */
    bool is_available;    /*!< Health status of the pedal board connection */
};

/*!
 * \brief Initializes the pedals internal handler.
 */
enum PedalsReturnCode pedals_init();

/*!
 * \brief Allows the setting of the throttle percentage.
 * \note The function checks if the throttle is in a valid range;
 * otherwise, the percentage will be left unchanged.
 * \param throttle_pct The throttle position in percentage (0.0f to 100.0f).
 * \return PEDALS_RC_OK on success, PEDALS_RC_ERROR in case of any error.
 */
enum PedalsReturnCode pedals_set_throttle_pct(float throttle_pct);

/*!
 * \brief Allows the setting of the brake percentage.
 * \note The function checks if the brake is in a valid range;
 * otherwise, the percentage will be left unchanged.
 * \param brake_pct The brake position in percentage (0.0f to 100.0f).
 * \return PEDALS_RC_OK on success, PEDALS_RC_ERROR in case of any error.
 */
enum PedalsReturnCode pedals_set_brake_pct(float brake_pct);

/*!
 * \brief Allows the setting of the brake pressure.
 * \note The function checks if the pressure is in a valid range;
 * otherwise, the value will be left unchanged.
 * \param brake_pressure The physical brake pressure value.
 * \return PEDALS_RC_OK on success, PEDALS_RC_ERROR in case of any error.
 */
enum PedalsReturnCode pedals_set_brake_pressure(float brake_pressure);

/*!
 * \brief Allows to modify the availability state of the pedals.
 * \param is_available Health status of the pedal board connection.
 * \return PEDALS_RC_OK on success, PEDALS_RC_ERROR in case of any error.
 */
enum PedalsReturnCode pedals_set_is_available(bool is_available);

/*!
 * \brief Function to determine the availability of the pedals
 * \return true if the pedals are availabele, false otherwise
 */
bool pedals_is_available();

/*!
 * \brief Maps current throttle percentage to the driver's requested motor torque.
 * \details This calculates the raw torque request based on the throttle position 
 * and the maximum motor torque (PEDALS_MAX_TORQUE). 
 * \note Safety limits (e.g., 80kW power limit) must be applied by 
 * the inverter control.
 * \return float Desired torque in Nm. Returns 0.0f if the pedals are unavailable.
 */
float pedals_get_requested_throttle_torque();

/*!
 * \brief Checks if the brake pedal is sufficiently pressed.
 * \details Uses a predefined threshold to determine if the 
 * driver is holding the car stationary.
 * \return true if the brake is pressed and the module is available.
 */
bool pedals_is_brake_pressed();

/*!
 * \brief Provides read-only access to the pedals state.
 * 
 * \return Constant pointer to the internal handler.
 */
const struct PedalsHandler *pedals_get_values();

#endif