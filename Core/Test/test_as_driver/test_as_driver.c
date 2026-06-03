/*!
 * \file test_as_driver.c
 * \author Dorijan Di Zepp
 * \date 2026-06-04
 * \brief Unit tests using FFF for testing the AS driver module.
 * \details Enforces comprehensive verification of initialization, flat-array
 * data copies, edge-case safety boundaries, and abstract state enums.
 */

#include <unity.h>
#include <stdbool.h>
#include <string.h>
#include "as-driver-api.h"
#include "fff.h"
#include "eagletrt-api.h"

extern struct ASDriverHandler as_driver_handler;

DEFINE_FFF_GLOBALS;

// Mocks for functional safety callbacks
FAKE_VALUE_FUNC(enum ASDriverReturnCode, air_release_from_line, enum ASDriverAirLine);

void setUp(void) {
    // Initialize AS driver module to pristine defaults before each test
    (void)as_driver_api_init(air_release_from_line);

    // Reset mock execution state
    RESET_FAKE(air_release_from_line);

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup as_driver_api_init Tests for as_driver_api_init function
 * \{
 */

void test_as_driver_api_init_null_air_release_from_line(void) {
    enum ASDriverReturnCode rc = as_driver_api_init(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Init should fail if the callback for air release is NULL");
}

void test_as_driver_api_init_struct_with_correct_default_values(void) {
    enum ASDriverReturnCode rc = as_driver_api_init(air_release_from_line);
    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);

    // Create a local reference structure representing the expected blank state
    struct ASDriverHandler expected_blank_handler;
    (void)memset(&expected_blank_handler, 0, sizeof(struct ASDriverHandler));

    // Explicitly map the state engine values that should be configured defaults
    expected_blank_handler.as_mission = AS_DRIVER_MISSION_NOT_SELECTED;
    expected_blank_handler.res_signal = AS_DRIVER_RES_SIGNAL_NONE;
    expected_blank_handler.watchdog_state = AS_DRIVER_WATCHDOG_STATE_UNTESTED;
    expected_blank_handler.release_air = air_release_from_line;

    // Compare the initialized global struct memory directly via Unity memory assertions
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_blank_handler, &as_driver_handler, sizeof(struct ASDriverHandler), "The internal state structure fields were not completely or correctly initialized to defaults");
}
/*! \} */

/*!
 * \defgroup as_driver_api_pressures Tests for brake pressure single-point and bulk array functions
 * \{
 */

void test_as_driver_api_set_brake_pressure_individual_success(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    as_driver_api_set_brake_pressure(AS_DRIVER_BRAKE_PRESSURE_1_1, 4.25f);
    as_driver_api_set_brake_pressure(AS_DRIVER_BRAKE_PRESSURE_2_2, 8.50f);

    TEST_ASSERT_EQUAL_FLOAT(4.25f, as_driver_handler.brake_pressures[AS_DRIVER_BRAKE_PRESSURE_1_1]);
    TEST_ASSERT_EQUAL_FLOAT(8.50f, as_driver_handler.brake_pressures[AS_DRIVER_BRAKE_PRESSURE_2_2]);
}

void test_as_driver_api_set_brake_pressure_out_of_bounds_ignored(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    // Force an invalid index modification attempt past the compiler check
    as_driver_api_set_brake_pressure((enum ASDriverBrakePressure)99, 150.0f);

    struct ASDriverHandler expected_clean;
    (void)memset(&expected_clean, 0, sizeof(struct ASDriverHandler));

    // Structural check confirming no out-of-bounds corruption happened
    TEST_ASSERT_EQUAL_MEMORY(&expected_clean, &as_driver_handler, sizeof(struct ASDriverHandler));
}

void test_as_driver_api_set_all_brake_pressures_success(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    const float input_pressures[AS_DRIVER_BRAKE_PRESSURE_COUNT] = {
        [AS_DRIVER_BRAKE_PRESSURE_1_1] = 1.5f,
        [AS_DRIVER_BRAKE_PRESSURE_1_2] = 2.5f,
        [AS_DRIVER_BRAKE_PRESSURE_1_3] = 3.5f,
        [AS_DRIVER_BRAKE_PRESSURE_1_4] = 4.5f,
        [AS_DRIVER_BRAKE_PRESSURE_2_1] = 5.5f,
        [AS_DRIVER_BRAKE_PRESSURE_2_2] = 6.5f
    };

    enum ASDriverReturnCode rc = as_driver_api_set_all_brake_pressures(input_pressures);

    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(input_pressures, as_driver_handler.brake_pressures, sizeof(float) * AS_DRIVER_BRAKE_PRESSURE_COUNT, "The bulk pressures buffer was not copied correctly into storage");
}

void test_as_driver_api_set_all_brake_pressures_null_pointer(void) {
    enum ASDriverReturnCode rc = as_driver_api_set_all_brake_pressures(NULL);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Set bulk pressures should return ERROR when given NULL pointer");
}

void test_as_driver_api_get_brake_pressure_individual_success(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));
    as_driver_handler.brake_pressures[AS_DRIVER_BRAKE_PRESSURE_1_4] = 12.34f;

    float pressure = as_driver_api_get_brake_pressure(AS_DRIVER_BRAKE_PRESSURE_1_4);

    TEST_ASSERT_EQUAL_FLOAT(12.34f, pressure);
}

