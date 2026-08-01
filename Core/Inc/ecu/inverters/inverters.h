/*!
 * \file inverters.h
 * \author Dorijan Di Zepp, Alessandro Bridi
 * \date 2026-07-31
 * \brief Generic inverters layer over the concrete Ephorus driver.
 *
 * \note The inverter-specific constants are derived from the "HV-Board_5" model
 * <a href="https://drive.google.com/file/d/1tRlrvqwPYmyLJJesLxHMAl7y99A7nsL0/view?usp=sharing">datasheet</a>.
 *
 * \note The motor constants defined are derived from the technical specifications of the Fischer TI085 series motors.
 * These motors are the manufacturer-recommended drive units for the HV-Board_5 inverter
 * system as specified in the "Recommended Motor" documentation.
 */

#ifndef INVERTERS_H
#define INVERTERS_H

#define _USE_MATH_DEFINES // NOLINT
#include <math.h>
#include <stdint.h>

#include "ephorus.h"

#define INVERTERS_HV_MAX_POWER_W (80000.0F) /*!< Maximum battery power allowed by Formula Student rules (80kW). */

#define INVERTERS_RPM_TO_RAD_COEFFICIENT ((2 * (float)M_PI) / 60.0F) /*!< Conversion factor: RPM to radians per second. */

#define INVERTERS_RPM_SPEED_THRESHOLD (0.1F) /*!< Avoid division by zero in case the RPM is equal/near to zero*/

#define INVERTERS_INVERTER_MAX_CONTINUOUS_CURRENT_A (74.0F) /*!< Maximum continuous phase current (Arms). */

#define INVERTERS_INVERTER_PEAK_CURRENT_A (90.0F) /*!< Absolute peak phase current (Arms). */

#define INVERTERS_MOTOR_PEAK_TORQUE_NM (29.1F) /*!< Maximum mechanical torque allowed per motor. */

#define INVERTERS_MOTOR_TORQUE_PER_CURRENT_NM_A (0.492F) /*!< Torque constant (Kt) in Nm/Arms. */

#define INVERTERS_MOTOR_MAX_MECHANICAL_POWER_W (35400.0F) /*!< Maximum mechanical power allowed per motor. */

#define INVERTERS_HV_MAX_REGEN_CURRENT_A (-40.0F) /*!< Maximum allowable regenerative current into the battery. */

#define INVERTERS_HV_MIN_CELL_VOLTAGE_V (2.8F) /*!< Minimum safe voltage for a single battery cell (V). */

#define INVERTERS_HV_CELL_COUNT (144) /*!< Total number of battery cells in series. */

#define INVERTERS_HV_MAX_REGEN_POWER_W (INVERTERS_HV_MAX_REGEN_CURRENT_A * INVERTERS_HV_MIN_CELL_VOLTAGE_V * INVERTERS_HV_CELL_COUNT) /*!< Maximum regenerative power allowed into the battery. */

#define INVERTERS_HV_MAX_CURRENT_A (130.0F) /*!< Maximum DC current allowed to be drawn from the battery. */

#define INVERTERS_HV_CELLS_PARALLEL_COUNT (3) /*!< Number of individual battery cells connected in parallel */

/*! \brief The single CAN network every inverter lives on. */
#define INVERTERS_NETWORK CAN_COMMUNICATION_NETWORK_INVERTER

/*!
 * \brief Return codes for the inverters module APIs.
 */
enum InvertersReturnCode {
    INVERTERS_RC_OK,            /*!< Operation completed successfully. */
    INVERTERS_RC_NULL_POINTER,  /*!< A null pointer was passed to a function. */
    INVERTERS_RC_INVALID_WHEEL, /*!< Wheel index out of range. */
    INVERTERS_RC_TX_ERROR,      /*!< Queueing a setpoint frame for transmission failed. */
};

/*!
 * \brief File-static state of the inverters module: the driver, the pending
 *     torque requests, the battery SoC used by the limits and the TX timing.
 */
struct InvertersHandler {
    struct EphorusHandler driver;                   /*!< Concrete inverter driver (held by value, typed). */
    float requested_torque_nm[EPHORUS_WHEEL_COUNT]; /*!< Latest raw torque request per wheel [Nm], before cut-off. */
    float hv_bms_soc;                               /*!< Latest State of Charge of the battery pack [0.0, 1.0]. */
    uint32_t last_tx_tick;                          /*!< Tick of the last setpoint broadcast. */
};

#endif // INVERTERS_H
