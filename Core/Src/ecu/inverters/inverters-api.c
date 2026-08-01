/*!
 * \file inverters-api.c
 * \author Dorijan Di Zepp, Alessandro Bridi
 * \date 2026-07-31
 * \brief Implementation of the inverters module.
 * \details Holds the concrete Ephorus driver by value and calls ephorus_api_*
 *     directly. On top of it, it keeps the vehicle "cut-off" safety layer that
 *     bounds the four torque requests (per-motor limits, voltage sag, 80 kW /
 *     battery-current / regen power limiting) before the driver serializes the
 *     setpoints. To use a different inverter, swap the EphorusHandler member in
 *     struct InvertersHandler and the ephorus_api_* calls below.
 */

#include "inverters-api.h"

#include "ephorus-api.h"
#include "can-communication-api.h"
#include "eagletrt-api.h"
#include <string.h>

/* The driver fills a fixed-size payload; it must match the transport's frame. */
static_assert(EPHORUS_FRAME_DATA_SIZE <= CAN_COMMUNICATION_FRAME_DATA_SIZE, "inverter frame payload size mismatch");

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
EAGLETRT_STATIC struct InvertersHandler inverters_handler;

/*! \brief Current (measured) speed of a wheel [RPM], sourced from decoded telemetry. */
EAGLETRT_STATIC float prv_inverters_wheel_rpm(enum EphorusWheel wheel) {
    return (float)inverters_handler.driver.wheels[wheel].tlm.speed_rpm;
}

/*!
 * \brief Calculates a global hardware reduction ratio to keep all motors within limits.
 *
 * \details This function implements a "ratio preservation" strategy. It compares each
 * requested torque against the motor's specific physical limit.
 * If any motor exceeds its limit, the function calculates a scaling factor (0.0 to 1.0).
 * By returning the smallest (most restrictive) ratio found across all four wheels,
 * the caller can scale the entire vehicle's torque request uniformly. This ensures
 * that the vehicle respects hardware constraints while maintaining the intended
 * torque-vectoring balance, preventing unpredictable handling.
 *
 * \param[in] torque_front_left   The raw torque requested for the front-left motor (Nm).
 * \param[in] torque_front_right  The raw torque requested for the front-right motor (Nm).
 * \param[in] torque_rear_left    The raw torque requested for the rear-left motor (Nm).
 * \param[in] torque_rear_right   The raw torque requested for the rear-right motor (Nm).
 * \param[in] limit_front_left    The maximum allowable torque for the front-left motor at current RPM (Nm).
 * \param[in] limit_front_right   The maximum allowable torque for the front-right motor at current RPM (Nm).
 * \param[in] limit_rear_left     The maximum allowable torque for the rear-left motor at current RPM (Nm).
 * \param[in] limit_rear_right    The maximum allowable torque for the rear-right motor at current RPM (Nm).
 *
 * \return float A scaling factor from 0.0 to 1.0. Returns 1.0 if no motors exceed their limits.
 */
EAGLETRT_STATIC float prv_inverters_get_motors_reduction(
    float torque_front_left, float torque_front_right, float torque_rear_left, float torque_rear_right, float limit_front_left, float limit_front_right, float limit_rear_left, float limit_rear_right) {
    float global_reduction = 1.0F;

    // Create arrays to iterate through the 4 wheels
    float requests[EPHORUS_WHEEL_COUNT] = { torque_front_left, torque_front_right, torque_rear_left, torque_rear_right };
    float limits[EPHORUS_WHEEL_COUNT] = { limit_front_left, limit_front_right, limit_rear_left, limit_rear_right };

    for (int i = 0; i < EPHORUS_WHEEL_COUNT; i++) {
        float absolute_request = fabsf(requests[i]);

        // If the request exceeds the limit, calculate the necessary reduction
        constexpr float epsilon = 0.001F; // Small threshold to avoid division by zero
        if (absolute_request > limits[i] && absolute_request > epsilon) {
            float local_ratio = limits[i] / absolute_request;
            // Keep the smallest ratio (the most restrictive cut)
            global_reduction = EAGLETRT_API_MIN(global_reduction, local_ratio);
        }
    }

    return global_reduction;
}

