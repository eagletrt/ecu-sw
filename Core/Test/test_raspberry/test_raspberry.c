/**
 * \file test_raspberry.c
 * \author Dorijan Di Zepp
 * \date 2026-05-03
 * \brief Unit tests using FFF for testing the raspberry module
 */

#include <unity.h>
#include <stdbool.h>
#include "raspberry-api.h"
#include "fff.h"
#include "eagletrt-api.h"

extern struct RaspberryHandler raspberry_handler;

DEFINE_FFF_GLOBALS;

// mocks for raspberry
FAKE_VALUE_FUNC(enum RaspberryReturnCode, pin_control, enum RaspberryControlPinState);

void setUp(void) {
    // initialize raspberry module
    rasp_api_init(pin_control, RASPBERRY_CONTROL_PIN_STATE_ON);

    // reset mock state
    RESET_FAKE(pin_control);

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup rasp_api_init Tests for rasp_api_init function
 * \{
 */

void test_rasp_api_init_null_pin_control(void) {
    enum RaspberryReturnCode rc = rasp_api_init(NULL, RASPBERRY_CONTROL_PIN_STATE_ON);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_ERROR, rc, "Init should fail if pin_control is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_CONTROL_PIN_STATE_UNKNOWN, raspberry_handler.current_pin_state, "The current pin state is unknown as it is not possible to set an initial state through the callback");
}

void test_rasp_api_init_invalid_control_pin_state(void) {
    enum RaspberryReturnCode rc = rasp_api_init(pin_control, -99);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_ERROR, rc, "Init should fail if initial state type is invalid");
    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_CONTROL_PIN_STATE_UNKNOWN, raspberry_handler.current_pin_state, "The current pin state is unknown as the user defined state is not valid");
}

void test_rasp_api_init_pin_control_callback_error(void) {
    pin_control_fake.return_val = RASPBERRY_RC_ERROR;
    enum RaspberryReturnCode rc = rasp_api_init(pin_control, RASPBERRY_CONTROL_PIN_STATE_OFF);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_ERROR, rc, "Init should fail if callback returns an error");
    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_CONTROL_PIN_STATE_UNKNOWN, raspberry_handler.current_pin_state, "The current pin state is unknown as the callback failed");
}

void test_rasp_api_init_pin_control_off(void) {
    enum RaspberryReturnCode rc = rasp_api_init(pin_control, RASPBERRY_CONTROL_PIN_STATE_OFF);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_OK, rc, "Init should return OK if callback changed pin status");
    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_CONTROL_PIN_STATE_OFF, raspberry_handler.current_pin_state, "The current pin state should be equal to the one passed during initialiation");
}

void test_rasp_api_init_pin_control_on(void) {
    enum RaspberryReturnCode rc = rasp_api_init(pin_control, RASPBERRY_CONTROL_PIN_STATE_ON);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_OK, rc, "Init should return OK if callback changed pin status");
    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_CONTROL_PIN_STATE_ON, raspberry_handler.current_pin_state, "The current pin state should be equal to the one passed during initialiation");
}
/*! \} */

/*!
 * \defgroup rasp_api_change_pin_state Tests for rasp_api_change_pin_state function
 * \{
 */

void test_rasp_api_change_pin_state_invalid_control_pin_state(void) {
    enum RaspberryReturnCode rc = rasp_api_change_pin_state(99U);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_ERROR, rc, "Change pin should fail as the state passed is not valid");
}

void test_rasp_api_change_pin_state_unchanged_state(void) {
    enum RaspberryControlPinState current_state = RASPBERRY_CONTROL_PIN_STATE_OFF;
    raspberry_handler.current_pin_state = current_state;
    rasp_api_change_pin_state(99U);

    TEST_ASSERT_EQUAL_MESSAGE(current_state, raspberry_handler.current_pin_state, "If the state passed is not valid, teh current pin state should be left unchanged");
}

void test_rasp_api_change_pin_state_pin_controll_callback_error(void) {
    pin_control_fake.return_val = RASPBERRY_RC_ERROR;
    enum RaspberryReturnCode rc = rasp_api_change_pin_state(RASPBERRY_CONTROL_PIN_STATE_ON);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_ERROR, rc, "Change pin should fail if the callback fails to change the pin state");
    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_CONTROL_PIN_STATE_UNKNOWN, raspberry_handler.current_pin_state, "The current pin state is unknown as the callback failed");
}

void test_rasp_api_change_pin_state_success(void) {
    enum RaspberryControlPinState current_state = RASPBERRY_CONTROL_PIN_STATE_ON;
    enum RaspberryControlPinState expected_state = RASPBERRY_CONTROL_PIN_STATE_OFF;

    rasp_api_init(pin_control, current_state);
    enum RaspberryReturnCode rc = rasp_api_change_pin_state(expected_state);

    TEST_ASSERT_EQUAL_MESSAGE(RASPBERRY_RC_OK, rc, "An OK code should be returned if the state passed is valid and the callback didn't fail");
    TEST_ASSERT_EQUAL_MESSAGE(expected_state, raspberry_handler.current_pin_state, "If the function is successful, the pin state should be updated");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup rasp_api_init
     * \{
     */
    RUN_TEST(test_rasp_api_init_null_pin_control);
    RUN_TEST(test_rasp_api_init_invalid_control_pin_state);
    RUN_TEST(test_rasp_api_init_pin_control_callback_error);
    RUN_TEST(test_rasp_api_init_pin_control_off);
    RUN_TEST(test_rasp_api_init_pin_control_on);
    /*! \} */

    /*!
     * \addtogroup rasp_api_change_pin_state
     * \{
     */
    RUN_TEST(test_rasp_api_change_pin_state_invalid_control_pin_state);
    RUN_TEST(test_rasp_api_change_pin_state_unchanged_state);
    RUN_TEST(test_rasp_api_change_pin_state_pin_controll_callback_error);
    RUN_TEST(test_rasp_api_change_pin_state_success);
    /*! \} */

    return UNITY_END();
}