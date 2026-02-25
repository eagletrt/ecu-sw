#include <unity.h>
#include "pedals.h"

/* Mock CAN interface */

/*!
 * \brief Simulates the arrival of a CAN frame from the pedal board.
 * \note In a real implementation, the online parameter will probably be managed by a watchdog or similar
 */
void mock_can_callback(float throttle, float brake_p, float brake_bar, bool online) {
    pedals_set_is_available(online);
    pedals_set_throttle_pct(throttle);
    pedals_set_brake_pct(brake_p);
    pedals_set_brake_pressure(brake_bar);
}

// ------------------------------------------

void setUp(void) {
    pedals_init();
}

void tearDown(void) {
}

/* --- Test Cases --- */

/*!
 * \brief Simulates normal CAN data flow and verifies torque mapping.
 */
void test_pedals_normal_driving_via_mock_can(void) {
    // simulate CAN frame: 25% throttle, 0% brake, 0bar, online
    mock_can_callback(25.0f, 0.0f, 0.0f, true);

    const struct PedalsHandler *h = pedals_get_values();
    TEST_ASSERT_TRUE_MESSAGE(h->is_available, "Pedals should be available after online CAN frame");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(25.0f, h->throttle_pct, "Throttle percentage should match CAN input");

    // torque should be 25% of 91.0Nm = 22.75Nm
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(22.75f, pedals_get_requested_throttle_torque(), "Torque calculation should be 25% of max torque");
}

/*!
 * \brief Verifies that if CAN indicates 'offline', torque is killed immediately.
 */
void test_pedals_failsafe_when_can_offline(void) {
    // normal operation
    mock_can_callback(100.0f, 0.0f, 0.0f, true);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(PEDALS_MAX_TORQUE, pedals_get_requested_throttle_torque(), "Max torque should be requested when throttle is 100%");

    // CAN disconnects
    mock_can_callback(100.0f, 0.0f, 0.0f, false);

    // torque must be 0 even if the last throttle was 100%
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_get_requested_throttle_torque(), "Torque must be 0.0f when pedals are unavailable");
}

/*!
 * \brief Verifies that the module ignores values outside physical limits sent via CAN.
 */
void test_pedals_ignores_invalid_can_data(void) {
    mock_can_callback(10.0f, 0.0f, 0.0f, true);

    // receive corrupted CAN frame with 150% throttle
    mock_can_callback(150.0f, 0.0f, 0.0f, true);

    // value should remain 10% (last valid state)
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(10.0f, pedals_get_values()->throttle_pct, "Throttle percentage should not update with out-of-bounds values");
}

/*!
 * \brief Verifies that brake_is_pressed reacts correctly to the threshold.
 */
void test_pedals_brake_threshold_logic(void) {
    pedals_set_is_available(true);

    // below threshold (PEDALS_BRAKE_THRESHOLD_PCT = 5.0)
    mock_can_callback(0.0f, 4.5f, 2.0f, true);
    TEST_ASSERT_FALSE_MESSAGE(pedals_is_brake_pressed(), "Brake should report NOT pressed below threshold");

    // above threshold
    mock_can_callback(0.0f, 5.5f, 3.0f, true);
    TEST_ASSERT_TRUE_MESSAGE(pedals_is_brake_pressed(), "Brake should report pressed above threshold");
}

// -------------------------------

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pedals_normal_driving_via_mock_can);
    RUN_TEST(test_pedals_failsafe_when_can_offline);
    RUN_TEST(test_pedals_ignores_invalid_can_data);
    RUN_TEST(test_pedals_brake_threshold_logic);
    return UNITY_END();
}