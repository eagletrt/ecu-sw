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

#include "ephorus-api.h"
#include "can-inverters-api.h"

static struct EphorusHandler handle;

static const uint32_t setpoint_ids[EPHORUS_WHEEL_COUNT] = {
    CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER1SETPOINTS,
    CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER2SETPOINTS,
    CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER3SETPOINTS,
    CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER4SETPOINTS,
};

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
static struct CanInvertersInverter1setpoints build_decode(enum EphorusWheel wheel) {
    uint32_t id = 0;
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    TEST_ASSERT_EQUAL_MESSAGE(EPHORUS_RC_OK, ephorus_api_build_setpoints(&handle, wheel, &id, data), "build_setpoints should succeed");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(setpoint_ids[wheel], id, "setpoints must be emitted under the wheel's own frame id");

    union CanInvertersMessages msg = { 0 };
    TEST_ASSERT_EQUAL_MESSAGE(0, can_inverters_api_deserialize_from_id((enum CanInvertersMessageFrameId)id, data, &msg), "frame should deserialize");
    return msg.inverter1setpoints;
}

/*!
 * \brief Serialize \p msg under \p id and feed it straight into the driver.
 *
 * \param id The message ID to serialize.
 * \param msg The message to serialize.
 */
static void feed(enum CanInvertersMessageFrameId id, union CanInvertersMessages *msg) {
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    TEST_ASSERT_TRUE_MESSAGE(can_inverters_api_serialize_from_id(id, msg, data) >= 0, "message should serialize");
    ephorus_api_handle_frame(&handle, (uint32_t)id, data);
}

void test_init_null_handle_is_rejected(void) {
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_init(NULL));
}

void test_init_leaves_all_wheels_inactive_and_zeroed(void) {
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        TEST_ASSERT_FALSE_MESSAGE(handle.wheels[wheel].active, "wheels start inactive");
        TEST_ASSERT_FALSE_MESSAGE(handle.wheels[wheel].armed, "wheels start disarmed");
        TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, handle.wheels[wheel].torque_nm, "torque request starts at 0");
    }
}

void test_attach_activates_only_the_target_wheel(void) {
    TEST_ASSERT_EQUAL(EPHORUS_RC_OK, ephorus_api_attach(&handle, EPHORUS_WHEEL_REAR_LEFT));
    TEST_ASSERT_TRUE(handle.wheels[EPHORUS_WHEEL_REAR_LEFT].active);
    TEST_ASSERT_FALSE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].active);
}

void test_attach_rejects_invalid_wheel_and_null(void) {
    TEST_ASSERT_EQUAL(EPHORUS_RC_INVALID_WHEEL, ephorus_api_attach(&handle, EPHORUS_WHEEL_COUNT));
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_attach(NULL, EPHORUS_WHEEL_FRONT_LEFT));
}

void test_commands_on_inactive_wheel_are_ignored(void) {
    // Not attached: every command is a no-op (no state escapes).
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_FRONT_LEFT, true);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 10.0f);

    TEST_ASSERT_FALSE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].armed, "arm must be ignored while inactive");
    TEST_ASSERT_FALSE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].running, "set_run must be ignored while inactive");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].torque_nm, "set_torque must be ignored while inactive");
}

void test_arm_enables_and_requests_ack_reset_pulses(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    struct EphorusWheelState *w = &handle.wheels[EPHORUS_WHEEL_FRONT_LEFT];
    TEST_ASSERT_TRUE(w->armed);
    TEST_ASSERT_FALSE(w->faulted);
    TEST_ASSERT_TRUE_MESSAGE(w->ack_pulse, "arm should request a one-shot AckErr pulse");
    TEST_ASSERT_TRUE_MESSAGE(w->reset_pulse, "arm should request a one-shot ResetError pulse");
}

void test_disarm_clears_run_and_arm(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_FRONT_LEFT, true);

    ephorus_api_disarm(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    TEST_ASSERT_FALSE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].armed);
    TEST_ASSERT_FALSE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].running);
}

void test_toggle_run_flips_the_request(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_FALSE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].running);
    ephorus_api_toggle_run(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_TRUE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].running);
    ephorus_api_toggle_run(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_FALSE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].running);
}

void test_set_torque_clamps_both_directions(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 1000.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(EPHORUS_MAX_TORQUE_NM, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].torque_nm, "positive request must clamp to +MAX");

    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, -1000.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-EPHORUS_MAX_TORQUE_NM, handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].torque_nm, "negative request must clamp to -MAX");
}

