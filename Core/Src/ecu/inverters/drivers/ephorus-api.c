/*!
 * \file ephorus-api.c
 * \date 2026-07-31
 * \authors Alessandro Bridi [ale.bridi15@gmail.com]
 *
 * \brief Implementation of the Ephorus 3.1 inverter driver.
 *
 * Pure logic: serialize/deserialize only. No HAL, no CAN transport.
 */

#include "ephorus-api.h"

#include "can-inverters-api.h"
#include "eagletrt-api.h"
#include "ephorus.h"

struct EphorusWheelCanIds {
    uint32_t tx_id;
    uint32_t outbound_a_id;
    uint32_t outbound_b_id;
};

EAGLETRT_STATIC const struct EphorusWheelCanIds ephorus_wheels_can_ids[EPHORUS_WHEEL_COUNT] = {
    [EPHORUS_WHEEL_FRONT_LEFT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER1SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER1OUTBOUNDA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER1OUTBOUNDB,
    },
    [EPHORUS_WHEEL_FRONT_RIGHT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER2SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER2OUTBOUNDA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER2OUTBOUNDB,
    },
    [EPHORUS_WHEEL_REAR_LEFT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER3SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER3OUTBOUNDA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER3OUTBOUNDB,
    },
    [EPHORUS_WHEEL_REAR_RIGHT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER4SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER4OUTBOUNDA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER4OUTBOUNDB,
    },
};

/*!
 * \brief true if the wheel sits on the "12" DC bus (front), else the "34" bus.
 *
 * \param wheel The wheel to check.
 *
 * \return true if the wheel is front-left or front-right, else false.
 */
EAGLETRT_STATIC bool ephorus_wheel_is_pair_12(enum EphorusWheel wheel) {
    return wheel <= EPHORUS_WHEEL_FRONT_RIGHT;
}

/*!
 * \brief Selects the setpoints / outbound frame ids for a wheel.
 *
 * \param wheel The wheel to query.
 * \param tx_id Pointer to receive the setpoints frame id.
 * \param outbound_a_id Pointer to receive the OutboundA frame id.
 * \param outbound_b_id Pointer to receive the OutboundB frame id.
 */
EAGLETRT_STATIC enum EphorusReturnCode ephorus_wheel_frame_ids(enum EphorusWheel wheel, uint32_t *tx_id, uint32_t *outbound_a_id, uint32_t *outbound_b_id) {
    if (wheel >= EPHORUS_WHEEL_COUNT || tx_id == NULL || outbound_a_id == NULL || outbound_b_id == NULL) {
        return EPHORUS_RC_NULL_POINTER;
    }

    *tx_id = ephorus_wheels_can_ids[wheel].tx_id;
    *outbound_a_id = ephorus_wheels_can_ids[wheel].outbound_a_id;
    *outbound_b_id = ephorus_wheels_can_ids[wheel].outbound_b_id;
    return EPHORUS_RC_OK;
}

/*!
 * \brief Returns the wheel if it exists and is attached, else NULL.
 *
 * \param handle The Ephorus driver handle.
 * \param wheel The wheel to query.
 *
 * \return Pointer to the wheel state if active, else NULL.
 */
EAGLETRT_STATIC struct EphorusWheelState *ephorus_active_wheel(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    if (handle == NULL || wheel >= EPHORUS_WHEEL_COUNT) {
        return NULL;
    }
    struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
    return wheel_state->active ? wheel_state : NULL;
}

/*!
 * \brief Maps a raw InverterState signal to the EphorusState enum.
 *
 * \param raw The raw InverterState value.
 * 
 * \return The corresponding EphorusState, or EPHORUS_STATE_ERROR if the raw value is invalid.
 */
EAGLETRT_STATIC enum EphorusState ephorus_state_from_raw(uint8_t raw) {
    switch (raw) {
        case EPHORUS_STATE_IDLE:
        case EPHORUS_STATE_DRIVE:
        case EPHORUS_STATE_ERROR:
        case EPHORUS_STATE_CONFIG_MISSING:
            return (enum EphorusState)raw;
        default:
            return EPHORUS_STATE_ERROR;
    }
}