void test_as_driver_api_get_brake_pressure_out_of_bounds_fallback(void) {
    float fallback_value = as_driver_api_get_brake_pressure((enum ASDriverBrakePressure)99);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, fallback_value, "Out of bounds pressure request should return a clean 0.0f fallback loop");
}

void test_as_driver_api_get_all_brake_pressures_success(void) {
    as_driver_handler.brake_pressures[AS_DRIVER_BRAKE_PRESSURE_1_1] = 10.1f;
    as_driver_handler.brake_pressures[AS_DRIVER_BRAKE_PRESSURE_2_1] = 50.5f;

    const float *retrieved_pressures = as_driver_api_get_all_brake_pressures();

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(as_driver_handler.brake_pressures, retrieved_pressures, sizeof(float) * AS_DRIVER_BRAKE_PRESSURE_COUNT, "The retrieved pointer layout does not mirror internal storage configuration");
}
/*! \} */

/*!
 * \defgroup as_driver_api_mechanical_sensors Tests for mechanical sensor single-point and bulk array functions
 * \{
 */

void test_as_driver_api_set_mechanical_sensor_individual_success(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    as_driver_api_set_mechanical_sensor(AS_DRIVER_MECHANICAL_SENSOR_BREMSWEG_1_1, 15.4f);
    as_driver_api_set_mechanical_sensor(AS_DRIVER_MECHANICAL_SENSOR_BREMSKRAFT_3_1, 320.0f);

    TEST_ASSERT_EQUAL_FLOAT(15.4f, as_driver_handler.mechanical_sensors[AS_DRIVER_MECHANICAL_SENSOR_BREMSWEG_1_1]);
    TEST_ASSERT_EQUAL_FLOAT(320.0f, as_driver_handler.mechanical_sensors[AS_DRIVER_MECHANICAL_SENSOR_BREMSKRAFT_3_1]);
}

void test_as_driver_api_set_mechanical_sensor_out_of_bounds_ignored(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    as_driver_api_set_mechanical_sensor((enum ASDriverMechanicalSensor)99, 500.0f);

    struct ASDriverHandler expected_clean;
    (void)memset(&expected_clean, 0, sizeof(struct ASDriverHandler));

    TEST_ASSERT_EQUAL_MEMORY(&expected_clean, &as_driver_handler, sizeof(struct ASDriverHandler));
}

void test_as_driver_api_set_all_mechanical_sensors_success(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    const float input_mech[AS_DRIVER_MECHANICAL_SENSOR_COUNT] = {
        [AS_DRIVER_MECHANICAL_SENSOR_BREMSWEG_1_1] = 12.5f,
        [AS_DRIVER_MECHANICAL_SENSOR_BREMSWEG_1_2] = 14.2f,
        [AS_DRIVER_MECHANICAL_SENSOR_BREMSKRAFT_3_1] = 250.0f
    };

    enum ASDriverReturnCode rc = as_driver_api_set_all_mechanical_sensors(input_mech);

    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(input_mech, as_driver_handler.mechanical_sensors, sizeof(float) * AS_DRIVER_MECHANICAL_SENSOR_COUNT, "The bulk mechanical array was not copied correctly into storage");
}

void test_as_driver_api_set_all_mechanical_sensors_null_pointer(void) {
    enum ASDriverReturnCode rc = as_driver_api_set_all_mechanical_sensors(NULL);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Set bulk mechanical sensors should return ERROR when given NULL pointer");
}

void test_as_driver_api_get_mechanical_sensor_individual_success(void) {
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));
    as_driver_handler.mechanical_sensors[AS_DRIVER_MECHANICAL_SENSOR_BREMSKRAFT_3_1] = 195.5f;

    float force = as_driver_api_get_mechanical_sensor(AS_DRIVER_MECHANICAL_SENSOR_BREMSKRAFT_3_1);

    TEST_ASSERT_EQUAL_FLOAT(195.5f, force);
}

