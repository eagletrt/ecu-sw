/*!
 * \file post-api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-25
 * \brief This file defines Power-On Self-Test (POST) functions for system initialization.
 */

#include "post-api.h"
#include "eagletrt-api.h"
#include "as-driver-api.h"
#include "buzzer-api.h"
#include "can-communication-api.h"
#include "inverters-api.h"
#include "pedals-api.h"
#include "raspberry-api.h"
#include "tractive-system-api.h"

enum PostReturnCode post_api_do_init(struct PostConfig *post_config) {
    if (post_config == NULL) {
        return POST_RC_ERROR;
    }

    // NULL pointer validation of all required dependency members
    if (post_config->as_air_release == NULL ||
        post_config->inverters_send_drive_command == NULL ||
        post_config->inverters_set_torque == NULL ||
        post_config->raspberry_pin_control == NULL ||
        post_config->ts_send_command == NULL) {
        return POST_RC_ERROR;
    }

    // Validate every slot inside the buzzer callback arrays
    for (size_t i = 0; i < (size_t)BUZZER_TYPE_COUNT; i++) {
        if (post_config->buzzer_on_ptrs[i] == NULL ||
            post_config->buzzer_off_ptrs[i] == NULL ||
            post_config->buzzer_delay_ptrs[i] == NULL ||
            post_config->buzzer_tick_ptrs[i] == NULL) {
            return POST_RC_ERROR;
        }
    }

    enum PostReturnCode final_status = POST_RC_OK;

    // Initialize all modules given the post configuration
    if (as_driver_api_init(post_config->as_air_release) != AS_DRIVER_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (buzzer_api_init(post_config->buzzer_on_ptrs,
                        post_config->buzzer_off_ptrs,
                        post_config->buzzer_delay_ptrs,
                        post_config->buzzer_tick_ptrs) != BUZZER_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    // Initialize the CAN Communication Module using the network configurations
    if (can_communication_api_init(post_config->can_networks) != CAN_COMMUNICATION_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (inverters_api_init(post_config->inverters_send_drive_command,
                           post_config->inverters_set_torque) != INVERTERS_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (pedals_api_init() != PEDALS_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    // Initialize RaspberryPi module using the configurable startup pin state
    // The pin state validity is already managed by the raspberry module
    if (raspberry_api_init(post_config->raspberry_pin_control,
                           post_config->raspberry_initial_state) != RASPBERRY_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (ts_api_init(post_config->ts_send_command) != TS_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    // Returns POST_RC_OK if everything passed or POST_RC_ERROR if one or more sub-modules failed
    return final_status;
}