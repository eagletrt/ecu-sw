/*!
 * \file test_ephorus.c
 * \author Alessandro Bridi
 * \date 2026-08-01
 * \brief Unit tests for the Ephorus 3.1 inverter driver (pure logic).
 *
 * \details Exercises the driver in isolation through its own handle - no CAN
 *     transport, no module. Three concerns:
 *       - command state (init / attach / arm / disarm / run / torque clamp);
 *       - setpoint serialization, i.e. the torque-control-via-speed-rail feature,
 *         drive gating and the one-shot ack/reset pulses;
 *       - received-frame decode: per-wheel telemetry, shared telemetry, and the
 *         fault model (per-inverter, shared-global and per-DC-bus latching).
 */

#include <unity.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "can-inverters.h"
#include "ephorus-api.h"
#include "can-inverters-api.h"

static struct EphorusHandler handle;

void setUp(void) {
    ephorus_api_init(&handle);
}

void tearDown(void) {
}

/*!
 * \brief Build a wheel's setpoint frame and decode it back into the struct.
 *
 * \param wheel The wheel to build and decode.
 *
 * \return The decoded setpoints struct.
 */
static struct CanInvertersEphorusinverter1setpoints build_decode(enum EphorusWheel wheel) {
    uint32_t id = 0;
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    ephorus_api_build_setpoints(&handle, wheel, &id, data);

    union CanInvertersMessages msg = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_deserialize_from_id((enum CanInvertersMessageFrameId)id, data, &msg));
    return msg.ephorusinverter1setpoints;
}

void test_init_null_handle_is_rejected(void) {
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_init(NULL));
}

void test_init_leaves_all_wheels_inactive_and_zeroed(void) {
    float torques[EPHORUS_WHEEL_COUNT] = { 0 };
    float expected_torques[EPHORUS_WHEEL_COUNT] = { 0 };
    enum EphorusInverterState states[EPHORUS_WHEEL_COUNT] = { 0 };
    enum EphorusInverterState expected_states[EPHORUS_WHEEL_COUNT] = { 0 };
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        torques[wheel] = handle.wheels[wheel].torque_nm;
        states[wheel] = handle.wheels[wheel].inverter_state;
    }

    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected_torques, torques, EPHORUS_WHEEL_COUNT, "All wheels must start with 0 torque");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected_states, states, EPHORUS_WHEEL_COUNT, "All wheels must start disarmed (no torque allowed)");
}

void test_attach_activates_only_the_target_wheel(void) {
    TEST_ASSERT_EQUAL(EPHORUS_RC_OK, ephorus_api_attach(&handle, EPHORUS_WHEEL_REAR_LEFT));
    TEST_ASSERT_TRUE(handle.wheels[EPHORUS_WHEEL_REAR_LEFT].enabled);
    TEST_ASSERT_FALSE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].enabled);
}

void test_attach_rejects_invalid_wheel(void) {
    TEST_ASSERT_EQUAL(EPHORUS_RC_INVALID_WHEEL, ephorus_api_attach(&handle, EPHORUS_WHEEL_COUNT));
}

void test_attach_rejects_null_handle(void) {
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_attach(NULL, EPHORUS_WHEEL_FRONT_LEFT));
}

void test_commands_on_inactive_wheel_are_ignored(void) {
    // Not attached: every command is a no-op (no state escapes).
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 10.0f);

    TEST_ASSERT_TRUE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].inverter_state != EPHORUS_INVERTER_STATE_ARMED, "arm must be ignored while inactive");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].torque_nm, "set_torque must be ignored while inactive");
}

void test_arm_enables_and_requests_ack_reset_pulses(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    struct EphorusWheelState *w = &handle.wheels[EPHORUS_WHEEL_FRONT_LEFT];
    TEST_ASSERT_TRUE_MESSAGE(w->inverter_state == EPHORUS_INVERTER_STATE_ARMED, "arm must set the inverter state to ARMED");
    TEST_ASSERT_TRUE_MESSAGE(w->ack_pulse, "arm should request a one-shot AckErr pulse");
    TEST_ASSERT_TRUE_MESSAGE(w->reset_pulse, "arm should request a one-shot ResetError pulse");
}

void test_disarm_clears_run_and_arm(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].inverter_state = EPHORUS_INVERTER_STATE_ARMED;
    ephorus_api_disarm(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    TEST_ASSERT_EQUAL(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].inverter_state, EPHORUS_INVERTER_STATE_DISARMED);
}

