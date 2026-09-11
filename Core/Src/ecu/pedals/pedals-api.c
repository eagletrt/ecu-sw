/*!
 * \file pedals-api.c
 * \author Dorijan Di Zepp
 * \date 2026-03-25
 * \brief Implementation of the pedals module API.
 */

#include "pedals-api.h"
#include "eagletrt.h"
#include "string.h"

constexpr uint32_t pedals_timeout_ms = 120U;

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
EAGLETRT_STATIC struct PedalsHandler pedals_handler;

enum PedalsReturnCode pedals_api_init(pedals_get_tick_callback get_tick) {
    memset(&pedals_handler, 0U, sizeof(pedals_handler));
    pedals_handler.get_tick = get_tick;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_throttle(float throttle) {
    if (throttle < 0.0F || throttle > 1.0F) {
        return PEDALS_RC_ERROR;
    }

    if (pedals_handler.get_tick != NULL) {
        pedals_handler.last_update_tick = pedals_handler.get_tick();
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

float pedals_api_get_requested_throttle_torque(void) {
    return PEDALS_MAX_TORQUE_NM * pedals_handler.throttle;
}

bool pedals_api_is_brake_pressed() {
    return (pedals_handler.brake_pressure >= PEDALS_BRAKE_THRESHOLD_BAR);
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

bool pedals_api_is_timeout() {
    if (pedals_handler.get_tick == NULL) {
        return true; // If no tick callback is provided, consider it a timeout.
    }

    uint32_t current_tick = pedals_handler.get_tick();
    return (current_tick - pedals_handler.last_update_tick) > pedals_timeout_ms;
}
