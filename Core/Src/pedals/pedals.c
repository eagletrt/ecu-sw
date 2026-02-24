/*!
 * \file pedals.c
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Implementation of pedals.
 */

#include "pedals.h"
#include "eagletrt-api.h"

EAGLETRT_STATIC struct PedalsHandler pedals_handler = { 0 };

enum PedalsReturnCode pedals_init() {
    pedals_handler.throttle_pct = 0.f;
    pedals_handler.brake_pct = 0.f;
    pedals_handler.brake_pressure = 0.f;
    pedals_handler.is_available = false; // assume disconnected until someone confirm availability
                                         // through pedals_set_is_available
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_set_throttle_pct(float throttle_pct) {
    if (throttle_pct < 0.0f || throttle_pct > 100.0f)
        return PEDALS_RC_ERROR; // leave unchanged the last throttle value

    pedals_handler.throttle_pct = throttle_pct;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_set_brake_pct(float brake_pct) {
    if (brake_pct < 0.0f || brake_pct > 100.0f)
        return PEDALS_RC_ERROR; // leave unchanged the last brake value

    pedals_handler.brake_pct = brake_pct;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_set_brake_pressure(float brake_pressure) {
    if (brake_pressure < 0.0f || brake_pressure > PEDALS_MAX_BK_PRESSURE)
        return PEDALS_RC_ERROR; // leave unchanged the last brake pressure value

    pedals_handler.brake_pressure = brake_pressure;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_set_is_available(bool is_available) {
    pedals_handler.is_available = is_available;
    return PEDALS_RC_OK;
}

bool pedals_is_available() {
    return pedals_handler.is_available;
}

float pedals_get_requested_throttle_torque() {
    // if pedals are offline, torque must be exactly 0
    if (!pedals_handler.is_available) {
        return 0.0f;
    }

    return PEDALS_MAX_TORQUE * (pedals_handler.throttle_pct / 100.0f);
}

bool pedals_is_brake_pressed() {
    // if communication is lost, assume the brake is NOT pressed
    if (!pedals_handler.is_available) {
        return false;
    }

    return (pedals_handler.brake_pct >= PEDALS_BRAKE_THRESHOLD_PCT);
}

const struct PedalsHandler *pedals_get_values() {
    return &pedals_handler;
}
