/*!
 * \file test_driver.c
 * \author Dorijan Di Zepp
 * \date 2026-05-14
 * \brief Unit tests using FFF for testing the driver module.
 */

#include <unity.h>
#include "ecu_fsm.h"
#include "driver-api.h"
#include "fff.h"
#include "eagletrt-api.h"

extern struct DriverHandler driver_handler;

/*!
 * \note The \c current_state variable is defined in \ref support_external.c.
 * We use \c extern here to access that global definition during tests.
 */
extern state_t current_state;

DEFINE_FFF_GLOBALS;

// Mocks for driver module
FAKE_VALUE_FUNC(enum DriverReturnCode, wait_for_driver);
FAKE_VALUE_FUNC(enum DriverReturnCode, continuous_check);

void setUp(void) {
    // Reset the global state before each test
    current_state = STATE_INIT;

    // Initialize driver module
    driver_api_init(DRIVER_TYPE_MANUAL, wait_for_driver, continuous_check);

    // Reset mock state
    RESET_FAKE(wait_for_driver);
    RESET_FAKE(continuous_check);

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup driver_api_init Tests for driver_api_init function
 * \{
 */

void test_driver_api_init_invalid_driver_type(void) {
    enum DriverReturnCode rc = driver_api_init(99U, wait_for_driver, continuous_check);

    TEST_ASSERT_EQUAL_MESSAGE(DRIVER_RC_ERROR, rc, "Init should fail if the driver type passed is not valid");
}

void test_driver_api_init_null_wait_for_driver_callback(void) {
    enum DriverReturnCode rc = driver_api_init(DRIVER_TYPE_MANUAL, NULL, continuous_check);

    TEST_ASSERT_EQUAL_MESSAGE(DRIVER_RC_ERROR, rc, "Init should fail if wait for driver callback is NULL");
}

void test_driver_api_init_null_continuous_check_callback(void) {
    enum DriverReturnCode rc = driver_api_init(DRIVER_TYPE_MANUAL, wait_for_driver, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(DRIVER_RC_ERROR, rc, "Init should fail if the continuous check callback is NULL");
}

void test_driver_api_init_not_in_init_and_idle(void) {
    // set the current FSM state to any state different from
    // both init and idle
    current_state = STATE_R2D;

    enum DriverReturnCode rc = driver_api_init(DRIVER_TYPE_MANUAL, wait_for_driver, continuous_check);

    TEST_ASSERT_EQUAL_MESSAGE(DRIVER_RC_ERROR, rc, "Init should fail current FSM state is neither Init and Idle");
}

void test_driver_api_init_on_init_state(void) {
    // a valid state is init to allow module initialization
    current_state = STATE_INIT;

    enum DriverReturnCode rc = driver_api_init(DRIVER_TYPE_MANUAL, wait_for_driver, continuous_check);

    TEST_ASSERT_EQUAL_MESSAGE(DRIVER_RC_OK, rc, "Init should complete if all parameters are passed and current state is a valid one");
}

void test_driver_api_init_on_idle_state(void) {
    // a valid state is idle to allow module initialization
    current_state = STATE_IDLE;

    enum DriverReturnCode rc = driver_api_init(DRIVER_TYPE_MANUAL, wait_for_driver, continuous_check);

    TEST_ASSERT_EQUAL_MESSAGE(DRIVER_RC_OK, rc, "Init should complete if all parameters are passed and current state is a valid one");
}
/*! \} */

/*!
 * \defgroup driver_api_is_driver_ready Tests for driver_api_is_driver_ready function
 * \{
 */

void test_driver_api_is_driver_ready_driver_not_ready(void) {
    // Simply verify that if the driver readiness check fails,
    // the function should also fail.
    enum DriverReturnCode expected_rc = DRIVER_RC_ERROR;
    wait_for_driver_fake.return_val = expected_rc;

    enum DriverReturnCode rc = driver_api_is_driver_ready();

    TEST_ASSERT_EQUAL_MESSAGE(expected_rc, rc, "If the driver readiness check fails, the API should also fail");
}

void test_driver_api_is_driver_ready_driver_ready(void) {
    // Simply verify that if the driver readiness check completes,
    // the function should also complete.
    enum DriverReturnCode expected_rc = DRIVER_RC_OK;
    wait_for_driver_fake.return_val = expected_rc;

    enum DriverReturnCode rc = driver_api_is_driver_ready();

    TEST_ASSERT_EQUAL_MESSAGE(expected_rc, rc, "If the driver readiness check is successful, the API should also be successful");
}
/*! \} */

/*!
 * \defgroup driver_api_continuous_check Tests for driver_api_continuous_check function
 * \{
 */

void test_driver_api_continuous_check_fails(void) {
    // Simply verify that if the continuous check fails,
    // the function should also fail.
    enum DriverReturnCode expected_rc = DRIVER_RC_ERROR;
    continuous_check_fake.return_val = expected_rc;

    enum DriverReturnCode rc = driver_api_continuous_check();

    TEST_ASSERT_EQUAL_MESSAGE(expected_rc, rc, "If the continuous check fails, the API should also fail");
}

void test_driver_api_continuous_check_completes(void) {
    // Simply verify that if the continuous check fails,
    // the function should also fail.
    enum DriverReturnCode expected_rc = DRIVER_RC_OK;
    continuous_check_fake.return_val = expected_rc;

    enum DriverReturnCode rc = driver_api_continuous_check();

    TEST_ASSERT_EQUAL_MESSAGE(expected_rc, rc, "If the continuous check is successful, the API should also be successful");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup driver_api_init
     * \{
     */
    RUN_TEST(test_driver_api_init_invalid_driver_type);
    RUN_TEST(test_driver_api_init_null_wait_for_driver_callback);
    RUN_TEST(test_driver_api_init_null_continuous_check_callback);
    RUN_TEST(test_driver_api_init_not_in_init_and_idle);
    RUN_TEST(test_driver_api_init_on_init_state);
    RUN_TEST(test_driver_api_init_on_idle_state);
    /*! \} */

    /*!
     * \addtogroup driver_api_is_driver_ready
     * \{
     */
    RUN_TEST(test_driver_api_is_driver_ready_driver_not_ready);
    RUN_TEST(test_driver_api_is_driver_ready_driver_ready);
    /*! \} */

    /*!
     * \addtogroup driver_api_continuous_check
     * \{
     */
    RUN_TEST(test_driver_api_continuous_check_fails);
    RUN_TEST(test_driver_api_continuous_check_completes);
    /*! \} */

    return UNITY_END();
}