/*!
 * \brief Extracts this wheel's per-inverter faults from a decoded error frame.
 *
 * \param general_error The decoded error frame.
 * \param wheel The wheel to query.
 *
 * \return Bitmask of EphorusWheelFault currently latched for this wheel.
 */
EAGLETRT_STATIC uint32_t ephorus_wheel_fault_bits(const struct CanInvertersGeneralerrorbits *general_error, enum EphorusWheel wheel) {
    if (general_error == NULL) {
        return 0;
    }

    /* Temporary container for the targeted wheel's error flags */
    struct InverterErrorFlags {
        bool control_error;
        bool timeout_comm;
        bool disable_under_load;
        bool position_sensor;
        bool motor_temperature;
        bool overtemperature;
        bool overspeed;
        bool overcurrent;
        bool short_circuit;
        bool sum_phase_currents;
        bool internal_fault;
    } err = { 0 };

    switch (wheel) {
        case EPHORUS_WHEEL_FRONT_LEFT:
            err.control_error = (general_error->err_inverter1_controlerror != 0);
            err.timeout_comm = (general_error->err_inverter1_timeout_comm != 0);
            err.disable_under_load = (general_error->err_inverter1_disable_under_load != 0);
            err.position_sensor = (general_error->err_inverter1_position_sensor != 0);
            err.motor_temperature = (general_error->err_inverter1_motortemperature != 0);
            err.overtemperature = (general_error->err_inverter1_overtemperature != 0);
            err.overspeed = (general_error->err_inverter1_overspeed != 0);
            err.overcurrent = (general_error->err_inverter1_overcurrent != 0);
            err.short_circuit = (general_error->err_inverter1_short_circuit != 0);
            err.sum_phase_currents = (general_error->err_inverter1_sum_phase_currents != 0);
            err.internal_fault = (general_error->err_inverter1_internal_fault != 0);
            break;

        case EPHORUS_WHEEL_FRONT_RIGHT:
            err.control_error = (general_error->err_inverter2_controlerror != 0);
            err.timeout_comm = (general_error->err_inverter2_timeout_comm != 0);
            err.disable_under_load = (general_error->err_inverter2_disable_under_load != 0);
            err.position_sensor = (general_error->err_inverter2_position_sensor != 0);
            err.motor_temperature = (general_error->err_inverter2_motortemperature != 0);
            err.overtemperature = (general_error->err_inverter2_overtemperature != 0);
            err.overspeed = (general_error->err_inverter2_overspeed != 0);
            err.overcurrent = (general_error->err_inverter2_overcurrent != 0);
            err.short_circuit = (general_error->err_inverter2_short_circuit != 0);
            err.sum_phase_currents = (general_error->err_inverter2_sum_phase_currents != 0);
            err.internal_fault = (general_error->err_inverter2_internal_fault != 0);
            break;

        case EPHORUS_WHEEL_REAR_LEFT:
            err.control_error = (general_error->err_inverter3_controlerror != 0);
            err.timeout_comm = (general_error->err_inverter3_timeout_comm != 0);
            err.disable_under_load = (general_error->err_inverter3_disable_under_load != 0);
            err.position_sensor = (general_error->err_inverter3_position_sensor != 0);
            err.motor_temperature = (general_error->err_inverter3_motortemperature != 0);
            err.overtemperature = (general_error->err_inverter3_overtemperature != 0);
            err.overspeed = (general_error->err_inverter3_overspeed != 0);
            err.overcurrent = (general_error->err_inverter3_overcurrent != 0);
            err.short_circuit = (general_error->err_inverter3_short_circuit != 0);
            err.sum_phase_currents = (general_error->err_inverter3_sum_phase_currents != 0);
            err.internal_fault = (general_error->err_inverter3_internal_fault != 0);
            break;

        case EPHORUS_WHEEL_REAR_RIGHT:
            err.control_error = (general_error->err_inverter4_controlerror != 0);
            err.timeout_comm = (general_error->err_inverter4_timeout_comm != 0);
            err.disable_under_load = (general_error->err_inverter4_disable_under_load != 0);
            err.position_sensor = (general_error->err_inverter4_position_sensor != 0);
            err.motor_temperature = (general_error->err_inverter4_motortemperature != 0);
            err.overtemperature = (general_error->err_inverter4_overtemperature != 0);
            err.overspeed = (general_error->err_inverter4_overspeed != 0);
            err.overcurrent = (general_error->err_inverter4_overcurrent != 0);
            err.short_circuit = (general_error->err_inverter4_short_circuit != 0);
            err.sum_phase_currents = (general_error->err_inverter4_sum_phase_currents != 0);
            err.internal_fault = (general_error->err_inverter4_internal_fault != 0);
            break;

        default:
            return 0;
    }

    uint32_t fault_bits = 0;
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_CONTROL_ERROR, err.control_error);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_TIMEOUT_COMM, err.timeout_comm);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_DISABLE_UNDER_LOAD, err.disable_under_load);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_POSITION_SENSOR, err.position_sensor);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_MOTOR_TEMPERATURE, err.motor_temperature);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_OVERTEMPERATURE, err.overtemperature);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_OVERSPEED, err.overspeed);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_OVERCURRENT, err.overcurrent);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_SHORT_CIRCUIT, err.short_circuit);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_SUM_PHASE_CURRENTS, err.sum_phase_currents);
    fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, EPHORUS_WHEEL_FAULT_INTERNAL_FAULT, err.internal_fault);

    return fault_bits;
}

