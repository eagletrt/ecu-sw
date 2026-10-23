/*!
 * \file vehicle.h
 * \author Dorijan Di Zepp
 * \brief Vehicle state transition flag context.
 * \details This header defines the lightweight telemetry and command flags 
 * used by the global FSM engine to manage transitions.
 */

#ifndef VEHICLE_H
#define VEHICLE_H

#include <stdbool.h>

/*!
 * \brief Return and status codes for vehicle-scoped subsystem operations.
 */
enum VehicleReturnCode {
    VEHICLE_RC_OK,           /*!< Operation completed successfully. */
    VEHICLE_RC_ERROR,        /*!< A generic or internal system error occurred. */
    VEHICLE_RC_NULL_POINTER, /*!< A null pointer was passed to an API function. */
};

/*!
 * \brief Callback used to drive the PTT output pin.
 * \param state true to assert the PTT line, false to release it.
 */
typedef enum VehicleReturnCode (*vehicle_ptt_control_callback)(bool state);

/*!
 * \struct VehicleHandler
 * \brief Flag container for evaluating core state transitions.
 * \details Stores transient flags parsed from incoming CAN messages.
 */
struct VehicleHandler {

    /*!
     * \brief Callback that drives the PTT output pin.
     * \details Mirrors the steering-wheel PTT button (received over CAN) onto a GPIO.
     */
    vehicle_ptt_control_callback ptt_control;

    /*!
     * \brief Edge-triggered activation command from the steering wheel.
     * \details Evaluates to true when the TSON button is pressed on the steering wheel,
     *     as reported over CAN (no longer read from a local pin).
     */
    bool ts_on_button_pressed;
};

#endif
