/*!
 * \file test_as_driver.c
 * \author Dorijan Di Zepp
 * \date 2026-05-31
 * \brief Unit tests using FFF for testing the AS driver module.
 * \details Tests target initialization and pointer-based data structs 
 * (pressures, mechanical sensors) due to safety-critical memory copies.
 * Trivial getters/setters (like ASMS, TSMS, SDC, RES flags) 
 * are omitted here as they lack conditional or pointer logic.
 */

#include <unity.h>
#include <stdbool.h>
#include "as-driver-api.h"
#include "fff.h"
#include "eagletrt-api.h"

extern struct ASDriverHandler as_driver_handler;

DEFINE_FFF_GLOBALS;

// mocks for raspberry
FAKE_VALUE_FUNC(enum ASDriverReturnCode, air_release_from_line, enum ASDriverAirLine);

void setUp(void) {
    // initialize as driver module
    as_driver_api_init(air_release_from_line);

    // reset mock state
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

    // Create a local reference structure representing the expected blank state
    struct ASDriverHandler expected_blank_handler;
    memset(&expected_blank_handler, 0, sizeof(struct ASDriverHandler));

    // Explicitly map the values that should be configured rather than purely zeroed out
    expected_blank_handler.as_mission = AS_DRIVER_MISSION_NOT_SELECTED;
    expected_blank_handler.release_air = air_release_from_line;

    // Compare the initialized global struct memory against the expected configuration layout
    int mem_check = memcmp(&as_driver_handler, &expected_blank_handler, sizeof(struct ASDriverHandler));

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mem_check, "The internal state structure fields were not completely or correctly initialized to defaults");
}
/*! \} */

/*!
 * \defgroup as_driver_api_pressures Tests for as_driver_api_set_pressures and 
 * as_driver_api_get_pressures functions
 * \{
 */

void test_as_driver_api_set_pressures_success(void) {
    memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    struct ASDriverPressures input_pressures = {
        .brake_pressure_1_1 = 1.5f,
        .brake_pressure_1_2 = 2.5f,
        .brake_pressure_1_3 = 3.5f,
        .brake_pressure_1_4 = 4.5f,
        .brake_pressure_2_1 = 5.5f,
        .brake_pressure_2_2 = 6.5f
    };

    enum ASDriverReturnCode rc = as_driver_api_set_pressures(&input_pressures);

    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);
    // Check that memory matches perfectly
    int mem_check = memcmp(&as_driver_handler.pressures, &input_pressures, sizeof(struct ASDriverPressures));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mem_check, "The pressures struct was not copied correctly into storage");
}

void test_as_driver_api_set_pressures_null_pointer(void) {
    const struct ASDriverPressures *invalid_input = NULL;

    enum ASDriverReturnCode rc = as_driver_api_set_pressures(invalid_input);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Set pressures should return ERROR when given NULL");
}

void test_as_driver_api_get_pressures_success(void) {
    // Manually prime the internal state with known values
    as_driver_handler.pressures.brake_pressure_1_1 = 10.1f;
    as_driver_handler.pressures.brake_pressure_1_2 = 20.2f;
    as_driver_handler.pressures.brake_pressure_1_3 = 30.3f;
    as_driver_handler.pressures.brake_pressure_1_4 = 40.4f;
    as_driver_handler.pressures.brake_pressure_2_1 = 50.5f;
    as_driver_handler.pressures.brake_pressure_2_2 = 60.6f;

    struct ASDriverPressures output_buffer;
    memset(&output_buffer, 0, sizeof(struct ASDriverPressures));

    enum ASDriverReturnCode rc = as_driver_api_get_pressures(&output_buffer);

    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);

    // Compare output container to private state layout
    int mem_check = memcmp(&output_buffer, &as_driver_handler.pressures, sizeof(struct ASDriverPressures));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mem_check, "The pressures data retrieved does not match internal storage");
}

void test_as_driver_api_get_pressures_null_pointer(void) {
    struct ASDriverPressures *invalid_output_buffer = NULL;

    enum ASDriverReturnCode rc = as_driver_api_get_pressures(invalid_output_buffer);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Get pressures should return ERROR when given NULL destination");
}
/*! \} */

/*!
 * \defgroup as_driver_api_mechanical_sensors Tests for as_driver_api_set_mechanical_sensors and 
 * as_driver_api_get_mechanical_sensors functions
 * \{
 */

void test_as_driver_api_set_mechanical_sensors_success(void) {
    memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    struct ASDriverMechanicalSensors input_mech = {
        .bremsweg_1_1 = 12.5f,
        .bremskraft_3_1 = 250.0f
    };

    enum ASDriverReturnCode rc = as_driver_api_set_mechanical_sensors(&input_mech);

    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);
    // Check that memory matches perfectly
    int mem_check = memcmp(&as_driver_handler.mechanical_sensors, &input_mech, sizeof(struct ASDriverMechanicalSensors));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mem_check, "The mechanical sensors struct was not copied correctly into storage");
}

void test_as_driver_api_set_mechanical_sensors_null_pointer(void) {
    const struct ASDriverMechanicalSensors *invalid_input = NULL;

    enum ASDriverReturnCode rc = as_driver_api_set_mechanical_sensors(invalid_input);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Set mechanical sensors should return ERROR when given NULL");
}

void test_as_driver_api_get_mechanical_sensors_success(void) {
    // Manually prime the internal state with known values
    as_driver_handler.mechanical_sensors.bremsweg_1_1 = 8.4f;
    as_driver_handler.mechanical_sensors.bremskraft_3_1 = 185.0f;

    struct ASDriverMechanicalSensors output_buffer;
    memset(&output_buffer, 0, sizeof(struct ASDriverMechanicalSensors));

    enum ASDriverReturnCode rc = as_driver_api_get_mechanical_sensors(&output_buffer);

    TEST_ASSERT_EQUAL_INT(AS_DRIVER_RC_OK, rc);
    // Compare output container to private state layout
    int mem_check = memcmp(&output_buffer, &as_driver_handler.mechanical_sensors, sizeof(struct ASDriverMechanicalSensors));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mem_check, "The mechanical sensors data retrieved does not match internal storage");
}

void test_as_driver_api_get_mechanical_sensors_null_pointer(void) {
    struct ASDriverMechanicalSensors *invalid_output_buffer = NULL;

    enum ASDriverReturnCode rc = as_driver_api_get_mechanical_sensors(invalid_output_buffer);

    TEST_ASSERT_EQUAL_INT_MESSAGE(AS_DRIVER_RC_ERROR, rc, "Get mechanical sensors should return ERROR when given NULL destination");
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
    RUN_TEST(test_as_driver_api_set_pressures_success);
    RUN_TEST(test_as_driver_api_set_pressures_null_pointer);
    RUN_TEST(test_as_driver_api_get_pressures_success);
    RUN_TEST(test_as_driver_api_get_pressures_null_pointer);
    /*! \} */

    /*!
     * \addtogroup as_driver_api_mechanical_sensors
     * \{
     */
    RUN_TEST(test_as_driver_api_set_mechanical_sensors_success);
    RUN_TEST(test_as_driver_api_set_mechanical_sensors_null_pointer);
    RUN_TEST(test_as_driver_api_get_mechanical_sensors_success);
    RUN_TEST(test_as_driver_api_get_mechanical_sensors_null_pointer);
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