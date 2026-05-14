/*!
 * \file driver-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-14
 * \brief Implementation of the driver control and monitoring API.
 */

#include <string.h>
#include "driver-api.h"
#include "ecu_fsm.h"
#include "eagletrt-api.h"

/*!
 * \brief Static driver handler.
 */
EAGLETRT_STATIC struct DriverHandler driver_handler;

/*!
 * \brief Reference to the vehicle's global Finite State Machine state.
 * \note Defined in \ref main.c .
 */
extern state_t current_state;

enum DriverReturnCode driver_api_init(enum DriverType driver_type, driver_wait_for_driver_callback wait_for_driver, driver_continuous_check_callback continuous_check) {
    // Verify if the function is called only if in Init or Idle state
    if (current_state != STATE_INIT && current_state != STATE_IDLE) {
        return DRIVER_RC_ERROR;
    }

    // Since the init function can be called many times in init or idle
    // it is required to first validate the parameters and
    // then reset the values to avoid loosing the past valid configuration
    if (driver_type >= DRIVER_TYPE_COUNT || (wait_for_driver == NULL) || (continuous_check == NULL)) {
        return DRIVER_RC_ERROR;
    }

    // All parameters are valid
    // memset to zero to avoid unexpected values
    memset(&driver_handler, 0U, sizeof(struct DriverHandler));

    driver_handler.driver_type = driver_type;

    driver_handler.wait_for_driver = wait_for_driver;
    driver_handler.continuous_check = continuous_check;

    return DRIVER_RC_OK;
}

enum DriverReturnCode driver_api_is_driver_ready(void) {
    // All the logic is defined within the callback meaning that
    // the function return code is based entirely on the return code
    // of the callback
    if (driver_handler.wait_for_driver == NULL) {
        return DRIVER_RC_ERROR;
    }

    return driver_handler.wait_for_driver();
}

enum DriverReturnCode driver_api_continuous_check(void) {
    // All the logic is defined within the callback meaning that
    // the function return code is based entirely on the return code
    // of the callback
    if (driver_handler.continuous_check == NULL) {
        return DRIVER_RC_ERROR;
    }

    return driver_handler.continuous_check();
}

enum DriverType driver_api_get_driver_type(void) {
    return driver_handler.driver_type;
}