void test_set_torque_clamps_high(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 1000.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(EPHORUS_MAX_TORQUE_NM, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].torque_nm, "positive request must clamp to +MAX");
}

void test_set_torque_clamps_low(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, -1000.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-EPHORUS_MAX_TORQUE_NM, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].torque_nm, "negative request must clamp to -MAX");
}

void test_build_setpoints_null_handler(void) {
    uint32_t id = 0;
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_build_setpoints(NULL, EPHORUS_WHEEL_FRONT_LEFT, &id, data));
}

void test_build_setpoints_null_id(void) {
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_FRONT_LEFT, NULL, data));
}

void test_build_setpoints_null_data(void) {
    uint32_t id = 0;
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_FRONT_LEFT, &id, NULL));
}

void test_build_setpoints_invalid_wheel(void) {
    uint32_t id = 0;
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    TEST_ASSERT_EQUAL(EPHORUS_RC_INVALID_WHEEL, ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_COUNT, &id, data));
}

void test_build_setpoints_positive_torque_drive_rail(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 12.0f);

    struct CanInvertersEphorusinverter1setpoints s = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, s.enableinverter, "Armed + running wheel must be enabled");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 12.0f, s.torquelimitpositive, "Positive request => upper bound = request");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitnegative, "Positive request => lower bound = 0");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(EPHORUS_DRIVE_SPEED_RPM, s.speedsetpoint, "Positive request => speed rail = 20000");
}

void test_build_setpoints_negative_torque_brake_rail(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_REAR_RIGHT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_REAR_RIGHT);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_REAR_RIGHT, -8.0f);

    struct CanInvertersEphorusinverter1setpoints s = build_decode(EPHORUS_WHEEL_REAR_RIGHT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, s.enableinverter, "Armed + running wheel must be enabled");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitpositive, "Negative request => upper bound = 0");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, -8.0f, s.torquelimitnegative, "Negative request => lower bound = request");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(0, s.speedsetpoint, "Negative request => speed rail = 0");
}

void test_build_setpoints_disarmed_is_disabled_and_zero(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 12.0f);
    // never armed

    struct CanInvertersEphorusinverter1setpoints s = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, s.enableinverter, "disarmed wheel must not enable");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitpositive, "disarmed wheel must emit 0 torque upper bound");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(EPHORUS_DRIVE_SPEED_RPM, s.speedsetpoint, "disarmed wheel must still emit max speed rail");
}

void test_build_setpoints_consumes_ack_and_reset_pulses_once(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    uint32_t id = 0;
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_FRONT_LEFT, &id, data); // consumes acks

    struct CanInvertersEphorusinverter1setpoints second = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, second.ackerr, "AckErr is a one-shot pulse");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, second.reseterror, "ResetError is a one-shot pulse");
}

void test_handle_frame_decodes_wheel_outbound_a(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    union CanInvertersMessages msg = {
        .ephorusinverter1telemetrya.inverterstate = EPHORUS_STATE_DRIVE,
        .ephorusinverter1telemetrya.inverterready = 1,
        .ephorusinverter1telemetrya.torqueactual = 4.5f,
        .ephorusinverter1telemetrya.temperaturemotor = 42.0f,
        .ephorusinverter1telemetrya.temperaturepowerswitches = 55.0f,
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER1TELEMETRYA, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER1TELEMETRYA, data);

    const struct EphorusWheelTelemetry *t = ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_MESSAGE(EPHORUS_STATE_DRIVE, t->state, "state must be decoded");
    TEST_ASSERT_TRUE_MESSAGE(t->ready, "ready must be decoded");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5f, 4.5f, t->torque_nm, "torque must be decoded");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, 42.0f, t->temp_motor_c, "motor temp must be decoded");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, 55.0f, t->temp_switches_c, "switches temp must be decoded");
}

void test_handle_frame_decodes_wheel_outbound_b(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_RIGHT);

    union CanInvertersMessages msg = {
        .ephorusinverter1telemetryb.speedactual = -2500,
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYB, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYB, data);

    const struct EphorusWheelTelemetry *t = ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_RIGHT);
    TEST_ASSERT_EQUAL_INT16_MESSAGE(-2500, t->speed_rpm, "speed must be decoded");
}