void test_build_setpoints_error_paths(void) {
    uint32_t id = 0;
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };

    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_build_setpoints(NULL, EPHORUS_WHEEL_FRONT_LEFT, &id, data));
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_FRONT_LEFT, NULL, data));
    TEST_ASSERT_EQUAL(EPHORUS_RC_NULL_POINTER, ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_FRONT_LEFT, &id, NULL));
    TEST_ASSERT_EQUAL(EPHORUS_RC_INVALID_WHEEL, ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_COUNT, &id, data));
    // Attached to none: the wheel is inactive.
    TEST_ASSERT_EQUAL(EPHORUS_RC_INACTIVE, ephorus_api_build_setpoints(&handle, EPHORUS_WHEEL_FRONT_LEFT, &id, data));
}

void test_build_setpoints_positive_torque_drive_rail(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_FRONT_LEFT, true);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 12.0f);

    struct CanInvertersInverter1setpoints s = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8(1, s.enableinverter);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, s.torquelimitpositive);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.torquelimitnegative);
    TEST_ASSERT_EQUAL_INT16(EPHORUS_DRIVE_SPEED_RPM, s.speedsetpoint);
}

void test_build_setpoints_negative_torque_brake_rail(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_REAR_RIGHT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_REAR_RIGHT);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_REAR_RIGHT, true);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_REAR_RIGHT, -8.0f);

    struct CanInvertersInverter1setpoints s = build_decode(EPHORUS_WHEEL_REAR_RIGHT);
    TEST_ASSERT_EQUAL_UINT8(1, s.enableinverter);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.torquelimitpositive);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -8.0f, s.torquelimitnegative);
    TEST_ASSERT_EQUAL_INT16(0, s.speedsetpoint);
}

void test_build_setpoints_disarmed_is_disabled_and_zero(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_FRONT_LEFT, true);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 12.0f);
    // never armed

    struct CanInvertersInverter1setpoints s = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, s.enableinverter, "disarmed wheel must not enable");
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.torquelimitpositive);
    TEST_ASSERT_EQUAL_INT16(0, s.speedsetpoint);
}

void test_build_setpoints_armed_but_not_running_is_enabled_but_zero_torque(void) {
    // drive = armed && !faulted -> enabled; torque only flows while running.
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_torque(&handle, EPHORUS_WHEEL_FRONT_LEFT, 12.0f);
    // running left false

    struct CanInvertersInverter1setpoints s = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, s.enableinverter, "armed wheel is enabled even when not running");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitpositive, "no torque window while not running");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(0, s.speedsetpoint, "no speed rail while not running");
}

void test_build_setpoints_consumes_ack_and_reset_pulses_once(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    struct CanInvertersInverter1setpoints first = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, first.ackerr, "first frame after arm carries AckErr");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, first.reseterror, "first frame after arm carries ResetError");

    struct CanInvertersInverter1setpoints second = build_decode(EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, second.ackerr, "AckErr is a one-shot pulse");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, second.reseterror, "ResetError is a one-shot pulse");
}

void test_handle_frame_decodes_wheel_outbound_a(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    union CanInvertersMessages msg = { 0 };
    msg.inverter1outbounda.inverterstate = EPHORUS_STATE_DRIVE;
    msg.inverter1outbounda.inverterready = 1;
    msg.inverter1outbounda.torqueactual = 4.5f;
    msg.inverter1outbounda.temperaturemotor = 42.0f;
    msg.inverter1outbounda.temperaturepowerswitches = 55.0f;
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER1OUTBOUNDA, &msg);

    const struct EphorusWheelTelemetry *t = ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_EQUAL(EPHORUS_STATE_DRIVE, t->state);
    TEST_ASSERT_TRUE(t->ready);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 4.5f, t->torque_nm);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 42.0f, t->temp_motor_c);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 55.0f, t->temp_switches_c);
}

void test_handle_frame_decodes_wheel_outbound_b(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_RIGHT);

    union CanInvertersMessages msg = { 0 };
    msg.inverter1outboundb.speedactual = -2500;
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER2OUTBOUNDB, &msg);

    const struct EphorusWheelTelemetry *t = ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_RIGHT);
    TEST_ASSERT_EQUAL_INT16(-2500, t->speed_rpm);
}

void test_handle_frame_routes_only_to_addressed_wheel(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_RIGHT);

    // Inverter 2 (front-right) OutboundB only.
    union CanInvertersMessages msg = { 0 };
    msg.inverter1outboundb.speedactual = 999;
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER2OUTBOUNDB, &msg);

    TEST_ASSERT_EQUAL_INT16_MESSAGE(999, ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_RIGHT)->speed_rpm, "addressed wheel updates");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(0, ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_LEFT)->speed_rpm, "other wheel is untouched");
}

