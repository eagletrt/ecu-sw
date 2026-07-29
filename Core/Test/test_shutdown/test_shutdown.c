#include "shutdown-api.h"
#include <unity.h>
#include "fff.h"

DEFINE_FFF_GLOBALS;

extern struct ShutdownHandler shutdown_handler;

FAKE_VALUE_FUNC(enum ShutdownReturnCode, shutdown_set_state, enum ShutdownState);

void setUp(void) {
    RESET_FAKE(shutdown_set_state);
    FFF_RESET_HISTORY();
    shutdown_api_init(shutdown_set_state);
}

void test_shutdown_api_init(void) {
    memset(&shutdown_handler, 0, sizeof(shutdown_handler));
    enum ShutdownReturnCode rc = shutdown_api_init(shutdown_set_state);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_OK, rc, "shutdown_api_init should return SHUTDOWN_RC_OK");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(shutdown_set_state, shutdown_handler.set_state, "set_state callback should be set correctly");
}

void test_shutdown_api_set_state_successful(void) {
    shutdown_set_state_fake.return_val = SHUTDOWN_RC_OK;
    enum ShutdownReturnCode rc = shutdown_api_set_state(SHUTDOWN_STATE_CLOSED);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_OK, rc, "shutdown_api_set_state should return SHUTDOWN_RC_OK when callback succeeds");
    TEST_ASSERT_EQUAL_MESSAGE(1, shutdown_set_state_fake.call_count, "shutdown_set_state should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_CLOSED, shutdown_set_state_fake.arg0_val, "shutdown_set_state should be called with the correct state");
}

void test_shutdown_api_set_state_invalid_state(void) {
    enum ShutdownReturnCode rc = shutdown_api_set_state(SHUTDOWN_STATE_COUNT); // Invalid state
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_ERROR, rc, "shutdown_api_set_state should return SHUTDOWN_RC_ERROR for invalid state");
    TEST_ASSERT_EQUAL_MESSAGE(0, shutdown_set_state_fake.call_count, "shutdown_set_state should not be called for invalid state");
}

void test_shutdown_api_set_state_null_callback(void) {
    shutdown_handler.set_state = NULL; // Simulate uninitialized callback
    enum ShutdownReturnCode rc = shutdown_api_set_state(SHUTDOWN_STATE_CLOSED);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_ERROR, rc, "shutdown_api_set_state should return SHUTDOWN_RC_ERROR when callback is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(0, shutdown_set_state_fake.call_count, "shutdown_set_state should not be called when callback is NULL");
}

void test_shutdown_api_set_reading_state_successful(void) {
    enum ShutdownReturnCode rc = shutdown_api_set_reding_state(SHUTDOWN_READING_BEFORE_ECU, SHUTDOWN_STATE_CLOSED);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_OK, rc, "shutdown_api_set_reding_state should return SHUTDOWN_RC_OK for valid reading");
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_CLOSED, shutdown_handler.state_before, "state_before should be set correctly");
}

void test_shutdown_api_set_reading_state_invalid_state(void) {
    enum ShutdownReturnCode rc = shutdown_api_set_reding_state(SHUTDOWN_READING_BEFORE_ECU, SHUTDOWN_STATE_COUNT); // Invalid state
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_ERROR, rc, "shutdown_api_set_reding_state should return SHUTDOWN_RC_ERROR for invalid state");
}

void test_shutdown_api_set_reading_state_invalid_reading(void) {
    enum ShutdownReturnCode rc = shutdown_api_set_reding_state(SHUTDOWN_READING_COUNT, SHUTDOWN_STATE_CLOSED); // Invalid reading
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_RC_ERROR, rc, "shutdown_api_set_reding_state should return SHUTDOWN_RC_ERROR for invalid reading");
}

void shutdown_api_get_reading_state_successful(void) {
    shutdown_handler.state_before = SHUTDOWN_STATE_CLOSED;
    shutdown_handler.state_after = SHUTDOWN_STATE_OPEN;

    enum ShutdownState state_before = shutdown_api_get_reading_state(SHUTDOWN_READING_BEFORE_ECU);
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_CLOSED, state_before, "shutdown_api_get_reading_state should return correct state for BEFORE_ECU");
}

void shutdown_api_get_reading_state_invalid_reading(void) {
    enum ShutdownState state = shutdown_api_get_reading_state(SHUTDOWN_READING_COUNT); // Invalid reading
    TEST_ASSERT_EQUAL_MESSAGE(SHUTDOWN_STATE_OPEN, state, "shutdown_api_get_reading_state should return SHUTDOWN_STATE_OPEN for invalid reading");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_shutdown_api_init);

    RUN_TEST(test_shutdown_api_set_state_successful);
    RUN_TEST(test_shutdown_api_set_state_invalid_state);
    RUN_TEST(test_shutdown_api_set_state_null_callback);

    RUN_TEST(test_shutdown_api_set_reading_state_successful);
    RUN_TEST(test_shutdown_api_set_reading_state_invalid_state);
    RUN_TEST(test_shutdown_api_set_reading_state_invalid_reading);

    RUN_TEST(shutdown_api_get_reading_state_successful);
    RUN_TEST(shutdown_api_get_reading_state_invalid_reading);

    return UNITY_END();
}
