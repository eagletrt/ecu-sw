#include <unity.h>
#include "tractive-system-api.h"

/* Mocking State */
static int mock_on_cmd_count = 0;
static int mock_off_cmd_count = 0;
static enum TSReturnCode mock_return = TS_RC_OK;
static enum TsStatus ts_status = TS_STATUS_UNKNOWN;

/* Mock implementations */
enum TSReturnCode mock_send_command(enum TSCommand cmd) {
    if (cmd == TS_COMMAND_ON)
        mock_on_cmd_count++;
    if (cmd == TS_COMMAND_OFF)
        mock_off_cmd_count++;
    return mock_return;
}

enum TsStatus mock_get_status() {
    return ts_status;
}

// ------------------------------------------

void setUp(void) {
    mock_on_cmd_count = 0;
    mock_off_cmd_count = 0;
    mock_return = TS_RC_OK;
    ts_status = TS_STATUS_UNKNOWN;
}

void tearDown(void) {
    ts_api_reset();
}

/* --- Tests --- */

/*!
 * \brief Verify that the initialization is successfull only if all callbacks are initialized
 */
void test_ts_init(void) {
    enum TSReturnCode rc;

    rc = ts_api_init(NULL, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, rc, "Initialization shouild fail when no callbacks are passed");

    rc = ts_api_init(mock_send_command, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, rc, "Initialization shouild fail when 'get status' callback is NULL");

    rc = ts_api_init(NULL, mock_get_status);
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, rc, "Initialization shouild fail when 'send command' callback is NULL");

    rc = ts_api_init(mock_send_command, mock_get_status);
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_OK, rc, "Initialization shouild be successfull is all callbacks are passed");
}

/*!
 * \brief Verify that a command can be sent correctly only if the callback is initialized and that
 * the return codes are correctly forwarded.
 */
void test_ts_request(void) {
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, ts_api_request_command(TS_COMMAND_OFF), "If the 'send command' callback is missing, an error should be returned");

    ts_api_init(mock_send_command, mock_get_status);
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_OK, ts_api_request_command(TS_COMMAND_ON), "A correct initialization should allow a correct sending of a command");

    //simulate a callback error
    mock_return = TS_RC_ERROR;
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, ts_api_request_command(TS_COMMAND_ON), "In case the callback fails, the return code should be forwarded to the caller");
}

/*!
 * \brief Verify that the TS status can be retrieved only if the callback is initialized.
 */
void test_ts_get_status(void) {
    TEST_ASSERT_EQUAL_MESSAGE(TS_STATUS_UNKNOWN, ts_api_get_status(), "If the 'get status' callback is missing, an unknown status should be returned");

    ts_api_init(mock_send_command, mock_get_status);
    ts_status = TS_STATUS_ON;
    TEST_ASSERT_EQUAL_MESSAGE(TS_STATUS_ON, ts_api_get_status(), "The status shouild correspond to the actual status of the TS");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ts_init);
    RUN_TEST(test_ts_request);
    RUN_TEST(test_ts_get_status);
    return UNITY_END();
}