/*!
 * \brief Calculates the physical torque limit for an individual motor based on current RPM.
 *
 * \details Protects the powertrain by returning the most restrictive of three limits:
 * 1. Motor peak mechanical torque (physical saturation).
 * 2. Inverter continuous current limit (thermal/electrical protection).
 * 3. Mechanical power limit (governed by \ref INVERTERS_MOTOR_MAX_MECHANICAL_POWER_W).
 *
 * \note Applies a check on rpm value to prevent division by zero errors using the
 * threshold \ref INVERTERS_RPM_SPEED_THRESHOLD.
 *
 * \param[in] rpm The current rotational speed of the motor. Sign is ignored via fabsf.
 *
 * \return The maximum allowable absolute torque in Newton-meters (Nm), always positive.
 */
EAGLETRT_STATIC float prv_inverters_get_motor_torque_limit(const float rpm) {
    // Inverter current limit (Torque = Current * Kt)
    // Limits the phase current to protect inverter's hardware
    // using INVERTER_MAX_CONTINUOUS_CURRENT_A instead of INVERTER_PEAK_CURRENT_A
    // will provide less power but can operate for much longer times safely
    constexpr float current_max_current = INVERTERS_INVERTER_MAX_CONTINUOUS_CURRENT_A * INVERTERS_MOTOR_TORQUE_PER_CURRENT_NM_A;

    // Ensure rpm is not zero to avoid division by zero
    float absolute_rpm = fabsf(rpm);
    absolute_rpm = EAGLETRT_API_MAX(absolute_rpm, INVERTERS_RPM_SPEED_THRESHOLD);

    // Motor power limit (Torque = Power / Omega)
    const float current_max_torque = INVERTERS_MOTOR_MAX_MECHANICAL_POWER_W / (absolute_rpm * INVERTERS_RPM_TO_RAD_COEFFICIENT);

    // Return the lowest of the three.
    // This protects the motor and the inverter
    return EAGLETRT_API_MIN(INVERTERS_MOTOR_PEAK_TORQUE_NM, EAGLETRT_API_MIN(current_max_current, current_max_torque));
}

/*!
 * \brief Estimates the Open Circuit Voltage (VOC) of a single battery cell.
 *
 * \details Uses a 4th-order polynomial based on the State of Charge (SOC) to
 * predict the cell voltage when no load is applied. This estimation is critical
 * for calculating the available power headroom before hitting the voltage floor.
 *
 * \note This model is dependant on the cells' characteristics.
 *
 * \return float Estimated voltage per cell (V). Clamped between the model's 0% and 100% values.
 */
float prv_inverters_pack_voc_model(void) {
    float soc = inverters_handler.hv_bms_soc;
    //TODO: verify the values as they correspond to the characteristics of the old pack
    // which should be "recycled" for kraken.
    // It may be possible to retrieve directly the VOC value from the SOC's CAN frame, to be checked.
    constexpr float voc_poly_4_order = -3.85189120F;
    constexpr float voc_poly_3_order = 9.42278296F;
    constexpr float voc_poly_2_order = -8.31949326F;
    constexpr float voc_poly_1_order = 4.04805239F;
    constexpr float voc_poly_0_order = 2.82544823F;

    return voc_poly_4_order * powf(soc, 4) + voc_poly_3_order * powf(soc, 3) - voc_poly_2_order * powf(soc, 2) + voc_poly_1_order * soc + voc_poly_0_order;
}

