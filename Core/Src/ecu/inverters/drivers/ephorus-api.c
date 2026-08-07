/*!
 * \file ephorus-api.c
 * \date 2026-07-31
 * \authors Alessandro Bridi [ale.bridi15@gmail.com]
 *
 * \brief Implementation of the Ephorus 3.1 inverter driver.
 *
 * Pure logic: serialize/deserialize only. No CAN transport.
 */

#include "ephorus-api.h"

#include "can-inverters-api.h"
#include "eagletrt-api.h"

EAGLETRT_STATIC const struct EphorusWheelCanIds ephorus_wheels_can_ids[EPHORUS_WHEEL_COUNT] = {
    [EPHORUS_WHEEL_FRONT_LEFT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER1SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER1TELEMETRYA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER1TELEMETRYB,
    },
    [EPHORUS_WHEEL_FRONT_RIGHT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYB,
    },
    [EPHORUS_WHEEL_REAR_LEFT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER3SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER3TELEMETRYA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER3TELEMETRYB,
    },
    [EPHORUS_WHEEL_REAR_RIGHT] = {
        .tx_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER4SETPOINTS,
        .outbound_a_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER4TELEMETRYA,
        .outbound_b_id = CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER4TELEMETRYB,
    },
};

/*!
 * \brief true if the wheel sits on the "12" DC bus (front), else the "34" bus.
 *
 * \param[in] wheel The wheel to check.
 *
 * \return true if the wheel is front-left or front-right, else false.
 */
EAGLETRT_STATIC bool prv_ephorus_wheel_is_pair_12(enum EphorusWheel wheel) {
    return (wheel == EPHORUS_WHEEL_FRONT_RIGHT) || (wheel == EPHORUS_WHEEL_FRONT_LEFT);
}

/*!
 * \brief Selects the setpoints / outbound frame ids for a wheel.
 *
 * \param wheel[in] The wheel to query.
 * \param ephorus_ids[out] The wheel's frame IDs.
 */
EAGLETRT_STATIC enum EphorusReturnCode prv_ephorus_wheel_get_frame_ids(enum EphorusWheel wheel, struct EphorusWheelCanIds *ephorus_ids) {
    if (wheel >= EPHORUS_WHEEL_COUNT || ephorus_ids == NULL) {
        return EPHORUS_RC_NULL_POINTER;
    }

    *ephorus_ids = ephorus_wheels_can_ids[wheel];
    return EPHORUS_RC_OK;
}

/*!
 * \brief Returns the wheel if it exists and is attached, else NULL.
 *
 * \param handle[in] The Ephorus driver handle.
 * \param wheel[in] The wheel to query.
 *
 * \return Pointer to the wheel state if active, else NULL.
 */
EAGLETRT_STATIC struct EphorusWheelState *prv_ephorus_get_active_wheel_state(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    if (handle == NULL || wheel >= EPHORUS_WHEEL_COUNT) {
        return NULL;
    }
    struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
    return wheel_state->enabled ? wheel_state : NULL;
}

/*!
 * \brief Maps a raw InverterState signal to the EphorusState enum.
 *
 * \param raw[in] The raw InverterState value.
 * 
 * \return The corresponding EphorusState, or EPHORUS_STATE_ERROR if the raw value is invalid.
 */
