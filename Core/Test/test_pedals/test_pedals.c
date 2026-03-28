/**
 * \file test_pedals.c
 * \author Dorijan Di Zepp
 * \date 2026-03-28
 * \brief Unit tests for the pedals module
 * \note Even if the module itself is pretty simple, it is important to make sure
 * that the handler correctly updates the internal state within defined physical limits (e.g. max pressure),
 * out-of-range values are rejected adn the last valid state is preserved.
 */

#include <unity.h>
#include <stdbool.h>
#include "pedals-api.h"
#include "eagletrt-api.h"

void setUp(void) {
    pedals_api_init();
}

/* --- Test Cases --- */

/*!
 * \defgroup pedals_api_init Tests for pedals_api_init function
 * \{
 */

void test_pedals_api_init_initial_state(void) {
    struct PedalsHandler expected_handler = { 0 };
    extern struct PedalsHandler pedals_handler;

    enum PedalsReturnCode rc = pedals_api_init();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "Return code do not match. It should return OK");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_handler, &pedals_handler, sizeof(expected_handler), "Structure handler data do not match");
}

/*! \} */

/*!
 * \defgroup pedals_api_set_throttle Tests for pedals_api_set_throttle function
 * \{
 */

void test_pedals_api_set_throttle_in_range(void) {
    float expected_throttle = 0.4f;

    enum PedalsReturnCode rc = pedals_api_set_throttle(expected_throttle);
    float throttle = pedals_api_get_throttle();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set a new throttle value if in range");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_throttle, throttle, "The latest throttle value stored should correspond to the latest value passed");
}

void test_pedals_api_set_throttle_lower_range(void) {
    float expected_throttle = 0.0f;

    enum PedalsReturnCode rc = pedals_api_set_throttle(expected_throttle);
    float throttle = pedals_api_get_throttle();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the throttle to zero");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_throttle, throttle, "The latest throttle value stored should correspond to the latest value passed");
}

void test_pedals_api_set_throttle_lower_out_of_range(void) {
    // as tested for the "after-initialization" values, the current value is zero
    float expected_throttle = 0.0f;
    float under_limit_throttle = -0.1f;

    enum PedalsReturnCode rc = pedals_api_set_throttle(under_limit_throttle);
    float throttle = pedals_api_get_throttle();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the throttle below zero");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_throttle, throttle, "The latest throttle value stored should correspond to the last valid value");
}

void test_pedals_api_set_throttle_upper_range(void) {
    float expected_throttle = 1.0f;

    enum PedalsReturnCode rc = pedals_api_set_throttle(expected_throttle);
    float throttle = pedals_api_get_throttle();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the throttle to one");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_throttle, throttle, "The latest throttle value stored should correspond to the latest value passed");
}

void test_pedals_api_set_throttle_upper_out_of_range(void) {
    // as tested for the "after-initialization" values, the current value is zero
    float expected_throttle = 0.0f;
    float over_limit_throttle = 1.1f;

    enum PedalsReturnCode rc = pedals_api_set_throttle(over_limit_throttle);
    float throttle = pedals_api_get_throttle();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the throttle above one");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_throttle, throttle, "The latest throttle value stored should correspond to the last valid value");
}

/*! \} */

/*!
 * \defgroup pedals_api_set_brake Tests for pedals_api_set_brake function
 * \{
 */

void test_pedals_api_set_brake_in_range(void) {
    float expected_brake = 0.4f;

    enum PedalsReturnCode rc = pedals_api_set_brake(expected_brake);
    float brake = pedals_api_get_brake();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set a new brake value if in range");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake, brake, "The latest brale value stored should correspond to the latest value passed");
}

void test_pedals_api_set_brake_lower_range(void) {
    float expected_brake = 0.0f;

    enum PedalsReturnCode rc = pedals_api_set_brake(expected_brake);
    float brake = pedals_api_get_brake();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake to zero");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake, brake, "The latest brake value stored should correspond to the latest value passed");
}

void test_pedals_api_set_brake_lower_out_of_range(void) {
    // as tested for the "after-initialization" values, the current value is zero
    float expected_brake = 0.0f;
    float under_limit_brake = -0.1f;

    enum PedalsReturnCode rc = pedals_api_set_brake(under_limit_brake);
    float brake = pedals_api_get_brake();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the brake below zero");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake, brake, "The latest brake value stored should correspond to the last valid value");
}

