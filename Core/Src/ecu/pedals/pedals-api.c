/*!
 * \file pedals-api.c
 * \author Dorijan Di Zepp
 * \date 2026-03-25
 * \brief Implementation of the pedals module API.
 */

#include "pedals-api.h"
#include "eagletrt-api.h"
#include "string.h"

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
EAGLETRT_STATIC struct PedalsHandler pedals_handler;

enum PedalsReturnCode pedals_api_init() {
    memset(&pedals_handler, 0U, sizeof(pedals_handler));
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_throttle(float throttle) {
    if (throttle < 0.0F || throttle > 1.0F) {
        return PEDALS_RC_ERROR;
    }

    pedals_handler.throttle = throttle;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_brake(float brake) {
    if (brake < 0.0F || brake > 1.0F) {
        return PEDALS_RC_ERROR;
    }

    pedals_handler.brake = brake;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_brake_pressure(float brake_pressure) {
    if (brake_pressure < 0.0F || brake_pressure > PEDALS_MAX_BRAKE_PRESSURE_BAR) {
        return PEDALS_RC_ERROR;
    }

    pedals_handler.brake_pressure = brake_pressure;
    return PEDALS_RC_OK;
}

float pedals_api_get_requested_throttle_torque() {
    return PEDALS_MAX_TORQUE_NM * pedals_handler.throttle;
}

bool pedals_api_is_brake_pressed() {
    return (pedals_handler.brake >= PEDALS_BRAKE_THRESHOLD_PERCENTAGE);
}

float pedals_api_get_throttle() {
    return pedals_handler.throttle;
}

float pedals_api_get_brake() {
    return pedals_handler.brake;
}

float pedals_api_get_brake_pressure() {
    return pedals_handler.brake_pressure;
}