EAGLETRT_STATIC enum EphorusState prv_ephorus_get_state_from_raw(uint8_t raw) {
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
 * \brief Fills a wheel's per-inverter faults from a decoded error frame.
 *
 * \param general_error[in] The decoded error frame.
 * \param errors[out] Array of bools to fill with the wheel's faults.
 */
EAGLETRT_STATIC void prv_ephorus_fill_inv1(const struct CanInvertersEphoruserror *general_error, bool errors[EPHORUS_WHEEL_FAULT_COUNT]) {
    errors[EPHORUS_WHEEL_FAULT_CONTROL_ERROR] = (general_error->inverter1controlerror != 0);
    errors[EPHORUS_WHEEL_FAULT_TIMEOUT_COMM] = (general_error->inverter1timeoutcomm != 0);
    errors[EPHORUS_WHEEL_FAULT_DISABLE_UNDER_LOAD] = (general_error->inverter1disableunderload != 0);
    errors[EPHORUS_WHEEL_FAULT_POSITION_SENSOR] = (general_error->inverter1positionsensor != 0);
    errors[EPHORUS_WHEEL_FAULT_MOTOR_TEMPERATURE] = (general_error->inverter1motortemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERTEMPERATURE] = (general_error->inverter1overtemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERSPEED] = (general_error->inverter1overspeed != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERCURRENT] = (general_error->inverter1overcurrent != 0);
    errors[EPHORUS_WHEEL_FAULT_SHORT_CIRCUIT] = (general_error->inverter1shortcircuit != 0);
    errors[EPHORUS_WHEEL_FAULT_SUM_PHASE_CURRENTS] = (general_error->inverter1sumphasecurrents != 0);
    errors[EPHORUS_WHEEL_FAULT_INTERNAL_FAULT] = (general_error->inverter1internalfault != 0);
}

/*!
 * \brief Fills a wheel's per-inverter faults from a decoded error frame.
 *
 * \param general_error[in] The decoded error frame.
 * \param errors[out] Array of bools to fill with the wheel's faults.
 */
EAGLETRT_STATIC void prv_ephorus_fill_inv2(const struct CanInvertersEphoruserror *general_error, bool errors[EPHORUS_WHEEL_FAULT_COUNT]) {
    errors[EPHORUS_WHEEL_FAULT_CONTROL_ERROR] = (general_error->inverter2controlerror != 0);
    errors[EPHORUS_WHEEL_FAULT_TIMEOUT_COMM] = (general_error->inverter2timeoutcomm != 0);
    errors[EPHORUS_WHEEL_FAULT_DISABLE_UNDER_LOAD] = (general_error->inverter2disableunderload != 0);
    errors[EPHORUS_WHEEL_FAULT_POSITION_SENSOR] = (general_error->inverter2positionsensor != 0);
    errors[EPHORUS_WHEEL_FAULT_MOTOR_TEMPERATURE] = (general_error->inverter2motortemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERTEMPERATURE] = (general_error->inverter2overtemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERSPEED] = (general_error->inverter2overspeed != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERCURRENT] = (general_error->inverter2overcurrent != 0);
    errors[EPHORUS_WHEEL_FAULT_SHORT_CIRCUIT] = (general_error->inverter2shortcircuit != 0);
    errors[EPHORUS_WHEEL_FAULT_SUM_PHASE_CURRENTS] = (general_error->inverter2sumphasecurrents != 0);
    errors[EPHORUS_WHEEL_FAULT_INTERNAL_FAULT] = (general_error->inverter2internalfault != 0);
}

/*!
 * \brief Fills a wheel's per-inverter faults from a decoded error frame.
 *
 * \param general_error[in] The decoded error frame.
 * \param errors[out] Array of bools to fill with the wheel's faults.
 */
EAGLETRT_STATIC void prv_ephorus_fill_inv3(const struct CanInvertersEphoruserror *general_error, bool errors[EPHORUS_WHEEL_FAULT_COUNT]) {
    errors[EPHORUS_WHEEL_FAULT_CONTROL_ERROR] = (general_error->inverter3controlerror != 0);
    errors[EPHORUS_WHEEL_FAULT_TIMEOUT_COMM] = (general_error->inverter3timeoutcomm != 0);
    errors[EPHORUS_WHEEL_FAULT_DISABLE_UNDER_LOAD] = (general_error->inverter3disableunderload != 0);
    errors[EPHORUS_WHEEL_FAULT_POSITION_SENSOR] = (general_error->inverter3positionsensor != 0);
    errors[EPHORUS_WHEEL_FAULT_MOTOR_TEMPERATURE] = (general_error->inverter3motortemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERTEMPERATURE] = (general_error->inverter3overtemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERSPEED] = (general_error->inverter3overspeed != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERCURRENT] = (general_error->inverter3overcurrent != 0);
    errors[EPHORUS_WHEEL_FAULT_SHORT_CIRCUIT] = (general_error->inverter3shortcircuit != 0);
    errors[EPHORUS_WHEEL_FAULT_SUM_PHASE_CURRENTS] = (general_error->inverter3sumphasecurrents != 0);
    errors[EPHORUS_WHEEL_FAULT_INTERNAL_FAULT] = (general_error->inverter3internalfault != 0);
}

/*!
 * \brief Fills a wheel's per-inverter faults from a decoded error frame.
 *
 * \param general_error[in] The decoded error frame.
 * \param errors[out] Array of bools to fill with the wheel's faults.
 */
EAGLETRT_STATIC void prv_ephorus_fill_inv4(const struct CanInvertersEphoruserror *general_error, bool errors[EPHORUS_WHEEL_FAULT_COUNT]) {
    errors[EPHORUS_WHEEL_FAULT_CONTROL_ERROR] = (general_error->inverter4controlerror != 0);
    errors[EPHORUS_WHEEL_FAULT_TIMEOUT_COMM] = (general_error->inverter4timeoutcomm != 0);
    errors[EPHORUS_WHEEL_FAULT_DISABLE_UNDER_LOAD] = (general_error->inverter4disableunderload != 0);
    errors[EPHORUS_WHEEL_FAULT_POSITION_SENSOR] = (general_error->inverter4positionsensor != 0);
    errors[EPHORUS_WHEEL_FAULT_MOTOR_TEMPERATURE] = (general_error->inverter4motortemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERTEMPERATURE] = (general_error->inverter4overtemperature != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERSPEED] = (general_error->inverter4overspeed != 0);
    errors[EPHORUS_WHEEL_FAULT_OVERCURRENT] = (general_error->inverter4overcurrent != 0);
    errors[EPHORUS_WHEEL_FAULT_SHORT_CIRCUIT] = (general_error->inverter4shortcircuit != 0);
    errors[EPHORUS_WHEEL_FAULT_SUM_PHASE_CURRENTS] = (general_error->inverter4sumphasecurrents != 0);
    errors[EPHORUS_WHEEL_FAULT_INTERNAL_FAULT] = (general_error->inverter4internalfault != 0);
}

typedef void (*ephorus_fill_fault_function)(const struct CanInvertersEphoruserror *, bool[EPHORUS_WHEEL_FAULT_COUNT]);

/*!
 * \brief Extracts this wheel's per-inverter faults from a decoded error frame.
 *
 * \param general_error[in] The decoded error frame.
 * \param wheel[in] The wheel to query.
 *
 * \return Bitmask of EphorusWheelFault currently latched for this wheel.
 */
EAGLETRT_STATIC uint32_t prv_ephorus_get_wheel_fault_bits(const struct CanInvertersEphoruserror *general_error, enum EphorusWheel wheel) {
    if (general_error == NULL) {
        return 0;
    }

    EAGLETRT_STATIC ephorus_fill_fault_function fillers[EPHORUS_WHEEL_COUNT] = {
        [EPHORUS_WHEEL_FRONT_LEFT] = prv_ephorus_fill_inv1,
        [EPHORUS_WHEEL_FRONT_RIGHT] = prv_ephorus_fill_inv2,
        [EPHORUS_WHEEL_REAR_LEFT] = prv_ephorus_fill_inv3,
        [EPHORUS_WHEEL_REAR_RIGHT] = prv_ephorus_fill_inv4,
    };

    bool errors[EPHORUS_WHEEL_FAULT_COUNT] = { 0 };
    fillers[wheel](general_error, errors);

    uint32_t fault_bits = 0;
    for (enum EphorusWheelFault fault = 0; fault < EPHORUS_WHEEL_FAULT_COUNT; fault++) {
        fault_bits = EAGLETRT_API_BIT_SET_IF(fault_bits, fault, errors[fault]);
    }

    return fault_bits;
}

/*!
 * \brief Extracts the shared faults from a decoded error frame.
 *
 * \param general_error[in] The decoded error frame.
 *
 * \return Bitmask of EphorusGeneralFault currently latched.
 */
EAGLETRT_STATIC uint32_t prv_ephorus_get_general_fault_bits(const struct CanInvertersEphoruserror *general_error) {
    uint32_t bits = 0;
    bits = EAGLETRT_API_BIT_SET_IF(bits, EPHORUS_GENERAL_FAULT_CONTROL_DISABLED, general_error->controldisabled != 0);
    bits = EAGLETRT_API_BIT_SET_IF(bits, EPHORUS_GENERAL_FAULT_LV_SUPPLY, general_error->lvsupply != 0);
    bits = EAGLETRT_API_BIT_SET_IF(bits, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_12, general_error->inverter12dcundervoltage != 0);
    bits = EAGLETRT_API_BIT_SET_IF(bits, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_12, general_error->inverter12dcovervoltage != 0);
    bits = EAGLETRT_API_BIT_SET_IF(bits, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_34, general_error->inverter34dcundervoltage != 0);
    bits = EAGLETRT_API_BIT_SET_IF(bits, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_34, general_error->inverter34dcovervoltage != 0);
    return bits;
}

/*!
 * \brief Checks if a general error is affecting the given wheel.
 * 
 * \param general_bits[in] The bitmask of shared faults.
 * \param wheel[in] The wheel to check.
 *
 * \return true if any shared fault affects this wheel, else false.
 */
EAGLETRT_STATIC bool prv_ephorus_has_wheel_general_faults(uint32_t general_bits, enum EphorusWheel wheel) {
    uint32_t global = EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_CONTROL_DISABLED) | EAGLETRT_API_BIT_SET(0, EPHORUS_GENERAL_FAULT_LV_SUPPLY);
    uint32_t dc = 0;
    bool is_pair_12 = prv_ephorus_wheel_is_pair_12(wheel);
    dc = EAGLETRT_API_BIT_SET_IF(dc, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_12, is_pair_12);
    dc = EAGLETRT_API_BIT_SET_IF(dc, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_12, is_pair_12);
    dc = EAGLETRT_API_BIT_SET_IF(dc, EPHORUS_GENERAL_FAULT_DC_UNDERVOLTAGE_34, !is_pair_12);
    dc = EAGLETRT_API_BIT_SET_IF(dc, EPHORUS_GENERAL_FAULT_DC_OVERVOLTAGE_34, !is_pair_12);
    return (general_bits & (global | dc)) != 0;
}

