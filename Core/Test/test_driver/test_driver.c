/*!
 * \file test_driver.c
 * \author Dorijan Di Zepp
 * \date 2026-05-13
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

// mock for driver module
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

int main(void) {
    UNITY_BEGIN();

    return UNITY_END();
}