void test_handle_frame_routes_only_to_addressed_wheel(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_RIGHT);

    // Inverter 2 (front-right) OutboundB only.
    union CanInvertersMessages msg = {
        .ephorusinverter1telemetryb.speedactual = 999,
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYB, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYB, data);

    TEST_ASSERT_EQUAL_INT16_MESSAGE(999, ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_RIGHT)->speed_rpm, "addressed wheel updates");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(0, ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_LEFT)->speed_rpm, "other wheel is untouched");
}

void test_handle_frame_decodes_general_telemetry(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    union CanInvertersMessages msg = {
        .ephorustelemetry.dclinkvoltage12actual = 560.0f,
        .ephorustelemetry.dclinkvoltage34actual = 548.0f,
        .ephorustelemetry.dclinkgood12 = 1,
        .ephorustelemetry.dclinkgood34 = 0,
        .ephorustelemetry.mirrorcontrolenable = 1,
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSTELEMETRY, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSTELEMETRY, data);

    const struct EphorusGeneralTelemetry *g = ephorus_api_get_general_telemetry(&handle);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 560.0f, g->dclink_voltage_12_v);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 548.0f, g->dclink_voltage_34_v);
    TEST_ASSERT_TRUE(g->dclink_good_12);
    TEST_ASSERT_FALSE(g->dclink_good_34);
    TEST_ASSERT_TRUE(g->enable_mirror);
}

void test_handle_frame_ignores_inactive_wheel(void) {
    // Wheel not attached: its telemetry must stay zeroed.
    union CanInvertersMessages msg = {
        .ephorusinverter1telemetryb.speedactual = 4321,
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYB, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSINVERTER2TELEMETRYB, data);

    TEST_ASSERT_EQUAL_INT16(0, ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_LEFT)->speed_rpm);
}

void test_faults_per_inverter_latch_only_that_wheel(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_RIGHT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_RIGHT);

    union CanInvertersMessages msg = {
        .ephoruserror.inverter2overcurrent = 1, // inverter 2 == front-right
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSERROR, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSERROR, data);

    TEST_ASSERT_TRUE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_RIGHT].inverter_state == EPHORUS_INVERTER_STATE_FAULT, "front-right must latch its overcurrent fault");
    TEST_ASSERT_TRUE_MESSAGE((ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_RIGHT)->fault_bits & (1u << EPHORUS_WHEEL_FAULT_OVERCURRENT)) != 0, "front-right telemetry must reflect the overcurrent fault");
    TEST_ASSERT_TRUE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].inverter_state != EPHORUS_INVERTER_STATE_FAULT, "front-left must be unaffected by inverter-2's fault");
}

void test_faults_global_hit_every_active_wheel(void) {
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        ephorus_api_attach(&handle, wheel);
        ephorus_api_arm(&handle, wheel);
    }

    enum EphorusInverterState expected_states[EPHORUS_WHEEL_COUNT] = { 0 };
    enum EphorusInverterState states[EPHORUS_WHEEL_COUNT] = { 0 };

    union CanInvertersMessages msg = {
        .ephoruserror.controldisabled = 1, // system-wide
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSERROR, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSERROR, data);

    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        states[wheel] = handle.wheels[wheel].inverter_state;
        expected_states[wheel] = EPHORUS_INVERTER_STATE_FAULT;
    }

    TEST_ASSERT_TRUE_MESSAGE((ephorus_api_get_general_telemetry(&handle)->fault_bits & (1u << EPHORUS_GENERAL_FAULT_CONTROL_DISABLED)) != 0, "global fault must be reflected in general telemetry");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(expected_states, states, sizeof(expected_states), "global fault must latch every active wheel");
}

void test_faults_dc_bus_hit_only_that_pair(void) {
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        ephorus_api_attach(&handle, wheel);
        ephorus_api_arm(&handle, wheel);
    }

    // DC bus "12" undervoltage affects wheels 0-1 only, not the "34" pair.
    union CanInvertersMessages msg = {
        .ephoruserror.inverter12dcundervoltage = 1,
    };
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    EAGLETRT_API_UNUSED(can_inverters_api_serialize_from_id(CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSERROR, &msg, data));
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_EPHORUSERROR, data);

    TEST_ASSERT_EQUAL_MESSAGE(EPHORUS_INVERTER_STATE_FAULT, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].inverter_state, "bus-12 fault latches front-left");
    TEST_ASSERT_EQUAL_MESSAGE(EPHORUS_INVERTER_STATE_FAULT, handle.wheels[EPHORUS_WHEEL_FRONT_RIGHT].inverter_state, "bus-12 fault latches front-right");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(EPHORUS_INVERTER_STATE_FAULT, handle.wheels[EPHORUS_WHEEL_REAR_LEFT].inverter_state, "bus-12 fault must spare rear-left (bus 34)");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(EPHORUS_INVERTER_STATE_FAULT, handle.wheels[EPHORUS_WHEEL_REAR_RIGHT].inverter_state, "bus-12 fault must spare rear-right (bus 34)");
}