/*!
 * \brief Updates a wheel's telemetry from an OutboundA frame.
 *
 * \param wheel[out] The wheel to update.
 * \param outbound_a[in] The decoded OutboundA frame.
 */
EAGLETRT_STATIC void prv_ephorus_apply_outbound_a(struct EphorusWheelState *wheel, const struct CanInvertersEphorusinverter1telemetrya *outbound_a) {
    wheel->tlm.state = prv_ephorus_get_state_from_raw(outbound_a->inverterstate);
    wheel->tlm.ready = outbound_a->inverterready != 0;
    wheel->tlm.torque_nm = outbound_a->torqueactual;
    wheel->tlm.temp_motor_c = outbound_a->temperaturemotor;
    wheel->tlm.temp_switches_c = outbound_a->temperaturepowerswitches;
}

/*!
 * \brief Updates a wheel's telemetry from an OutboundB frame.
 *
 * \param wheel[out] The wheel to update.
 * \param outbound_b[in] The decoded OutboundB frame.
 */
EAGLETRT_STATIC void prv_ephorus_apply_outbound_b(struct EphorusWheelState *wheel, const struct CanInvertersEphorusinverter1telemetryb *outbound_b) {
    wheel->tlm.speed_rpm = outbound_b->speedactual;
}

/*!
 * \brief Updates the general telemetry from a GeneralOutbound frame.
 *
 * \param handle[out] The Ephorus driver handle.
 * \param general_outbound[in] The decoded GeneralOutbound frame.
 */