/*!
 * \brief Estimates the DC internal resistance of a single battery cell.
 *
 * \details Provides the estimated resistance used to predict voltage sag
 * (V_sag = I_total * R_int). The model follows a linear relationship
 * where resistance slightly increases with State of Charge.
 *
 * \note This model is dependant on the cells' characteristics.
 *
 * \return float The estimated internal resistance of a single cell in Ohms (Ω).
 */
float prv_inverters_internal_resistance_model(void) {
    float soc = inverters_handler.hv_bms_soc;
    //TODO: verify the values as they correspond to the characteristics of the old pack
    // which should be "recycled" for kraken.
    // It may be possible to retrieve directly the resistance value from the SOC's CAN frame, to be checked.
    constexpr float resistance_poly_1_order = 0.0021F;
    constexpr float resistance_poly_0_order = 0.0141F;
    return resistance_poly_0_order + resistance_poly_1_order * soc;
}

/*!
 * \brief Limits total vehicle torque to respect battery power and current constraints.
 *
 * \details Calculates the instantaneous mechanical power and compares it against three limits:
 * 1. The provided 'power_max' (usually the 80kW regulatory limit or voltage sag limit).
 * 2. The physical DC current limit (Battery voltage * Max pack current).
 * 3. The hard-coded battery regeneration limit ( \ref INVERTERS_HV_MAX_REGEN_POWER_W ).
 * If any limit is exceeded, a uniform reduction ratio is applied to all wheels to
 * maintain the torque-vectoring balance while reducing total power consumption or absorption.
 *
 * \param[in] power_max The maximum allowable total power (Watts).
 * \param[in] angular_velocity_front_left Angular velocity (rad/s) of the front left wheel.
 * \param[in] angular_velocity_front_right Angular velocity (rad/s) of the front right wheel.
 * \param[in] angular_velocity_rear_left Angular velocity (rad/s) of the rear left wheel.
 * \param[in] angular_velocity_rear_right Angular velocity (rad/s) of the rear right wheel.
 * \param[in,out] torque_front_left Pointer to the front left torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the front right torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the rear left torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the rear right torque; to be scaled on low voltage.
 */
