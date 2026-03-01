/*!
 * \file pedals-api.c
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Manages the pedal system state via getters and setters.
 * \warning Pedal availability is managed externally. This module does not 
 * perform health checks; it must be explicitly notified by an external 
 * safety function if the pedal hardware becomes unavailable or invalid.
 */

#include "pedals-api.h"
#include "eagletrt-api.h"

EAGLETRT_STATIC struct PedalsHandler pedals_handler;

enum PedalsReturnCode pedals_api_init() {
    pedals_handler.throttle = 0.f;
    pedals_handler.brake = 0.f;
    pedals_handler.brake_pressure = 0.f;
    pedals_handler.is_available = false; // assume disconnected until someone confirms availability
                                         // through pedals_set_is_available (e.g. after POST)
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_throttle(float throttle) {
    if (!pedals_handler.is_available) {
        return PEDALS_RC_ERROR; // ignore updates if hardware is compromised
    }

    if (throttle < 0.0f || throttle > 1.0f)
        return PEDALS_RC_ERROR; // leave unchanged the last throttle value

    pedals_handler.throttle = throttle;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_brake(float brake) {
    if (!pedals_handler.is_available) {
        return PEDALS_RC_ERROR; // ignore updates if hardware is compromised
    }

    if (brake < 0.0f || brake > 1.0f)
        return PEDALS_RC_ERROR; // leave unchanged the last brake value

    pedals_handler.brake = brake;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_brake_pressure(float brake_pressure) {
    if (!pedals_handler.is_available) {
        return PEDALS_RC_ERROR; // ignore updates if hardware is compromised
    }

    if (brake_pressure < 0.0f || brake_pressure > PEDALS_MAX_BRAKE_PRESSURE_BAR)
        return PEDALS_RC_ERROR; // leave unchanged the last brake pressure value

    pedals_handler.brake_pressure = brake_pressure;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_is_available(bool is_available) {
    pedals_handler.is_available = is_available;
    return PEDALS_RC_OK;
}

float pedals_api_get_requested_throttle_torque() {
    // if pedals are offline, torque must be exactly 0
    if (!pedals_handler.is_available) {
        return 0.0f;
    }

    return PEDALS_MAX_TORQUE_NM * pedals_handler.throttle;
}

bool pedals_api_is_brake_pressed() {
    // if communication is lost, assume the brake NOT pressed
    if (!pedals_handler.is_available) {
        return false;
    }

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

bool pedals_api_get_is_available() {
    return pedals_handler.is_available;
}