EAGLETRT_STATIC void prv_ephorus_apply_general(struct EphorusHandler *handle, const struct CanInvertersEphorustelemetry *general_outbound) {
    handle->general.dclink_voltage_12_v = general_outbound->dclinkvoltage12actual;
    handle->general.dclink_voltage_34_v = general_outbound->dclinkvoltage34actual;
    handle->general.dclink_good_12 = general_outbound->dclinkgood12 != 0;
    handle->general.dclink_good_34 = general_outbound->dclinkgood34 != 0;
    handle->general.enable_mirror = general_outbound->mirrorcontrolenable != 0;
}

/*!
 * \brief Updates the general and per-wheel fault bits from a GeneralErrorBits frame.
 *
 * \param handle[out] The Ephorus driver handle.
 * \param general_error[in] The decoded GeneralErrorBits frame.
 */
EAGLETRT_STATIC void prv_ephorus_apply_errors(struct EphorusHandler *handle, const struct CanInvertersEphoruserror *general_error) {
    uint32_t general_bits = prv_ephorus_get_general_fault_bits(general_error);
    handle->general.fault_bits = general_bits;

    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
        if (!wheel_state->enabled) {
            continue;
        }
        wheel_state->tlm.fault_bits = prv_ephorus_get_wheel_fault_bits(general_error, wheel);
        bool faulted = wheel_state->tlm.fault_bits != 0 || prv_ephorus_has_wheel_general_faults(general_bits, wheel);
        /* Latch on the rising edge: a fresh fault inhibits motion until re-armed. */
        if (faulted && (wheel_state->inverter_state != EPHORUS_INVERTER_STATE_FAULT)) {
            wheel_state->inverter_state = EPHORUS_INVERTER_STATE_FAULT;
        }
    }
}