void test_handle_frame_decodes_general_telemetry(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);

    union CanInvertersMessages msg = { 0 };
    msg.generaloutbound.dclinkvoltage12actual = 560.0f;
    msg.generaloutbound.dclinkvoltage34actual = 548.0f;
    msg.generaloutbound.dclinkgood12 = 1;
    msg.generaloutbound.dclinkgood34 = 0;
    msg.generaloutbound.mirrorcontrolenable = 1;
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_GENERALOUTBOUND, &msg);

    const struct EphorusGeneralTelemetry *g = ephorus_api_general_telemetry(&handle);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 560.0f, g->dclink_voltage_12_v);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 548.0f, g->dclink_voltage_34_v);
    TEST_ASSERT_TRUE(g->dclink_good_12);
    TEST_ASSERT_FALSE(g->dclink_good_34);
    TEST_ASSERT_TRUE(g->enable_mirror);
}

void test_handle_frame_ignores_inactive_wheel(void) {
    // Wheel not attached: its telemetry must stay zeroed.
    union CanInvertersMessages msg = { 0 };
    msg.inverter1outboundb.speedactual = 4321;
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_INVERTER1OUTBOUNDB, &msg);

    TEST_ASSERT_EQUAL_INT16(0, ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_LEFT)->speed_rpm);
}

void test_handle_frame_invalid_id_and_nulls_are_safe(void) {
    // Must not crash / must not alter state.
    ephorus_api_handle_frame(&handle, 0x7FF, (const uint8_t[EPHORUS_FRAME_DATA_SIZE]){ 0 });
    ephorus_api_handle_frame(NULL, CAN_INVERTERS_MESSAGE_FRAME_ID_GENERALOUTBOUND, (const uint8_t[EPHORUS_FRAME_DATA_SIZE]){ 0 });
    ephorus_api_handle_frame(&handle, CAN_INVERTERS_MESSAGE_FRAME_ID_GENERALOUTBOUND, NULL);
    TEST_PASS();
}

void test_faults_per_inverter_latch_only_that_wheel(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_RIGHT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_RIGHT);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_FRONT_LEFT, true);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_FRONT_RIGHT, true);

    union CanInvertersMessages msg = { 0 };
    msg.generalerrorbits.err_inverter2_overcurrent = 1; // inverter 2 == front-right
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_GENERALERRORBITS, &msg);

    TEST_ASSERT_TRUE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_RIGHT].faulted, "front-right must latch its overcurrent fault");
    TEST_ASSERT_FALSE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_RIGHT].running, "a latched fault clears the run request");
    TEST_ASSERT_TRUE((ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_FRONT_RIGHT)->fault_bits & (1u << EPHORUS_WHEEL_FAULT_OVERCURRENT)) != 0);

    TEST_ASSERT_FALSE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].faulted, "front-left must be unaffected by inverter-2's fault");
    TEST_ASSERT_TRUE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].running, "front-left keeps running");
}

void test_faults_global_hit_every_active_wheel(void) {
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        ephorus_api_attach(&handle, wheel);
        ephorus_api_arm(&handle, wheel);
        ephorus_api_set_run(&handle, wheel, true);
    }

    union CanInvertersMessages msg = { 0 };
    msg.generalerrorbits.err_controldisabled = 1; // system-wide
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_GENERALERRORBITS, &msg);

    TEST_ASSERT_TRUE((ephorus_api_general_telemetry(&handle)->fault_bits & (1u << EPHORUS_GENERAL_FAULT_CONTROL_DISABLED)) != 0);
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        TEST_ASSERT_TRUE_MESSAGE(handle.wheels[wheel].faulted, "a global fault must latch on every active wheel");
        TEST_ASSERT_FALSE_MESSAGE(handle.wheels[wheel].running, "a global fault clears every run request");
    }
}

void test_faults_dc_bus_hit_only_that_pair(void) {
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        ephorus_api_attach(&handle, wheel);
        ephorus_api_arm(&handle, wheel);
        ephorus_api_set_run(&handle, wheel, true);
    }

    // DC bus "12" undervoltage affects wheels 0-1 only, not the "34" pair.
    union CanInvertersMessages msg = { 0 };
    msg.generalerrorbits.err_inverter1_2_dc_undervoltage = 1;
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_GENERALERRORBITS, &msg);

    TEST_ASSERT_TRUE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].faulted, "bus-12 fault latches front-left");
    TEST_ASSERT_TRUE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_RIGHT].faulted, "bus-12 fault latches front-right");
    TEST_ASSERT_FALSE_MESSAGE(handle.wheels[EPHORUS_WHEEL_REAR_LEFT].faulted, "bus-12 fault must spare rear-left (bus 34)");
    TEST_ASSERT_FALSE_MESSAGE(handle.wheels[EPHORUS_WHEEL_REAR_RIGHT].faulted, "bus-12 fault must spare rear-right (bus 34)");
}

