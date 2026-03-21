/**
 * \file test_pedals.c
 * \author Dorijan Di Zepp
 * \date 2026-03-21
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

void tearDown(void) {
}

/* --- Test Cases --- */

/*!
 * \defgroup pedals_api_init Tests for pedals_api_init function
 * \{
 */

void test_pedals_api_init_initial_state(void) {
    float throttle, brake, brake_pressure;

    pedals_api_get_throttle(&throttle);
    pedals_api_get_brake(&brake);
    pedals_api_get_brake_pressure(&brake_pressure);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, throttle, "Throttle should be set to zero after initialization");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, brake, "Brake should be set to zero after initialization");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, brake_pressure, "Brake pressure should be set to zero after initialization");
}

/*! \} */

/*!
 * \defgroup pedals_api_set_throttle Tests for pedals_api_set_throttle function
 * \{
 */

void test_pedals_api_set_throttle_in_range(void) {
    enum PedalsReturnCode rc;

    rc = pedals_api_set_throttle(0.4f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set a new throttle value if in range");

    rc = pedals_api_set_throttle(0.0f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the throttle to 0.0f");

    rc = pedals_api_set_throttle(1.0f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the throttle to 1.0f");
}

void test_pedals_api_set_throttle_out_range(void) {
    enum PedalsReturnCode rc;

    rc = pedals_api_set_throttle(1.1f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the throttle with a value greater than 1.0f");

    rc = pedals_api_set_throttle(-0.1f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the throttle to a negative value");
}

/*! \} */

/*!
 * \defgroup pedals_api_set_brake Tests for pedals_api_set_brake function
 * \{
 */

void test_pedals_api_set_brake_in_range(void) {
    enum PedalsReturnCode rc;

    rc = pedals_api_set_brake(0.4f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set a new brake value if in range");

    rc = pedals_api_set_brake(0.0f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake to 0.0f");

    rc = pedals_api_set_brake(1.0f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake to 1.0f");
}

void test_pedals_api_set_brake_out_range(void) {
    enum PedalsReturnCode rc;

    rc = pedals_api_set_brake(1.1f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the brake with a value greater than 1.0f");

    rc = pedals_api_set_brake(-0.1f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the brake to a negative value");
}

/*! \} */

/*!
 * \defgroup pedals_api_set_brake_pressure Tests for pedals_api_set_brake_pressure function
 * \{
 */

void test_pedals_api_set_brake_pressure_in_range(void) {
    enum PedalsReturnCode rc;

    rc = pedals_api_set_brake_pressure((PEDALS_BRAKE_THRESHOLD_PERCENTAGE / 2));
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set a new brake pressure value if in range");

    rc = pedals_api_set_brake_pressure(0.0f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake pressure to 0.0f");

    rc = pedals_api_set_brake_pressure(PEDALS_MAX_BRAKE_PRESSURE_BAR);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "It should be possible to set the brake pressure to the max value");
}

void test_pedals_api_set_brake_pressure_out_range(void) {
    enum PedalsReturnCode rc;

    rc = pedals_api_set_brake_pressure(PEDALS_MAX_BRAKE_PRESSURE_BAR + 1.5f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to exceed the maximum brake pressure");

    rc = pedals_api_set_brake_pressure(-0.1f);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "It should not be possible to set the brake pressure to a negative value");
}

/*! \} */

/*!
 * \defgroup pedals_api_getters Tests for pedals_api_get_throttle, pedals_api_get_brake
    and pedals_api_get_brake_pressure functions
 * \{
 */

void test_pedals_api_get_throttle(void) {
    float out;
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, pedals_api_get_throttle(NULL), "Getter should return error when passed a NULL pointer");

    pedals_api_set_throttle(0.75f);
    enum PedalsReturnCode rc = pedals_api_get_throttle(&out);

    TEST_ASSERT_EQUAL(PEDALS_RC_OK, rc);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.75f, out, "Getter did not return the expected throttle value");
}

void test_pedals_api_get_brake(void) {
    float out;
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, pedals_api_get_brake(NULL), "Getter should return error when passed a NULL pointer");

    pedals_api_set_brake(0.75f);
    enum PedalsReturnCode rc = pedals_api_get_brake(&out);

    TEST_ASSERT_EQUAL(PEDALS_RC_OK, rc);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.75f, out, "Getter did not return the expected brake value");
}

void test_pedals_api_get_brake_pressure(void) {
    float out;
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, pedals_api_get_brake_pressure(NULL), "Getter should return error when passed a NULL pointer");

    pedals_api_set_brake_pressure(PEDALS_MAX_BRAKE_PRESSURE_BAR);
    enum PedalsReturnCode rc = pedals_api_get_brake_pressure(&out);

    TEST_ASSERT_EQUAL(PEDALS_RC_OK, rc);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(PEDALS_MAX_BRAKE_PRESSURE_BAR, out, "Getter did not return the expected brake pressure value");
}

/*! \} */

/*!
 * \defgroup pedals_api_get_requested_throttle_torque Tests for pedals_api_get_requested_throttle_torque function
 * \{
 */

void test_pedals_api_get_requested_throttle_torque_null_pointer(void) {
    enum PedalsReturnCode rc = pedals_api_get_requested_throttle_torque(NULL);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "Getter should return an error if the output pointer is NULL.");
}

void test_pedals_api_get_requested_throttle_torque_return_value(void) {
    float out;
    enum PedalsReturnCode rc = pedals_api_get_requested_throttle_torque(&out);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "Getter should return correctly if the output pointer is not NULL.");
    // after init, throttle is set to 0.0f and so the requested torque
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, out, "Requested throttle torque should be zero if current throttle percentage is zero");

    // the computation is a simple product [PEDALS_MAX_TORQUE_NM * throttle]
    // by setting the throttle percentage to the max value, we expect the max torque available
    pedals_api_set_throttle(1.0f);
    pedals_api_get_requested_throttle_torque(&out);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(PEDALS_MAX_TORQUE_NM, out, "Requested throttle torque should be the max available if the throttle percentage is 1.0f");
}

/*! \} */

/*!
 * \defgroup pedals_api_is_brake_pressed Tests for pedals_api_is_brake_pressed function
 * \{
 */

void test_pedals_api_is_brake_pressed_null_pointer(void) {
    enum PedalsReturnCode rc = pedals_api_is_brake_pressed(NULL);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_ERROR, rc, "Getter should return an error if the output pointer is NULL.");
}

void test_pedals_api_is_brake_pressed_threshold(void) {
    bool out;
    enum PedalsReturnCode rc = pedals_api_is_brake_pressed(&out);
    TEST_ASSERT_EQUAL_MESSAGE(PEDALS_RC_OK, rc, "Getter should return correctly if the output pointer is not NULL.");
    // after init, brake is set to 0.0f
    TEST_ASSERT_EQUAL_MESSAGE(false, out, "Brake should not be pressed if current brake percentage is zero");

    // verify just below the threshold
    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE - 0.01f);
    pedals_api_is_brake_pressed(&out);
    TEST_ASSERT_EQUAL_MESSAGE(false, out, "Brake should not be pressed if under threshold");

    // verify exactly as the threshold
    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE);
    pedals_api_is_brake_pressed(&out);
    TEST_ASSERT_EQUAL_MESSAGE(true, out, "Brake should be pressed if exactly as the threshold");

    // verify just above the threshold
    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE + 0.01f);
    pedals_api_is_brake_pressed(&out);
    TEST_ASSERT_EQUAL_MESSAGE(true, out, "Brake should be pressed if above threshold");
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
    RUN_TEST(test_pedals_api_set_throttle_out_range);
    /*! \} */

    /*!
     * \addtogroup pedals_api_set_brake
     * \{
     */
    RUN_TEST(test_pedals_api_set_brake_in_range);
    RUN_TEST(test_pedals_api_set_brake_out_range);
    /*! \} */

    /*!
     * \addtogroup pedals_api_set_brake_pressure
     * \{
     */
    RUN_TEST(test_pedals_api_set_brake_pressure_in_range);
    RUN_TEST(test_pedals_api_set_brake_pressure_out_range);
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
    RUN_TEST(test_pedals_api_get_requested_throttle_torque_null_pointer);
    RUN_TEST(test_pedals_api_get_requested_throttle_torque_return_value);
    /*! \} */

    /*!
     * \addtogroup pedals_api_is_brake_pressed
     * \{
     */
    RUN_TEST(test_pedals_api_is_brake_pressed_null_pointer);
    RUN_TEST(test_pedals_api_is_brake_pressed_threshold);
    /*! \} */

    return UNITY_END();
}