enum EphorusReturnCode ephorus_api_init(struct EphorusHandler *handle) {
    if (handle == NULL) {
        return EPHORUS_RC_NULL_POINTER;
    }

    memset(handle, 0, sizeof(*handle));
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
        prv_ephorus_wheel_get_frame_ids(wheel, &wheel_state->can_ids);
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
    handle->wheels[wheel].enabled = true;
    return EPHORUS_RC_OK;
}

void ephorus_api_arm(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    struct EphorusWheelState *wheel_state = prv_ephorus_get_active_wheel_state(handle, wheel);
    if (wheel_state == NULL) {
        return;
    }
    wheel_state->ack_pulse = true;   /* rising edge AckErr while still disabled */
    wheel_state->reset_pulse = true; /* request a latched-error reset */
    wheel_state->inverter_state = EPHORUS_INVERTER_STATE_ARMED;
}

void ephorus_api_disarm(struct EphorusHandler *handle, enum EphorusWheel wheel) {
    struct EphorusWheelState *wheel_state = prv_ephorus_get_active_wheel_state(handle, wheel);
    if (wheel_state == NULL) {
        return;
    }
    wheel_state->inverter_state = EPHORUS_INVERTER_STATE_DISARMED;
}

void ephorus_api_set_torque(struct EphorusHandler *handle, enum EphorusWheel wheel, float torque_nm) {
    struct EphorusWheelState *wheel_state = prv_ephorus_get_active_wheel_state(handle, wheel);
    if (wheel_state != NULL) {
        constexpr float max_torque = EPHORUS_MAX_TORQUE_NM;
        wheel_state->torque_nm = EAGLETRT_API_CLAMP(torque_nm, -max_torque, max_torque);
    }
}