void test_rearm_clears_a_latched_fault(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].inverter_state = EPHORUS_INVERTER_STATE_FAULT;

    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(EPHORUS_INVERTER_STATE_FAULT, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].inverter_state, "re-arming must clear the latched fault");
}

void test_telemetry_wheel_get_success(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    const struct EphorusWheelTelemetry *t = ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_NOT_NULL(t);
}

void test_telemetry_wheel_get_invalid(void) {
    const struct EphorusWheelTelemetry *t = ephorus_api_get_wheel_telemetry(&handle, EPHORUS_WHEEL_COUNT);
    TEST_ASSERT_NULL(t);
}

void test_telemetry_wheel_get_null_handle(void) {
    const struct EphorusWheelTelemetry *t = ephorus_api_get_wheel_telemetry(NULL, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_NULL(t);
}

void test_telemetry_general_get_success(void) {
    const struct EphorusGeneralTelemetry *g = ephorus_api_get_general_telemetry(&handle);
    TEST_ASSERT_NOT_NULL(g);
}

void test_telemetry_general_get_null_handle(void) {
    const struct EphorusGeneralTelemetry *g = ephorus_api_get_general_telemetry(NULL);
    TEST_ASSERT_NULL(g);
}

int main(void) {
    UNITY_BEGIN();

    // init / attach
    RUN_TEST(test_init_null_handle_is_rejected);
    RUN_TEST(test_init_leaves_all_wheels_inactive_and_zeroed);
    RUN_TEST(test_attach_activates_only_the_target_wheel);
    RUN_TEST(test_attach_rejects_invalid_wheel);
    RUN_TEST(test_attach_rejects_null_handle);

    // command state
    RUN_TEST(test_commands_on_inactive_wheel_are_ignored);
    RUN_TEST(test_arm_enables_and_requests_ack_reset_pulses);
    RUN_TEST(test_disarm_clears_run_and_arm);
    RUN_TEST(test_set_torque_clamps_high);
    RUN_TEST(test_set_torque_clamps_low);

    // build_setpoints
    RUN_TEST(test_build_setpoints_null_handler);
    RUN_TEST(test_build_setpoints_null_id);
    RUN_TEST(test_build_setpoints_null_data);
    RUN_TEST(test_build_setpoints_invalid_wheel);
    RUN_TEST(test_build_setpoints_positive_torque_drive_rail);
    RUN_TEST(test_build_setpoints_negative_torque_brake_rail);
    RUN_TEST(test_build_setpoints_disarmed_is_disabled_and_zero);
    RUN_TEST(test_build_setpoints_consumes_ack_and_reset_pulses_once);

    // telemetry decode
    RUN_TEST(test_handle_frame_decodes_wheel_outbound_a);
    RUN_TEST(test_handle_frame_decodes_wheel_outbound_b);
    RUN_TEST(test_handle_frame_routes_only_to_addressed_wheel);
    RUN_TEST(test_handle_frame_decodes_general_telemetry);
    RUN_TEST(test_handle_frame_ignores_inactive_wheel);

    // fault model
    RUN_TEST(test_faults_per_inverter_latch_only_that_wheel);
    RUN_TEST(test_faults_global_hit_every_active_wheel);
    RUN_TEST(test_faults_dc_bus_hit_only_that_pair);
    RUN_TEST(test_rearm_clears_a_latched_fault);

    // getters + names
    RUN_TEST(test_telemetry_wheel_get_success);
    RUN_TEST(test_telemetry_wheel_get_invalid);
    RUN_TEST(test_telemetry_wheel_get_null_handle);
    RUN_TEST(test_telemetry_general_get_success);
    RUN_TEST(test_telemetry_general_get_null_handle);

    return UNITY_END();
}