/*!
 * \brief Extracts the shared faults from a decoded error frame.
 *
 * \param general_error The decoded error frame.
 *
 * \return Bitmask of EphorusGeneralFault currently latched.
 */
EAGLETRT_STATIC uint32_t ephorus_general_fault_bits(const struct CanInvertersGeneralerrorbits *general_error) {
    uint32_t bits = 0;
    if (general_error->err_controldisabled) {
        bits = EAGLETRT_API_BIT_SET(bits, EPHORUS_GENERAL_FAULT_CONTROL_DISABLED);
    }
    if (general_error->err_lv_supply) {
        bits = EAGLETRT_API_BIT_SET(bits, EPHORUS_GENERAL_FAULT_LV_SUPPLY);
    }
    if (general_error->err_inverter1_2_dc_undervoltage) {
        bits = EAGLETRT_API_BIT_SET(bits, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_12);
    }
    if (general_error->err_inverter1_2_dc_overvoltage) {
        bits = EAGLETRT_API_BIT_SET(bits, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_12);
    }
    if (general_error->err_inverter3_4_dc_undervoltage) {
        bits = EAGLETRT_API_BIT_SET(bits, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_34);
    }
    if (general_error->err_inverter3_4_dc_overvoltage) {
        bits = EAGLETRT_API_BIT_SET(bits, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_34);
    }
    return bits;
}

/*!
 * \brief true if any shared fault (global or this wheel's DC bus) is set.
 * 
 * \param general_bits The bitmask of shared faults.
 * \param wheel The wheel to check.
 *
 * \return true if any shared fault affects this wheel, else false.
 */
EAGLETRT_STATIC bool ephorus_general_faults_hit_wheel(uint32_t general_bits, enum EphorusWheel wheel) {
    uint32_t global = EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_CONTROL_DISABLED) | EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_LV_SUPPLY);
    uint32_t dc = ephorus_wheel_is_pair_12(wheel)
                      ? (EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_12) | EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_12))
                      : (EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_34) | EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_34));
    return (general_bits & (global | dc)) != 0;
}

/*!
 * \brief Updates a wheel's telemetry from an OutboundA frame.
 *
 * \param wheel The wheel to update.
 * \param outbound_a The decoded OutboundA frame.
 */
EAGLETRT_STATIC void ephorus_apply_outbound_a(struct EphorusWheelState *wheel, const struct CanInvertersInverter1outbounda *outbound_a) {
    wheel->tlm.state = ephorus_state_from_raw(outbound_a->inverterstate);
    wheel->tlm.ready = outbound_a->inverterready != 0;
    wheel->tlm.torque_nm = outbound_a->torqueactual;
    wheel->tlm.temp_motor_c = outbound_a->temperaturemotor;
    wheel->tlm.temp_switches_c = outbound_a->temperaturepowerswitches;
}

