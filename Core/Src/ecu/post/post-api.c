/*!
 * \file post-api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-25
 * \brief This file defines Power-On Self-Test (POST) functions for system initialization.
 */

#include "post-api.h"
#include "as-driver-api.h"
#include "buzzer-api.h"
#include "can-communication-api.h"
#include "identity-api.h"
#include "inverters-api.h"
#include "pedals-api.h"
#include "raspberry-api.h"
#include "shutdown-api.h"
#include "temperatures-api.h"
#include "tsac-api.h"
#include "vehicle-api.h"
#include "lights-api.h"

enum PostReturnCode post_api_do_init(struct PostConfig *post_config) {
    if (post_config == NULL) {
        return POST_RC_ERROR;
    }

    // NULL pointer validation of all required dependency members
    if (post_config->as_air_release == NULL ||
        post_config->raspberry_pin_control == NULL ||
        post_config->shutdown_control_relay == NULL ||
        post_config->pedals_get_tick == NULL ||
        post_config->tsac_get_tick == NULL ||
        post_config->vehicle_tson_pressed == NULL ||
        post_config->lights_set_state == NULL) {
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

    if (inverters_api_init() != INVERTERS_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (inverters_api_attach(EPHORUS_WHEEL_REAR_LEFT) != INVERTERS_RC_OK ||
        inverters_api_attach(EPHORUS_WHEEL_REAR_RIGHT) != INVERTERS_RC_OK ||
        inverters_api_attach(EPHORUS_WHEEL_FRONT_LEFT) != INVERTERS_RC_OK ||
        inverters_api_attach(EPHORUS_WHEEL_FRONT_RIGHT) != INVERTERS_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (pedals_api_init(post_config->pedals_get_tick) != PEDALS_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    // Initialize RaspberryPi module using the configurable startup pin state
    // The pin state validity is already managed by the raspberry module
    if (raspberry_api_init(post_config->raspberry_pin_control,
                           post_config->raspberry_initial_state) != RASPBERRY_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (shutdown_api_init(post_config->shutdown_control_relay) != SHUTDOWN_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (identity_api_init() != IDENTITY_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (tsac_api_init(post_config->tsac_get_tick) != TSAC_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (vehicle_api_init(post_config->vehicle_tson_pressed) != VEHICLE_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (lights_api_init(post_config->lights_set_state) != LIGHTS_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    if (temperatures_api_init() != TEMPERATURES_RC_OK) {
        final_status = POST_RC_ERROR;
    }

    // Returns POST_RC_OK if everything passed or POST_RC_ERROR if one or more sub-modules failed
    return final_status;
}
