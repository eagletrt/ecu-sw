/*!
 * \file pedals.h
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Module for interfacing with the pedals board.
 */

#ifndef PEDALS_H
#define PEDALS_H

#include <stdint.h>
#include <stdbool.h>

#define PEDALS_MAX_BK_PRESSURE (100.0f) // bars or PSI
#define PEDALS_MAX_TORQUE (91.0f)       // Nm

#define PEDALS_BRAKE_THRESHOLD_PCT (5.0f) // percentage

#define PEDALS_TIMEOUT_MS (100)

/*!
 * \brief Internal state of the pedals module.
 */
struct PedalsHandler {
    float throttle_pct;    /*!< Throttle in percentage 0.0 to 100.0 */
    float brake_pct;       /*!< Brake in percentage 0.0 to 100.0 */
    float brake_pressure;  /*!< Brake pressure in bars or PSI */
    bool is_available;     /*!< Health status of the pedal board connection */
    uint32_t last_rx_tick; /*!< Last time (ms) a CAN message was received */
};

/*!
 * \brief Initializes the pedals internal handler.
 */
void pedals_init();

/*!
 * \brief Updates pedal values from incoming CAN data.
 * 
 * \param id CAN message identifier.
 * \param data Pointer to the 8-byte CAN payload.
 * 
 * \note This function updates the 'last_rx_tick' to reset the timeout watchdog.
 */
void pedals_update_from_can(uint32_t id, uint8_t *data);

/*!
 * \brief Monitors communication health. 
 * 
 * \note Must be called periodically. If the timeout is 
 * exceeded, it sets is_available to false.
 */
void pedals_rx_timeout();

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