/*!
 * \brief Updates a wheel's telemetry from an OutboundB frame.
 *
 * \param wheel The wheel to update.
 * \param outbound_b The decoded OutboundB frame.
 */
EAGLETRT_STATIC void ephorus_apply_outbound_b(struct EphorusWheelState *wheel, const struct CanInvertersInverter1outboundb *outbound_b) {
    wheel->tlm.speed_rpm = outbound_b->speedactual;
}

/*!
 * \brief Updates the general telemetry from a GeneralOutbound frame.
 *
 * \param handle The Ephorus driver handle.
 * \param general_outbound The decoded GeneralOutbound frame.
 */
EAGLETRT_STATIC void ephorus_apply_general(struct EphorusHandler *handle, const struct CanInvertersGeneraloutbound *general_outbound) {
    handle->general.dclink_voltage_12_v = general_outbound->dclinkvoltage12actual;
    handle->general.dclink_voltage_34_v = general_outbound->dclinkvoltage34actual;
    handle->general.dclink_good_12 = general_outbound->dclinkgood12 != 0;
    handle->general.dclink_good_34 = general_outbound->dclinkgood34 != 0;
    handle->general.enable_mirror = general_outbound->mirrorcontrolenable != 0;
}

/*!
 * \brief Updates the general and per-wheel fault bits from a GeneralErrorBits frame.
 *
 * \param handle The Ephorus driver handle.
 * \param general_error The decoded GeneralErrorBits frame.
 */
EAGLETRT_STATIC void ephorus_apply_errors(struct EphorusHandler *handle, const struct CanInvertersGeneralerrorbits *general_error) {
    uint32_t general_bits = ephorus_general_fault_bits(general_error);
    handle->general.fault_bits = general_bits;

    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
        if (!wheel_state->active) {
            continue;
        }
        wheel_state->tlm.fault_bits = ephorus_wheel_fault_bits(general_error, wheel);
        bool faulted = wheel_state->tlm.fault_bits != 0 || ephorus_general_faults_hit_wheel(general_bits, wheel);
        /* Latch on the rising edge: a fresh fault inhibits motion until re-armed. */
        if (faulted && !wheel_state->faulted) {
            wheel_state->faulted = true;
            wheel_state->running = false;
        }
    }
}

enum EphorusReturnCode ephorus_api_init(struct EphorusHandler *handle) {
    if (handle == NULL) {
        return EPHORUS_RC_NULL_POINTER;
    }

    *handle = (struct EphorusHandler){ 0 };
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
        ephorus_wheel_frame_ids(wheel, &wheel_state->tx_id, &wheel_state->outbound_a_id, &wheel_state->outbound_b_id);
        wheel_state->torque_nm = 0.0F;
    }
    return EPHORUS_RC_OK;
}

enum EphorusReturnCode ephorus_api_attach(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    if (handle == NULL) {
        return EPHORUS_RC_NULL_POINTER;
    }
    if (wheel >= EPHORUS_WHEEL_COUNT) {
        return EPHORUS_RC_INVALID_WHEEL;
    }
    handle->wheels[wheel].active = true;
    return EPHORUS_RC_OK;
}

void ephorus_api_arm(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    struct EphorusWheelState *wheel_state = ephorus_active_wheel(handle, wheel);
    if (wheel_state == NULL) {
        return;
    }
    wheel_state->faulted = false;
    wheel_state->ack_pulse = true;   /* rising edge AckErr while still disabled */
    wheel_state->reset_pulse = true; /* request a latched-error reset */
    wheel_state->armed = true;
}

void ephorus_api_disarm(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    struct EphorusWheelState *wheel_state = ephorus_active_wheel(handle, wheel);
    if (wheel_state == NULL) {
        return;
    }
    wheel_state->running = false;
    wheel_state->armed = false;
}

