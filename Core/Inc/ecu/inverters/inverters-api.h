/*!
 * \file inverters-api.h
 * \author Dorijan Di Zepp, Alessandro Bridi
 * \date 2026-07-31
 * \brief Public API for the inverters control module.
 */

#ifndef INVERTERS_API_H
#define INVERTERS_API_H

#include <stdbool.h>
#include <stdint.h>

#include "inverters.h"
#include "can-communication.h"

/*!
 * \brief Initialize the module: reset the driver and attach all four wheels.
 * 
 * \details Clears the pending torque requests and battery SoC. Every wheel starts
 *     disarmed, so nothing is commanded until it is armed and set to run.
 *      
 * \retval INVERTERS_RC_OK If the module is fully initialized and ready.
 * \retval INVERTERS_RC_ERROR If initialization failed.
 */
enum InvertersReturnCode inverters_api_init(void);

/*!
 * \brief Attach (activate) a wheel so it transmits and decodes.
 *
 * \param wheel Wheel to activate.
 *
 * \retval INVERTERS_RC_OK on success.
 * \retval INVERTERS_RC_INVALID_WHEEL if \p wheel is out of range.
 */
enum InvertersReturnCode inverters_api_attach(enum EphorusWheel wheel);

/*! 
 * \brief Arm a wheel (clear latched errors, enable drive).
 *
 * \param wheel Wheel to arm.
 */
void inverters_api_arm(enum EphorusWheel wheel);

/*!
 * \brief Disarm a wheel (disable drive).
 *
 * \param wheel Wheel to disarm.
 */
void inverters_api_disarm(enum EphorusWheel wheel);

/*!
 * \brief Set a wheel's signed torque request [Nm] (positive drives, negative brakes).
 *
 * \details The request is stored raw; the cut-off safety layer is applied across
 *     all four wheels at \ref inverters_api_step time before serialization.
 *
 * \param wheel Wheel to command.
 * \param nm Signed torque request [Nm].
 */
void inverters_api_set_torque(enum EphorusWheel wheel, float torque_nm);

/*!
 * \brief Returns true if all wheels are in the drive state.
 *
 * \return true if all wheels are in the drive state, else false.
 */
bool inverters_api_is_all_in_drive(void);

/*!
 * \brief Set the current SoC (State of Charge) of the battery.
 *
 * \details The value is automatically clamped into the range 0.0 and 1.0. It feeds
 *     the voltage-sag and battery-power limits of the cut-off layer.
 *
 * \param hv_bms_soc The current State of Charge of the battery.
 */
void inverters_api_set_soc(float hv_bms_soc);

/*!
 * \brief Apply the cut-off safety layer and queue setpoint frames for every wheel.
 *
 * \details No-op until EPHORUS_TX_PERIOD_MS has elapsed, so it is safe (and
 *     intended) to call on every main-loop iteration. When due, it applies the
 *     internal "cut-off" safety layer to the four pending torque requests - the
 *     80 kW power limit, battery-current and regen constraints, per-motor torque
 *     limits and voltage-sag protection - then serializes and pushes a setpoints
 *     frame per attached wheel into the can-communication TX queue. Call
 *     can_communication_api_process_tx afterwards to flush them.
 *
 * \param tick Current tick.
 *
 * \retval INVERTERS_RC_OK frames queued or not yet due.
 * \retval INVERTERS_RC_TX_ERROR a TX queue rejected a frame.
 */
enum InvertersReturnCode inverters_api_step(uint32_t tick);

/*!
 * \brief Borrow a wheel's telemetry (NULL if \p wheel out of range).
 *
 * \param wheel Wheel to query.
 *
 * \return Pointer to the wheel's telemetry (never NULL if \p wheel is valid).
 */
const struct EphorusWheelTelemetry *inverters_api_get_wheel_telemetry(enum EphorusWheel wheel);

/*!
 * \brief Borrow the shared telemetry.
 *
 * \return Pointer to the shared telemetry (never NULL).
 */
const struct EphorusGeneralTelemetry *inverters_api_get_general_telemetry(void);

/*!
 * \brief Router entry point: decode and dispatch one received frame.
 *  
 * \details Matches the can_communication_receive_callback intent. Wire it up from
 *     can_communication_router_api_receive_inverter.
 *
 * \param[in] frame The frame popped off the RX queue.
 *
 * \retval CAN_COMMUNICATION_RC_OK on success.
 * \retval CAN_COMMUNICATION_RC_NULL_POINTER if \p frame is NULL.
 */
enum CanCommunicationReturnCode inverters_api_on_receive(const struct CanCommunicationFrame *frame);

#endif // INVERTERS_API_H
