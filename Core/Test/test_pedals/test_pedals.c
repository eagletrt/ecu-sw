#include <unity.h>
#include "pedals-api.h"

void setUp(void) {
    pedals_api_init();
}

void tearDown(void) {
}

/* --- Test Cases --- */

/*!
 * \brief Verify that after initialization, the pedals availability is set to false
 */
void test_pedals_init(void) {
    pedals_api_init();
    TEST_ASSERT_FALSE_MESSAGE(pedals_api_get_is_available(), "Pedals should be unavailable after initialization");
}

/*!
 * \brief Verify that the throttle setting is successfull only if value is in valid range
 */
void test_pedals_set_throttle(void) {
    pedals_api_init();

    // if pedals are unavailable, no change can be done
    pedals_api_set_throttle(0.1f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_throttle(), "Throttle should be set to 0.0");

    pedals_api_set_is_available(true);

    pedals_api_set_throttle(0.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pedals_api_get_throttle(), "Throttle should be set to 0.5");

    pedals_api_set_throttle(1.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pedals_api_get_throttle(), "Throttle should be set to 0.5 as 1.5 is over the limit");

    pedals_api_set_throttle(-0.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pedals_api_get_throttle(), "Throttle should be set to 0.5 as negative values are not valid");

    //test limit cases
    pedals_api_set_throttle(1.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pedals_api_get_throttle(), "Throttle should be set to 1");

    pedals_api_set_throttle(0.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_throttle(), "Throttle should be set to 0");
}

/*!
 * \brief Verify that the brake setting is successfull only if value is in valid range
 */
void test_pedals_set_brake(void) {
    pedals_api_init();

    // if pedals are unavailable, no change can be done
    pedals_api_set_brake(0.1f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_brake(), "Brake should be set to 0.0");

    pedals_api_set_is_available(true);

    pedals_api_set_brake(0.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pedals_api_get_brake(), "Brake should be set to 0.5");

    pedals_api_set_brake(1.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pedals_api_get_brake(), "Brake should be set to 0.5 as 1.5 is over the limit");

    pedals_api_set_brake(-0.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pedals_api_get_brake(), "Brake should be set to 0.5 as negative values are not valid");

    //test limit cases
    pedals_api_set_brake(1.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pedals_api_get_brake(), "Brake should be set to 1");

    pedals_api_set_brake(0.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_brake(), "Brake should be set to 0");
}

/*!
 * \brief Verify that the brake pressure setting is successfull only if value is in valid range
 */
void test_pedals_set_brake_pressure(void) {
    pedals_api_init();

    // if pedals are unavailable, no change can be done
    pedals_api_set_brake_pressure(0.1f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_brake_pressure(), "Brake pressure should be set to 0.0");

    pedals_api_set_is_available(true);

    pedals_api_set_brake_pressure(40.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(40.0f, pedals_api_get_brake_pressure(), "Brake pressure should be set to 40.0");

    pedals_api_set_brake_pressure(PEDALS_MAX_BRAKE_PRESSURE_BAR + 1);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(40.0f, pedals_api_get_brake_pressure(), "Brake pressure should be set to 40.0 as PEDALS_MAX_BRAKE_PRESSURE_BAR + 1 is over the limit");

    pedals_api_set_brake_pressure(-0.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(40.0f, pedals_api_get_brake_pressure(), "Brake pressure should be set to 40.0 as negative values are not valid");

    //test limit cases
    pedals_api_set_brake_pressure(PEDALS_MAX_BRAKE_PRESSURE_BAR);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(PEDALS_MAX_BRAKE_PRESSURE_BAR, pedals_api_get_brake_pressure(), "Brake pressure should be set to PEDALS_MAX_BRAKE_PRESSURE_BAR");

    pedals_api_set_brake_pressure(0.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_brake_pressure(), "Brake pressure should be set to 0");
}

/*!
 * \brief Verify that the torque value is fetched correctly even in the case the pedals are unavailable
 */
void test_pedals_get_requested_throttle_torque(void) {
    pedals_api_init();

    // pedals unavailable by default, torque should be zero
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_requested_throttle_torque(), "Torque should be zero as pedals are NOT available");

    pedals_api_set_is_available(true);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_requested_throttle_torque(), "Torque should be zero as throttle is set to zero");

    // torque = PEDALS_MAX_TORQUE_NM (91.0f) * throttle
    pedals_api_set_throttle(0.5f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(PEDALS_MAX_TORQUE_NM * 0.5f, pedals_api_get_requested_throttle_torque(), "Torque should be 5% of the max torque");

    // check that even if throttle is different from zero, if pedals unavailable torque is set to zero
    pedals_api_set_is_available(false);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pedals_api_get_requested_throttle_torque(), "Torque should be zero as pedals are NOT available and throttle is greater than zero");
}

/*!
 * \brief Verify that the brake is considered pressed only if over the threshold
 */
void test_pedals_is_brake_pressed(void) {
    pedals_api_init();

    TEST_ASSERT_FALSE_MESSAGE(pedals_api_is_brake_pressed(), "Brake is NOT pressed if pedals are unavailable");

    pedals_api_set_is_available(true);

    TEST_ASSERT_FALSE_MESSAGE(pedals_api_is_brake_pressed(), "Brake is NOT pressed if brake percentage is set to 0");

    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE - 0.01f);
    TEST_ASSERT_FALSE_MESSAGE(pedals_api_is_brake_pressed(), "Brake is NOT pressed if brake percentage is below the threshold");

    pedals_api_set_brake(PEDALS_BRAKE_THRESHOLD_PERCENTAGE);
    TEST_ASSERT_TRUE_MESSAGE(pedals_api_is_brake_pressed(), "Brake is pressed if brake percentage is the same as the threshold value");

    pedals_api_set_brake(0.8f);
    TEST_ASSERT_TRUE_MESSAGE(pedals_api_is_brake_pressed(), "Brake is pressed if brake percentage is above the threshold");
}

// -------------------------------

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pedals_init);
    RUN_TEST(test_pedals_set_throttle);
    RUN_TEST(test_pedals_set_brake);
    RUN_TEST(test_pedals_set_brake_pressure);
    RUN_TEST(test_pedals_get_requested_throttle_torque);
    RUN_TEST(test_pedals_is_brake_pressed);
    return UNITY_END();
}