void ephorus_api_set_run(struct EphorusHandler *handle, enum EphorusWheel wheel, bool run) {
    struct EphorusWheelState *wheel_state = ephorus_active_wheel(handle, wheel);
    if (wheel_state != NULL) {
        wheel_state->running = run;
    }
}

void ephorus_api_toggle_run(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    struct EphorusWheelState *wheel_state = ephorus_active_wheel(handle, wheel);
    if (wheel_state != NULL) {
        wheel_state->running = !wheel_state->running;
    }
}

void ephorus_api_set_torque(struct EphorusHandler *handle, enum EphorusWheel wheel, float torque_nm) {
    struct EphorusWheelState *wheel_state = ephorus_active_wheel(handle, wheel);
    if (wheel_state != NULL) {
        constexpr float max_torque = EPHORUS_MAX_TORQUE_NM;
        wheel_state->torque_nm = EAGLETRT_API_CLAMP(torque_nm, -max_torque, max_torque);
    }
}

enum EphorusReturnCode ephorus_api_build_setpoints(struct EphorusHandler *handle, enum EphorusWheel wheel, uint32_t *out_id, uint8_t data[EPHORUS_FRAME_DATA_SIZE]) {
    if (handle == NULL || out_id == NULL || data == NULL) {
        return EPHORUS_RC_NULL_POINTER;
    }
    if (wheel >= EPHORUS_WHEEL_COUNT) {
        return EPHORUS_RC_INVALID_WHEEL;
    }
    struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
    if (!wheel_state->active) {
        return EPHORUS_RC_INACTIVE;
    }

    const bool drive = wheel_state->armed && !wheel_state->faulted;
    const float torque = (wheel_state->running && drive) ? wheel_state->torque_nm : 0.0F;

    /* Torque-control-via-speed-rail: the sign of the request picks the torque
     * window and the speed rail the inverter chases. */
    float torque_limit_positive;
    float torque_limit_negative;
    int16_t speed_setpoint;
    if (torque > 0.0F) {
        /* Drive: allow up to +T, forbid braking, chase the high speed rail. */
        torque_limit_positive = torque;
        torque_limit_negative = 0.0F;
        speed_setpoint = (int16_t)EPHORUS_DRIVE_SPEED_RPM;
    } else if (torque < 0.0F) {
        /* Brake/regen: forbid driving, allow down to T, chase zero speed. */
        torque_limit_positive = 0.0F;
        torque_limit_negative = torque;
        speed_setpoint = 0;
    } else {
        /* Coast: no torque either way. */
        torque_limit_positive = 0.0F;
        torque_limit_negative = 0.0F;
        speed_setpoint = 0;
    }

    /* Consume the one-shot pulses. */
    const bool ack = wheel_state->ack_pulse;
    const bool reset = wheel_state->reset_pulse;
    wheel_state->ack_pulse = false;
    wheel_state->reset_pulse = false;

    /* All inverterNsetpoints share one layout in the union; fill the canonical
     * member and let serialize_from_id() encode it under this wheel's tx id. */
    union CanInvertersMessages msg = { 0 };
    msg.inverter1setpoints = (struct CanInvertersInverter1setpoints){
        .enableinverter = drive ? 1 : 0,
        .reseterror = reset ? 1 : 0,
        .ascallowed = 0,
        .currentcontrol = 0, /* never current mode over this network */
        .ackerr = ack ? 1 : 0,
        .speedsetpoint = speed_setpoint,
        .torquelimitpositive = torque_limit_positive,
        .torquelimitnegative = torque_limit_negative,
    };

    if (can_inverters_api_serialize_from_id((enum CanInvertersMessageFrameId)wheel_state->tx_id, &msg, data) < 0) {
        return EPHORUS_RC_SERIALIZE_ERROR;
    }

    *out_id = wheel_state->tx_id;
    return EPHORUS_RC_OK;
}