void test_pedals_api_set_brake_upper_range(void) {
    float expected_brake = 1.0f;

    enum PedalsReturnCode rc = pedals_api_set_brake(expected_brake);
    float brake = pedals_api_get_brake();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake to one");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake, brake, "The latest brake value stored should correspond to the latest value passed");
}

void test_pedals_api_set_brake_upper_out_of_range(void) {
    // as tested for the "after-initialization" values, the current value is zero
    float expected_brake = 0.0f;
    float over_limit_brake = 1.1f;

    enum PedalsReturnCode rc = pedals_api_set_brake(over_limit_brake);
    float brake = pedals_api_get_brake();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the brake above pne");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake, brake, "The latest brake value stored should correspond to the last valid value");
}

/*! \} */

/*!
 * \defgroup pedals_api_set_brake_pressure Tests for pedals_api_set_brake_pressure function
 * \{
 */

void test_pedals_api_set_brake_pressure_in_range(void) {
    float expected_brake_pressure = PEDALS_BRAKE_THRESHOLD_PERCENTAGE / 2;

    enum PedalsReturnCode rc = pedals_api_set_brake_pressure(expected_brake_pressure);
    float brake_pressure = pedals_api_get_brake_pressure();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set a new brake pressure value if in range");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake_pressure, brake_pressure, "The latest brake pressure value stored should correspond to the latest value passed");
}

void test_pedals_api_set_brake_pressure_lower_range(void) {
    float expected_brake_pressure = 0.0f;

    enum PedalsReturnCode rc = pedals_api_set_brake_pressure(expected_brake_pressure);
    float brake_pressure = pedals_api_get_brake_pressure();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake pressure to zero");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake_pressure, brake_pressure, "The latest brake pressure value stored should correspond to the latest value passed");
}

void test_pedals_api_set_brake_pressure_lower_out_of_range(void) {
    // as tested for the "after-initialization" values, the current value is zero
    float expected_brake_pressure = 0.0f;
    float under_limit_brake_pressure = -0.1f;

    enum PedalsReturnCode rc = pedals_api_set_brake_pressure(under_limit_brake_pressure);
    float brake_pressure = pedals_api_get_brake_pressure();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the brake pressure below zero");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake_pressure, brake_pressure, "The latest brake pressure value stored should correspond to the last valid value");
}

void test_pedals_api_set_brake_pressure_upper_range(void) {
    float expected_brake_pressure = PEDALS_MAX_BRAKE_PRESSURE_BAR;

    enum PedalsReturnCode rc = pedals_api_set_brake_pressure(expected_brake_pressure);
    float brake_pressure = pedals_api_get_brake_pressure();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake pressure to PEDALS_MAX_BRAKE_PRESSURE_BAR");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake_pressure, brake_pressure, "The latest brake pressure value stored should correspond to the latest value passed");
}

void test_pedals_api_set_brake_pressure_upper_out_of_range(void) {
    // as tested for the "after-initialization" values, the current value is zero
    float expected_brake_pressure = 0.0f;
    float over_limit_brake_pressure = PEDALS_MAX_BRAKE_PRESSURE_BAR + 0.1f;

    enum PedalsReturnCode rc = pedals_api_set_brake_pressure(over_limit_brake_pressure);
    float brake_pressure = pedals_api_get_brake_pressure();

    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the brake pressure above PEDALS_MAX_BRAKE_PRESSURE_BAR");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake_pressure, brake_pressure, "The latest brake pressure value stored should correspond to the last valid value");
}

/*! \} */

/*!
 * \defgroup pedals_api_getters Tests for pedals_api_get_throttle, pedals_api_get_brake
    and pedals_api_get_brake_pressure functions
 * \{
 */

void test_pedals_api_get_throttle(void) {
    float expected_throttle = 0.75f;
    pedals_api_set_throttle(expected_throttle);

    float throttle = pedals_api_get_throttle();

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_throttle, throttle, "Getter did not return the expected throttle value");
}

void test_pedals_api_get_brake(void) {
    float expected_brake = 0.75f;
    pedals_api_set_brake(expected_brake);

    float brake = pedals_api_get_brake();

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake, brake, "Getter did not return the expected brake value");
}

void test_pedals_api_get_brake_pressure(void) {
    float expected_brake_pressure = PEDALS_MAX_BRAKE_PRESSURE_BAR;
    pedals_api_set_brake_pressure(expected_brake_pressure);

    float brake_pressure = pedals_api_get_brake_pressure();

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected_brake_pressure, brake_pressure, "Getter did not return the expected brake pressure value");
}

/*! \} */