bool ephorus_api_is_all_in_drive(struct EphorusHandler *handle) {
    if (handle == NULL) {
        return false;
    }
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
        if (!wheel_state->enabled) {
            continue;
        }
        if (wheel_state->inverter_state != EPHORUS_INVERTER_STATE_ARMED) {
            return false;
        }
        if (wheel_state->tlm.state != EPHORUS_STATE_DRIVE || !(wheel_state->tlm.ready) || wheel_state->tlm.fault_bits != 0 || prv_ephorus_has_wheel_general_faults(handle->general.fault_bits, wheel)) {
            return false;
        }
    }
    return true;
}

enum EphorusReturnCode ephorus_api_build_setpoints(struct EphorusHandler *handle, enum EphorusWheel wheel, uint32_t *out_id, uint8_t data[EPHORUS_FRAME_DATA_SIZE]) {
    if (handle == NULL || out_id == NULL || data == NULL) {
        return EPHORUS_RC_NULL_POINTER;
    }
    if (wheel >= EPHORUS_WHEEL_COUNT) {
        return EPHORUS_RC_INVALID_WHEEL;
    }
    struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
    if (!wheel_state->enabled) {
        return EPHORUS_RC_INACTIVE;
    }

    const bool drive = wheel_state->inverter_state == EPHORUS_INVERTER_STATE_ARMED;
    const float torque = drive ? wheel_state->torque_nm : 0.0F;

    /* Torque-control-via-speed-rail: the sign of the request picks the torque
     * window and the speed rail the inverter chases. */
    float torque_limit_positive;
    float torque_limit_negative;
    int16_t speed_setpoint;
    if (torque >= 0.0F) {
        /* Drive: allow up to +T, forbid braking, chase the high speed rail. */
        torque_limit_positive = torque;
        torque_limit_negative = 0.0F;
        speed_setpoint = (int16_t)EPHORUS_DRIVE_SPEED_RPM;
    } else {
        /* Brake/regen: forbid driving, allow down to T, chase zero speed. */
        torque_limit_positive = 0.0F;
        torque_limit_negative = torque;
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
    msg.ephorusinverter1setpoints = (struct CanInvertersEphorusinverter1setpoints){
        .enableinverter = drive,
        .reseterror = reset,
        .ascallowed = 1,     /* hardcoded as it should be always used with fisher motors */
        .currentcontrol = 0, /* never current mode over this network */
        .ackerr = ack,
        .speedsetpoint = speed_setpoint,
        .torquelimitpositive = torque_limit_positive,
        .torquelimitnegative = torque_limit_negative,
    };

    uint32_t tx_id = wheel_state->can_ids.tx_id;
    if (can_inverters_api_serialize_from_id((enum CanInvertersMessageFrameId)tx_id, &msg, data) < 0) {
        return EPHORUS_RC_SERIALIZE_ERROR;
    }

    *out_id = tx_id;
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
        prv_ephorus_apply_general(handle, &msg.ephorustelemetry);
        return;
    }
    if (frame_id == EPHORUS_RX_ERRORS) {
        prv_ephorus_apply_errors(handle, &msg.ephoruserror);
        return;
    }

    /* Wheel-specific outbound frame: route to the matching active wheel. */
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        struct EphorusWheelState *wheel_state = &handle->wheels[wheel];
        if (!wheel_state->enabled) {
            continue;
        }
        if (frame_id == wheel_state->can_ids.outbound_a_id) {
            prv_ephorus_apply_outbound_a(wheel_state, &msg.ephorusinverter1telemetrya);
            return;
        }
        if (frame_id == wheel_state->can_ids.outbound_b_id) {
            prv_ephorus_apply_outbound_b(wheel_state, &msg.ephorusinverter1telemetryb);
            return;
        }
    }
}

const struct EphorusWheelTelemetry *ephorus_api_get_wheel_telemetry(const struct EphorusHandler *handle, enum EphorusWheel wheel) {
    if (handle == NULL || wheel >= EPHORUS_WHEEL_COUNT) {
        return NULL;
    }
    return &handle->wheels[wheel].tlm;
}

const struct EphorusGeneralTelemetry *ephorus_api_get_general_telemetry(const struct EphorusHandler *handle) {
    return handle == NULL ? NULL : &handle->general;
}