EAGLETRT_STATIC void prv_inverters_limit_torque_by_power(float power_max, float angular_velocity_front_left, float angular_velocity_front_right, float angular_velocity_rear_left, float angular_velocity_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {
    // Total mechanical power: P = Sum(T * w)
    float total_mechanical_power = (*torque_front_left * angular_velocity_front_left) + (*torque_front_right * angular_velocity_front_right) + (*torque_rear_left * angular_velocity_rear_left) + (*torque_rear_right * angular_velocity_rear_right);
    float reduction_ratio = 1.0F;

    // Physical DC current limit
    // Using the Voc model and cell count to find the real-time battery voltage
    float pack_voltage = prv_inverters_pack_voc_model() * INVERTERS_HV_CELL_COUNT;
    float physical_limit = pack_voltage * INVERTERS_HV_MAX_CURRENT_A;

    // Update power_max to the most restrictive limit
    // This ensures we respect both the 80kW rule and the 140A battery limit
    power_max = EAGLETRT_API_MIN(power_max, physical_limit);

    constexpr float low_power_threshold = 0.5F; // Watts, below which we consider the battery "dead"
    if (fabsf(power_max) < low_power_threshold) {
        reduction_ratio = 0.0F; // kill torque if battery is almost "dead"
    } else if (total_mechanical_power > power_max && total_mechanical_power >= 0.0F) {
        // discharge and power limit scaling
        reduction_ratio = EAGLETRT_API_MIN(EAGLETRT_API_MAX(power_max / total_mechanical_power, 0.0F), 1.0F);
    } else if (total_mechanical_power < INVERTERS_HV_MAX_REGEN_POWER_W && total_mechanical_power < 0.0F) {
        // regen scaling, avoid  "pushing" more than the cells can absorb
        reduction_ratio = EAGLETRT_API_MIN(EAGLETRT_API_MAX(INVERTERS_HV_MAX_REGEN_POWER_W / total_mechanical_power, 0.0F), 1.0F);
    }

    // Apply the same ratio to all motors
    *torque_front_left *= reduction_ratio;
    *torque_front_right *= reduction_ratio;
    *torque_rear_left *= reduction_ratio;
    *torque_rear_right *= reduction_ratio;
}

/*!
 * \brief High-level wrapper to enforce the regulatory 80kW power limit.
 *
 * \details Scales the requested torques if the total vehicle power exceeds the
 * maximum allowed by Formula Student rules ( \ref INVERTERS_HV_MAX_POWER_W ).
 * It converts motor speeds from RPM to angular velocity (rad/s) and delegates
 * the scaling math to \ref prv_inverters_limit_torque_by_power.
 *
 * \param[in] rpm_front_left Current speed of the front left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the front left torque request.
 * \param[in,out] torque_front_right Pointer to the front right torque request.
 * \param[in,out] torque_rear_left Pointer to the rear left torque request.
 * \param[in,out] torque_rear_right Pointer to the rear right torque request.
 */
EAGLETRT_STATIC void prv_inverters_maximum_allowable_power(float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {
    // Convert speeds to angular velocity (rad/s)
    float w_front_left = rpm_front_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_front_right = rpm_front_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_left = rpm_rear_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_right = rpm_rear_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    prv_inverters_limit_torque_by_power(INVERTERS_HV_MAX_POWER_W, w_front_left, w_front_right, w_rear_left, w_rear_right, torque_front_left, torque_front_right, torque_rear_left, torque_rear_right);
}

/*!
 * \brief Protects the battery from under-voltage by scaling torque during voltage sag.
 *
 * \details If the voltage approaches the absolute minimum threshold ( \ref INVERTERS_HV_MIN_CELL_VOLTAGE_V * \ref INVERTERS_HV_CELL_COUNT),
 * it should proportionally reduce torque across all motors to prevent errors.
 *
 * \param[in] rpm_front_left Current speed of the front left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the front left torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the front right torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the rear left torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the rear right torque; to be scaled on low voltage.
 */
EAGLETRT_STATIC void prv_inverters_minimum_cell_voltage_limit(float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {
    float w_front_left = rpm_front_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_front_right = rpm_front_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_left = rpm_rear_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_right = rpm_rear_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    float voc = prv_inverters_pack_voc_model();
    float d_v = EAGLETRT_API_MAX(voc - INVERTERS_HV_MIN_CELL_VOLTAGE_V, 0.0);
    float resistance = prv_inverters_internal_resistance_model();
    float i_max = d_v / resistance;
    i_max *= INVERTERS_HV_CELLS_PARALLEL_COUNT;

    float packV = voc * INVERTERS_HV_CELL_COUNT;
    float p_max = packV * i_max;
    prv_inverters_limit_torque_by_power(p_max, w_front_left, w_front_right, w_rear_left, w_rear_right, torque_front_left, torque_front_right, torque_rear_left, torque_rear_right);
}

/*!
 * \brief Orchestrates the safety cut-off pipeline for all four inverters.
 *
 * \details This function acts as the "overseer" of the powertrain. It processes raw torque
 * requests (which can be identical for all wheels or different if using for e.g. torque vectoring)
 * and ensures they stay within the "safe operating env" defined by the rules and hardware limits.
 *
 * The motor speeds used by the limits are sourced from the latest decoded inverter
 * telemetry ( \ref prv_inverters_wheel_rpm ) and the battery SoC from the handler.
 *
 * \param[in,out] torque_front_left_nm  Pointer to front left request. Modified if limits are hit.
 * \param[in,out] torque_front_right_nm Pointer to front right request. Modified if limits are hit.
 * \param[in,out] torque_rear_left_nm   Pointer to rear left request. Modified if limits are hit.
 * \param[in,out] torque_rear_right_nm  Pointer to rear right request. Modified if limits are hit.
 */
EAGLETRT_STATIC void prv_inverters_apply_cut_off(float *torque_front_left_nm, float *torque_front_right_nm, float *torque_rear_left_nm, float *torque_rear_right_nm) {
    // Load current rpm of all motors from the decoded telemetry
    float rpm_front_left = prv_inverters_wheel_rpm(EPHORUS_WHEEL_FRONT_LEFT);
    float rpm_front_right = prv_inverters_wheel_rpm(EPHORUS_WHEEL_FRONT_RIGHT);
    float rpm_rear_left = prv_inverters_wheel_rpm(EPHORUS_WHEEL_REAR_LEFT);
    float rpm_rear_right = prv_inverters_wheel_rpm(EPHORUS_WHEEL_REAR_RIGHT);

    // Individual hardware protection
    // Calculate the required reduction for each motor independently
    float limit_front_left = prv_inverters_get_motor_torque_limit(rpm_front_left);
    float limit_front_right = prv_inverters_get_motor_torque_limit(rpm_front_right);
    float limit_rear_left = prv_inverters_get_motor_torque_limit(rpm_rear_left);
    float limit_rear_right = prv_inverters_get_motor_torque_limit(rpm_rear_right);

    // Find the "most restricted" motor's ratio
    float motor_reduction = prv_inverters_get_motors_reduction(
        *torque_front_left_nm, *torque_front_right_nm, *torque_rear_left_nm, *torque_rear_right_nm, limit_front_left, limit_front_right, limit_rear_left, limit_rear_right);

    // Apply ratio to all motors
    // This preserves the balance of the car
    *torque_front_left_nm *= motor_reduction;
    *torque_front_right_nm *= motor_reduction;
    *torque_rear_left_nm *= motor_reduction;
    *torque_rear_right_nm *= motor_reduction;

    // Voltage sag protection
    // If the given throttle request will drop the voltage too much (risk of sag),
    // automatically cuts the torque
    prv_inverters_minimum_cell_voltage_limit(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);

    // Global battery protection and regulation check
    // This scales all motors proportionally to stay under 80kW and regen limits
    prv_inverters_maximum_allowable_power(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);
}

enum InvertersReturnCode inverters_api_init(void) {
    ephorus_api_init(&inverters_handler.driver);

    memset(inverters_handler.requested_torque_nm, 0, sizeof(inverters_handler.requested_torque_nm));
    inverters_handler.hv_bms_soc = 0.0F;
    inverters_handler.last_tx_tick = 0;

    // Four-wheel ECU: activate every wheel so it transmits and decodes.
    enum InvertersReturnCode return_code = INVERTERS_RC_OK;
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        if (ephorus_api_attach(&inverters_handler.driver, wheel) != EPHORUS_RC_OK) {
            return_code = INVERTERS_RC_INVALID_WHEEL;
        }
    }
    return return_code;
}

enum InvertersReturnCode inverters_api_attach(enum EphorusWheel wheel) {
    if (ephorus_api_attach(&inverters_handler.driver, wheel) != EPHORUS_RC_OK) {
        return INVERTERS_RC_INVALID_WHEEL;
    }
    return INVERTERS_RC_OK;
}

void inverters_api_arm(enum EphorusWheel wheel) {
    ephorus_api_arm(&inverters_handler.driver, wheel);
}

void inverters_api_disarm(enum EphorusWheel wheel) {
    ephorus_api_disarm(&inverters_handler.driver, wheel);
}

void inverters_api_set_run(enum EphorusWheel wheel, bool run) {
    ephorus_api_set_run(&inverters_handler.driver, wheel, run);
}

void inverters_api_toggle_run(enum EphorusWheel wheel) {
    ephorus_api_toggle_run(&inverters_handler.driver, wheel);
}

void inverters_api_set_torque(enum EphorusWheel wheel, float torque_nm) {
    if (wheel >= EPHORUS_WHEEL_COUNT) {
        return;
    }
    inverters_handler.requested_torque_nm[wheel] = torque_nm;
}

void inverters_api_set_soc(float hv_bms_soc) {
    inverters_handler.hv_bms_soc = EAGLETRT_API_CLAMP(hv_bms_soc, 0.0F, 1.0F);
}

enum InvertersReturnCode inverters_api_step(uint32_t tick) {
    if ((tick - inverters_handler.last_tx_tick) < EPHORUS_TX_PERIOD_MS) {
        return INVERTERS_RC_OK;
    }
    inverters_handler.last_tx_tick = tick;

    // Apply the cut-off safety layer to the pending requests before serialization,
    // so the requested torque never damages the battery pack nor exceeds the rules.
    float torque_front_left_nm = inverters_handler.requested_torque_nm[EPHORUS_WHEEL_FRONT_LEFT];
    float torque_front_right_nm = inverters_handler.requested_torque_nm[EPHORUS_WHEEL_FRONT_RIGHT];
    float torque_rear_left_nm = inverters_handler.requested_torque_nm[EPHORUS_WHEEL_REAR_LEFT];
    float torque_rear_right_nm = inverters_handler.requested_torque_nm[EPHORUS_WHEEL_REAR_RIGHT];

    prv_inverters_apply_cut_off(&torque_front_left_nm, &torque_front_right_nm, &torque_rear_left_nm, &torque_rear_right_nm);

    ephorus_api_set_torque(&inverters_handler.driver, EPHORUS_WHEEL_FRONT_LEFT, torque_front_left_nm);
    ephorus_api_set_torque(&inverters_handler.driver, EPHORUS_WHEEL_FRONT_RIGHT, torque_front_right_nm);
    ephorus_api_set_torque(&inverters_handler.driver, EPHORUS_WHEEL_REAR_LEFT, torque_rear_left_nm);
    ephorus_api_set_torque(&inverters_handler.driver, EPHORUS_WHEEL_REAR_RIGHT, torque_rear_right_nm);

    enum InvertersReturnCode return_code = INVERTERS_RC_OK;
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct CanCommunicationFrame frame = { 0 };
        uint32_t id = 0;
        enum EphorusReturnCode build = ephorus_api_build_setpoints(&inverters_handler.driver, wheel, &id, frame.data);
        if (build == EPHORUS_RC_INACTIVE) {
            continue; // wheel not attached
        }
        if (build != EPHORUS_RC_OK) {
            return_code = INVERTERS_RC_TX_ERROR;
            continue;
        }
        frame.id = id;
        frame.length = EPHORUS_FRAME_DATA_SIZE;
        if (can_communication_api_add_to_tx(INVERTERS_NETWORK, &frame) != CAN_COMMUNICATION_RC_OK) {
            return_code = INVERTERS_RC_TX_ERROR;
        }
    }
    return return_code;
}

const struct EphorusWheelTelemetry *inverters_api_wheel_telemetry(enum EphorusWheel wheel) {
    return ephorus_api_wheel_telemetry(&inverters_handler.driver, wheel);
}

const struct EphorusGeneralTelemetry *inverters_api_general_telemetry(void) {
    return ephorus_api_general_telemetry(&inverters_handler.driver);
}

const char *inverters_api_state_name(enum EphorusState state) {
    return ephorus_api_state_name(state);
}

const char *inverters_api_wheel_fault_name(int fault_bit) {
    return ephorus_api_wheel_fault_name(fault_bit);
}

const char *inverters_api_general_fault_name(int fault_bit) {
    return ephorus_api_general_fault_name(fault_bit);
}

enum CanCommunicationReturnCode inverters_api_on_receive(const struct CanCommunicationFrame *frame) {
    if (frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }
    ephorus_api_handle_frame(&inverters_handler.driver, frame->id, frame->data);
    return CAN_COMMUNICATION_RC_OK;
}
