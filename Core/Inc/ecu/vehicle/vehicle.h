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
#include "can-primary.h"

/*!
 * \brief Return and status codes for vehicle-scoped subsystem operations.
 */
enum VehicleReturnCode {
    VEHICLE_RC_OK,           /*!< Operation completed successfully. */
    VEHICLE_RC_ERROR,        /*!< A generic or internal system error occurred. */
    VEHICLE_RC_NULL_POINTER, /*!< A null pointer was passed to an API function. */
};

/*!
 * \struct VehicleHandler
 * \brief Flag container for evaluating core state transitions.
 * \details Stores transient flags parsed from incoming CAN messages.
 */
struct VehicleHandler {

    /*! 
     * \brief Edge-triggered activation command from the steering wheel.
     * \details Evaluates to true when the button is pressed on the steering wheel.
     */
    bool ts_on_button_pressed;

    /*! 
     * \brief Tractive system safety voltage threshold flag.
     * \details Evaluates to true if the DC link or Accumulator voltage exceeds 
     * the mandatory safety-critical 60V threshold. Used as a permissive block.
     */
    bool voltage_higher_than_60v;

    /*!
     * \brief Tractive system state to be requested to TSAC.
     * \details Set to true when the FSM requires the tractive system to be enabled.
     */
    bool ts_state_to_require;

    enum CanPrimaryTsacstatusMainboardstatus tsac_status; /*!< Current state of the HV accumulator as reported by TSAC. */
};

#endif