/*!
 * \defgroup pedals_api_get_requested_throttle_torque Tests for pedals_api_get_requested_throttle_torque function
 * \{
 */

void test_pedals_api_get_requested_throttle_torque_after_init(void) {
    // after init, throttle is set to 0.0f and so the requested torque
    float throttle_torque = pedals_api_get_requested_throttle_torque();

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, throttle_torque, "Requested throttle torque should be zero if current throttle percentage is zero");
}

void test_pedals_api_get_requested_throttle_torque_max_value(void) {
    // the computation is a simple product [PEDALS_MAX_TORQUE_NM * throttle]
    // by setting the throttle percentage to the max value, we expect the max torque available
    pedals_api_set_throttle(1.0f);
    float throttle_torque = pedals_api_get_requested_throttle_torque();

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(PEDALS_MAX_TORQUE_NM, throttle_torque, "Requested throttle torque should be the max available if the throttle percentage is 1.0f");
}

/*! \} */

/*!
 * \defgroup pedals_api_is_brake_pressed Tests for pedals_api_is_brake_pressed function
 * \{
 */

void test_pedals_api_is_brake_pressed_under_threshold(void) {
    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE / 2);

    bool brake_pressed = pedals_api_is_brake_pressed();

    TEST_ASSERT_FALSE_MESSAGE(brake_pressed, "Brake should not be pressed if current brake percentage is under threshold");
}

void test_pedals_api_is_brake_pressed_same_as_threshold(void) {
    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE);

    bool brake_pressed = pedals_api_is_brake_pressed();

    TEST_ASSERT_TRUE_MESSAGE(brake_pressed, "Brake should be pressed if current brake percentage is equal threshold");
}

void test_pedals_api_is_brake_pressed_above_threshold(void) {
    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE + 0.1f);

    bool brake_pressed = pedals_api_is_brake_pressed();

    TEST_ASSERT_TRUE_MESSAGE(brake_pressed, "Brake should be pressed if current brake percentage is over threshold");
}

/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup pedals_api_init
     * \{
     */
    RUN_TEST(test_pedals_api_init_initial_state);
    /*! \} */

    /*!
     * \addtogroup pedals_api_set_throttle
     * \{
     */
    RUN_TEST(test_pedals_api_set_throttle_in_range);
    RUN_TEST(test_pedals_api_set_throttle_lower_range);
    RUN_TEST(test_pedals_api_set_throttle_lower_out_of_range);
    RUN_TEST(test_pedals_api_set_throttle_upper_range);
    RUN_TEST(test_pedals_api_set_throttle_upper_out_of_range);
    /*! \} */

    /*!
     * \addtogroup pedals_api_set_brake
     * \{
     */
    RUN_TEST(test_pedals_api_set_brake_in_range);
    RUN_TEST(test_pedals_api_set_brake_lower_range);
    RUN_TEST(test_pedals_api_set_brake_lower_out_of_range);
    RUN_TEST(test_pedals_api_set_brake_upper_range);
    RUN_TEST(test_pedals_api_set_brake_upper_out_of_range);
    /*! \} */

    /*!
     * \addtogroup pedals_api_set_brake_pressure
     * \{
     */
    RUN_TEST(test_pedals_api_set_brake_pressure_in_range);
    RUN_TEST(test_pedals_api_set_brake_pressure_lower_range);
    RUN_TEST(test_pedals_api_set_brake_pressure_lower_out_of_range);
    RUN_TEST(test_pedals_api_set_brake_pressure_upper_range);
    RUN_TEST(test_pedals_api_set_brake_pressure_upper_out_of_range);
    /*! \} */

    /*!
     * \addtogroup pedals_api_getters
     * \{
     */
    RUN_TEST(test_pedals_api_get_throttle);
    RUN_TEST(test_pedals_api_get_brake);
    RUN_TEST(test_pedals_api_get_brake_pressure);
    /*! \} */

    /*!
     * \addtogroup pedals_api_get_requested_throttle_torque
     * \{
     */
    RUN_TEST(test_pedals_api_get_requested_throttle_torque_after_init);
    RUN_TEST(test_pedals_api_get_requested_throttle_torque_max_value);
    /*! \} */

    /*!
     * \addtogroup pedals_api_is_brake_pressed
     * \{
     */
    RUN_TEST(test_pedals_api_is_brake_pressed_under_threshold);
    RUN_TEST(test_pedals_api_is_brake_pressed_same_as_threshold);
    RUN_TEST(test_pedals_api_is_brake_pressed_above_threshold);
    /*! \} */

    return UNITY_END();
}