void test_as_driver_api_get_mechanical_sensor_out_of_bounds_fallback(void) {
    float fallback_value = as_driver_api_get_mechanical_sensor((enum ASDriverMechanicalSensor)99);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, fallback_value, "Out of bounds mechanical request should return a clean 0.0f fallback loop");
}

void test_as_driver_api_get_all_mechanical_sensors_success(void) {
    as_driver_handler.mechanical_sensors[AS_DRIVER_MECHANICAL_SENSOR_BREMSWEG_1_2] = 9.1f;

    const float *retrieved_sensors = as_driver_api_get_all_mechanical_sensors();

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(as_driver_handler.mechanical_sensors, retrieved_sensors, sizeof(float) * AS_DRIVER_MECHANICAL_SENSOR_COUNT, "The retrieved mechanical array pointer does not mirror internal storage");
}
/*! \} */

/*!
 * \defgroup as_driver_api_release_air Tests for as_driver_api_release_air function
 * \{
 */

void test_as_driver_api_release_air_success(void) {
    air_release_from_line_fake.return_val = AS_DRIVER_RC_OK;

    enum ASDriverReturnCode rc = as_driver_api_release_air(AS_DRIVER_AIR_LINE_1);

    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, air_release_from_line_fake.call_count);
    TEST_ASSERT_EQUAL_INT(AS_DRIVER_AIR_LINE_1, air_release_from_line_fake.arg0_val);
}

void test_as_driver_api_release_air_callback_failure(void) {
    // Force the FFF mock callback to return an error status code
    air_release_from_line_fake.return_val = AS_DRIVER_RC_ERROR;

    enum ASDriverReturnCode rc = as_driver_api_release_air(AS_DRIVER_AIR_LINE_2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "The API should forward the error return code directly from the failing callback");

    // Ensure the callback was actually executed exactly once with the correct parameter
    TEST_ASSERT_EQUAL_INT(1, air_release_from_line_fake.call_count);
    TEST_ASSERT_EQUAL_INT(AS_DRIVER_AIR_LINE_2, air_release_from_line_fake.arg0_val);
}

void test_as_driver_api_release_air_null_callback(void) {
    // Intentionally break the internal callback reference to test safety boundaries
    as_driver_handler.release_air = NULL;

    enum ASDriverReturnCode rc = as_driver_api_release_air(AS_DRIVER_AIR_LINE_2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Should fail when attempting to release air without a valid callback");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup as_driver_api_init
     * \{
     */
    RUN_TEST(test_as_driver_api_init_null_air_release_from_line);
    RUN_TEST(test_as_driver_api_init_struct_with_correct_default_values);
    /*! \} */

    /*!
     * \addtogroup as_driver_api_pressures
     * \{
     */
    RUN_TEST(test_as_driver_api_set_brake_pressure_individual_success);
    RUN_TEST(test_as_driver_api_set_brake_pressure_out_of_bounds_ignored);
    RUN_TEST(test_as_driver_api_set_all_brake_pressures_success);
    RUN_TEST(test_as_driver_api_set_all_brake_pressures_null_pointer);
    RUN_TEST(test_as_driver_api_get_brake_pressure_individual_success);
    RUN_TEST(test_as_driver_api_get_brake_pressure_out_of_bounds_fallback);
    RUN_TEST(test_as_driver_api_get_all_brake_pressures_success);
    /*! \} */

    /*!
     * \addtogroup as_driver_api_mechanical_sensors
     * \{
     */
    RUN_TEST(test_as_driver_api_set_mechanical_sensor_individual_success);
    RUN_TEST(test_as_driver_api_set_mechanical_sensor_out_of_bounds_ignored);
    RUN_TEST(test_as_driver_api_set_all_mechanical_sensors_success);
    RUN_TEST(test_as_driver_api_set_all_mechanical_sensors_null_pointer);
    RUN_TEST(test_as_driver_api_get_mechanical_sensor_individual_success);
    RUN_TEST(test_as_driver_api_get_mechanical_sensor_out_of_bounds_fallback);
    RUN_TEST(test_as_driver_api_get_all_mechanical_sensors_success);
    /*! \} */

    /*!
     * \addtogroup as_driver_api_release_air
     * \{
     */
    RUN_TEST(test_as_driver_api_release_air_success);
    RUN_TEST(test_as_driver_api_release_air_null_callback);
    RUN_TEST(test_as_driver_api_release_air_callback_failure);
    /*! \} */

    return UNITY_END();
}