void test_rearm_clears_a_latched_fault(void) {
    ephorus_api_attach(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    ephorus_api_set_run(&handle, EPHORUS_WHEEL_FRONT_LEFT, true);

    union CanInvertersMessages msg = { 0 };
    msg.generalerrorbits.err_inverter1_overcurrent = 1;
    feed(CAN_INVERTERS_MESSAGE_FRAME_ID_GENERALERRORBITS, &msg);
    TEST_ASSERT_TRUE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].faulted);

    ephorus_api_arm(&handle, EPHORUS_WHEEL_FRONT_LEFT);
    TEST_ASSERT_FALSE_MESSAGE(handle.wheels[EPHORUS_WHEEL_FRONT_LEFT].faulted, "re-arming must clear the latched fault");
}

void test_telemetry_getters_bounds(void) {
    TEST_ASSERT_NOT_NULL(ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_REAR_RIGHT));
    TEST_ASSERT_NULL(ephorus_api_wheel_telemetry(&handle, EPHORUS_WHEEL_COUNT));
    TEST_ASSERT_NULL(ephorus_api_wheel_telemetry(NULL, EPHORUS_WHEEL_FRONT_LEFT));
    TEST_ASSERT_NOT_NULL(ephorus_api_general_telemetry(&handle));
    TEST_ASSERT_NULL(ephorus_api_general_telemetry(NULL));
}

void test_name_helpers_never_null(void) {
    TEST_ASSERT_EQUAL_STRING("Drive", ephorus_api_state_name(EPHORUS_STATE_DRIVE));
    TEST_ASSERT_EQUAL_STRING("?", ephorus_api_state_name((enum EphorusState)99));
    TEST_ASSERT_EQUAL_STRING("Overcurrent", ephorus_api_wheel_fault_name(EPHORUS_WHEEL_FAULT_OVERCURRENT));
    TEST_ASSERT_EQUAL_STRING("?", ephorus_api_wheel_fault_name(999));
    TEST_ASSERT_EQUAL_STRING("ControlDisabled", ephorus_api_general_fault_name(EPHORUS_GENERAL_FAULT_CONTROL_DISABLED));
    TEST_ASSERT_EQUAL_STRING("?", ephorus_api_general_fault_name(999));
}

int main(void) {
    UNITY_BEGIN();

    // init / attach
    RUN_TEST(test_init_null_handle_is_rejected);
    RUN_TEST(test_init_leaves_all_wheels_inactive_and_zeroed);
    RUN_TEST(test_attach_activates_only_the_target_wheel);
    RUN_TEST(test_attach_rejects_invalid_wheel_and_null);

    // command state
    RUN_TEST(test_commands_on_inactive_wheel_are_ignored);
    RUN_TEST(test_arm_enables_and_requests_ack_reset_pulses);
    RUN_TEST(test_disarm_clears_run_and_arm);
    RUN_TEST(test_toggle_run_flips_the_request);
    RUN_TEST(test_set_torque_clamps_both_directions);

    // build_setpoints
    RUN_TEST(test_build_setpoints_error_paths);
    RUN_TEST(test_build_setpoints_positive_torque_drive_rail);
    RUN_TEST(test_build_setpoints_negative_torque_brake_rail);
    RUN_TEST(test_build_setpoints_disarmed_is_disabled_and_zero);
    RUN_TEST(test_build_setpoints_armed_but_not_running_is_enabled_but_zero_torque);
    RUN_TEST(test_build_setpoints_consumes_ack_and_reset_pulses_once);

    // telemetry decode
    RUN_TEST(test_handle_frame_decodes_wheel_outbound_a);
    RUN_TEST(test_handle_frame_decodes_wheel_outbound_b);
    RUN_TEST(test_handle_frame_routes_only_to_addressed_wheel);
    RUN_TEST(test_handle_frame_decodes_general_telemetry);
    RUN_TEST(test_handle_frame_ignores_inactive_wheel);
    RUN_TEST(test_handle_frame_invalid_id_and_nulls_are_safe);

    // fault model
    RUN_TEST(test_faults_per_inverter_latch_only_that_wheel);
    RUN_TEST(test_faults_global_hit_every_active_wheel);
    RUN_TEST(test_faults_dc_bus_hit_only_that_pair);
    RUN_TEST(test_rearm_clears_a_latched_fault);

    // getters + names
    RUN_TEST(test_telemetry_getters_bounds);
    RUN_TEST(test_name_helpers_never_null);

    return UNITY_END();
}
