#include "shutdown-api.h"
#include <unity.h>
#include "fff.h"

DEFINE_FFF_GLOBALS;

extern struct ShutdownHandler shutdown_handler;

FAKE_VALUE_FUNC(enum ShutdownReturnCode, shutdown_control_relay, bool);

void setUp(void) {
    RESET_FAKE(shutdown_control_relay);
    FFF_RESET_HISTORY();
    shutdown_api_init(shutdown_control_relay);
}

void test_shutdown_api_init(void) {
    memset(&shutdown_handler, 0, sizeof(shutdown_handler));
    enum ShutdownReturnCode rc = shutdown_api_init(shutdown_control_relay);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_OK, rc, "shutdown_api_init should return SHUTDOWN_RC_OK");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(shutdown_control_relay, shutdown_handler.control_relay, "control_relay callback should be set correctly");
}

void test_shutdown_api_init_null_callback(void) {
    enum ShutdownReturnCode rc = shutdown_api_init(NULL);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_ERROR, rc, "shutdown_api_init should return SHUTDOWN_RC_ERROR for NULL callback");
}

void test_shutdown_api_set_voltage_successful(void) {
    shutdown_control_relay_fake.return_val = SHUTDOWN_RC_OK;
    enum ShutdownReturnCode rc = shutdown_api_set_voltage(SHUTDOWN_NAME_BEFORE_ECU, 18.0F);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_OK, rc, "shutdown_api_set_voltage should return SHUTDOWN_RC_OK when callback succeeds");
    TEST_ASSERT_EQUAL_MESSAGE(18.0F, shutdown_handler.voltages[SHUTDOWN_NAME_BEFORE_ECU], "Voltage should be set correctly in shutdown_handler");
}

void test_shutdown_api_set_voltage_invalid_name(void) {
    enum ShutdownReturnCode rc = shutdown_api_set_voltage(SHUTDOWN_NAME_COUNT, 18.0F);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_ERROR, rc, "shutdown_api_set_voltage should return SHUTDOWN_RC_ERROR for invalid name");
}

void test_shutdown_api_control_relay_successful(void) {
    shutdown_control_relay_fake.return_val = SHUTDOWN_RC_OK;
    enum ShutdownReturnCode rc = shutdown_api_control_relay(true);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_OK, rc, "shutdown_api_control_relay should return SHUTDOWN_RC_OK when callback succeeds");
    TEST_ASSERT_EQUAL_MESSAGE(1, shutdown_control_relay_fake.call_count, "shutdown_control_relay should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(true, shutdown_control_relay_fake.arg0_val, "shutdown_control_relay should be called with true");
}

void test_shutdown_api_control_relay_callback_not_set(void) {
    shutdown_handler.control_relay = NULL; // Simulate callback not set
    enum ShutdownReturnCode rc = shutdown_api_control_relay(true);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_ERROR, rc, "shutdown_api_control_relay should return SHUTDOWN_RC_ERROR when callback is not set");
}

void test_shutdown_api_get_state_open(void) {
    shutdown_handler.voltages[SHUTDOWN_NAME_BEFORE_ECU] = 0.0F; // Below lower threshold
    enum ShutdownState state = shutdown_api_get_state(SHUTDOWN_NAME_BEFORE_ECU);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_OPEN, state, "shutdown_api_get_state should return SHUTDOWN_STATE_OPEN for voltage below lower threshold");
}

void test_shutdown_api_get_state_closed(void) {
    shutdown_handler.voltages[SHUTDOWN_NAME_BEFORE_ECU] = 24.0F; // Above upper threshold
    enum ShutdownState state = shutdown_api_get_state(SHUTDOWN_NAME_BEFORE_ECU);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_CLOSED, state, "shutdown_api_get_state should return SHUTDOWN_STATE_CLOSED for voltage above upper threshold");
}

void test_shutdown_api_get_state_error(void) {
    shutdown_handler.voltages[SHUTDOWN_NAME_BEFORE_ECU] = 12.0F; // Implausible state
    enum ShutdownState state = shutdown_api_get_state(SHUTDOWN_NAME_BEFORE_ECU);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_ERROR, state, "shutdown_api_get_state should return SHUTDOWN_STATE_ERROR for implausible voltage");
}

void test_shutdown_api_get_state_invalid_name(void) {
    enum ShutdownState state = shutdown_api_get_state(SHUTDOWN_NAME_COUNT); // Invalid name
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_ERROR, state, "shutdown_api_get_state should return SHUTDOWN_STATE_ERROR for invalid name");
}

void test_shutdown_api_get_voltage_successful(void) {
    shutdown_handler.voltages[SHUTDOWN_NAME_BEFORE_ECU] = 18.0F;
    float voltage = shutdown_api_get_voltage(SHUTDOWN_NAME_BEFORE_ECU);
    TEST_ASSERT_EQUAL_MESSAGE(18.0F, voltage, "shutdown_api_get_voltage should return the correct voltage for a valid name");
}

void test_shutdown_api_get_voltage_invalid_name(void) {
    float voltage = shutdown_api_get_voltage(SHUTDOWN_NAME_COUNT); // Invalid name
    TEST_ASSERT_EQUAL_MESSAGE(0.0F, voltage, "shutdown_api_get_voltage should return 0.0F for invalid name");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_shutdown_api_init);
    RUN_TEST(test_shutdown_api_init_null_callback);

    RUN_TEST(test_shutdown_api_set_voltage_successful);
    RUN_TEST(test_shutdown_api_set_voltage_invalid_name);

    RUN_TEST(test_shutdown_api_control_relay_successful);
    RUN_TEST(test_shutdown_api_control_relay_callback_not_set);

    RUN_TEST(test_shutdown_api_get_state_open);
    RUN_TEST(test_shutdown_api_get_state_closed);
    RUN_TEST(test_shutdown_api_get_state_error);
    RUN_TEST(test_shutdown_api_get_state_invalid_name);

    RUN_TEST(test_shutdown_api_get_voltage_successful);
    RUN_TEST(test_shutdown_api_get_voltage_invalid_name);

    return UNITY_END();
}