void ephorus_api_handle_frame(struct EphorusHandler *handle, uint32_t frame_id, const uint8_t data[EPHORUS_FRAME_DATA_SIZE]) {
    if (handle == NULL || data == NULL) {
        return;
    }
    if (!can_inverters_api_id_is_valid((enum CanInvertersMessageFrameId)frame_id)) {
        return;
    }

    union CanInvertersMessages msg = { 0 };
    if (can_inverters_api_deserialize_from_id((enum CanInvertersMessageFrameId)frame_id, (uint8_t *)data, &msg) != 0) {
        return;
    }

    if (frame_id == EPHORUS_RX_GENERAL) {
        ephorus_apply_general(handle, &msg.generaloutbound);
        return;
    }
    if (frame_id == EPHORUS_RX_ERRORS) {
        ephorus_apply_errors(handle, &msg.generalerrorbits);
        return;
    }

    /* Wheel-specific outbound frame: route to the matching active wheel. */
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
        if (!wheel_state->active) {
            continue;
        }
        if (frame_id == wheel_state->outbound_a_id) {
            ephorus_apply_outbound_a(wheel_state, &msg.inverter1outbounda);
            return;
        }
        if (frame_id == wheel_state->outbound_b_id) {
            ephorus_apply_outbound_b(wheel_state, &msg.inverter1outboundb);
            return;
        }
    }
}

const struct EphorusWheelTelemetry *ephorus_api_wheel_telemetry(const struct EphorusHandler *handle, enum EphorusWheel wheel) {
    if (handle == NULL || wheel >= EPHORUS_WHEEL_COUNT) {
        return NULL;
    }
    return &handle->wheels[wheel].tlm;
}

const struct EphorusGeneralTelemetry *ephorus_api_general_telemetry(const struct EphorusHandler *handle) {
    return handle == NULL ? NULL : &handle->general;
}

const char *ephorus_api_state_name(enum EphorusState state) {
    switch (state) {
        case EPHORUS_STATE_IDLE:
            return "Idle";
        case EPHORUS_STATE_DRIVE:
            return "Drive";
        case EPHORUS_STATE_ERROR:
            return "Error";
        case EPHORUS_STATE_CONFIG_MISSING:
            return "Config Missing";
        default:
            return "?";
    }
}

const char *ephorus_api_wheel_fault_name(uint32_t fault_bit) {
    switch ((enum EphorusWheelFault)fault_bit) {
        case EPHORUS_WHEEL_FAULT_TIMEOUT_COMM:
            return "Timeout_Comm";
        case EPHORUS_WHEEL_FAULT_DISABLE_UNDER_LOAD:
            return "Disable_Under_Load";
        case EPHORUS_WHEEL_FAULT_POSITION_SENSOR:
            return "Position_Sensor";
        case EPHORUS_WHEEL_FAULT_MOTOR_TEMPERATURE:
            return "Motortemperature";
        case EPHORUS_WHEEL_FAULT_OVERTEMPERATURE:
            return "Overtemperature";
        case EPHORUS_WHEEL_FAULT_OVERSPEED:
            return "Overspeed";
        case EPHORUS_WHEEL_FAULT_CONTROL_ERROR:
            return "ControlError";
        case EPHORUS_WHEEL_FAULT_OVERCURRENT:
            return "Overcurrent";
        case EPHORUS_WHEEL_FAULT_SHORT_CIRCUIT:
            return "Short_Circuit";
        case EPHORUS_WHEEL_FAULT_SUM_PHASE_CURRENTS:
            return "Sum_Phase_Currents";
        case EPHORUS_WHEEL_FAULT_INTERNAL_FAULT:
            return "Internal_Fault";
        default:
            return "?";
    }
}

const char *ephorus_api_general_fault_name(uint32_t fault_bit) {
    switch ((enum EphorusGeneralFault)fault_bit) {
        case EPHORUS_GENERAL_FAULT_CONTROL_DISABLED:
            return "ControlDisabled";
        case EPHORUS_GENERAL_FAULT_LV_SUPPLY:
            return "LV_Supply";
        case EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_12:
            return "DC12_Undervoltage";
        case EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_12:
            return "DC12_Overvoltage";
        case EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_34:
            return "DC34_Undervoltage";
        case EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_34:
            return "DC34_Overvoltage";
        default:
            return "?";
    }
}
