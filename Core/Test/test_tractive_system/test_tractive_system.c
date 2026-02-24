#include <unity.h>
#include "tractive_system.h"

/* Mocking State */
static int mock_on_cmd_count = 0;
static int mock_off_cmd_count = 0;
static enum TSReturnCode next_mock_return = TS_RC_OK;

/* Mock implementations */
enum TSReturnCode mock_send_command(enum TSCommand cmd) {
    if (cmd == TS_COMMAND_ON)
        mock_on_cmd_count++;
    if (cmd == TS_COMMAND_OFF)
        mock_off_cmd_count++;
    return next_mock_return;
}

// ------------------------------------------

void setUp(void) {
    mock_on_cmd_count = 0;
    mock_off_cmd_count = 0;
    next_mock_return = TS_RC_OK;
    TS_init(mock_send_command);
}

void tearDown(void) {
}

/* --- Tests --- */

/**
 * Verifies that the status starts at UNKNOWN and requests are cleared.
 */
void test_ts_init_state(void) {
    TEST_ASSERT_EQUAL(TS_STATUS_UNKNOWN, TS_get_status());
    TEST_ASSERT_FALSE(TS_is_power_on_requested());
    TEST_ASSERT_FALSE(TS_is_power_off_requested());
}

/**
 * Verifies that request_on is set only if hardware command succeeds and 
 * status is appropriate.
 */
void test_ts_request_on_flow(void) {
    TS_set_status(TS_STATUS_OFF);

    // successful request
    enum TSReturnCode rc = TS_request_power_on();
    TEST_ASSERT_EQUAL(TS_RC_OK, rc);
    TEST_ASSERT_TRUE(TS_is_power_on_requested());
    TEST_ASSERT_EQUAL_INT(1, mock_on_cmd_count);

    // test hardware failure
    TS_clear_request();
    next_mock_return = TS_RC_ERROR;
    rc = TS_request_power_on();
    TEST_ASSERT_EQUAL(TS_RC_ERROR, rc);
    TEST_ASSERT_FALSE(TS_is_power_on_requested());
}

/**
 * Verifies that we cannot request power on if the system is already ON.
 */
void test_ts_request_on_guard(void) {
    TS_set_status(TS_STATUS_ON);

    // Requesting ON while already ON should do nothing (return OK but no HW call)
    enum TSReturnCode rc = TS_request_power_on();
    TEST_ASSERT_EQUAL(TS_RC_OK, rc);
    TEST_ASSERT_EQUAL_INT(0, mock_on_cmd_count);
    TEST_ASSERT_FALSE(TS_is_power_on_requested());
}

/**
 * Verifies that OFF can be requested from any state, including FATAL.
 */
void test_ts_request_off_from_anywhere(void) {
    TS_set_status(TS_STATUS_FATAL);

    enum TSReturnCode rc = TS_request_power_off();
    TEST_ASSERT_EQUAL(TS_RC_OK, rc);
    TEST_ASSERT_TRUE(TS_is_power_off_requested());
    TEST_ASSERT_EQUAL_INT(1, mock_off_cmd_count);
}

/**
 * Verifies the string representation for logging.
 */
void test_ts_get_state_name(void) {
    TEST_ASSERT_EQUAL_STRING("OFF", TS_get_state_name(TS_STATUS_OFF));
    TEST_ASSERT_EQUAL_STRING("FATAL", TS_get_state_name(TS_STATUS_FATAL));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", TS_get_state_name(99));
}

/**
 * Verifies that clearing requests works independently of state.
 */
void test_ts_clear_requests(void) {
    TS_set_status(TS_STATUS_OFF);
    TS_request_power_on();
    TEST_ASSERT_TRUE(TS_is_power_on_requested());

    TS_clear_request();
    TEST_ASSERT_FALSE(TS_is_power_on_requested());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ts_init_state);
    RUN_TEST(test_ts_request_on_flow);
    RUN_TEST(test_ts_request_on_guard);
    RUN_TEST(test_ts_request_off_from_anywhere);
    RUN_TEST(test_ts_get_state_name);
    RUN_TEST(test_ts_clear_requests);
    return UNITY_END();
}