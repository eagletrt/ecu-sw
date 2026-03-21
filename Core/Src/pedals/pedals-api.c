/*!
 * \file pedals-api.c
 * \author Dorijan Di Zepp
 * \date 2026-03-20
 * \brief Implementation of the pedals module API.
 */

#include "pedals-api.h"
#include "eagletrt-api.h"

EAGLETRT_STATIC struct PedalsHandler pedals_handler;

enum PedalsReturnCode pedals_api_init() {
    pedals_handler.throttle = 0.0f;
    pedals_handler.brake = 0.0f;
    pedals_handler.brake_pressure = 0.0f;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_throttle(float throttle) {
    if (throttle < 0.0f || throttle > 1.0f) {
        return PEDALS_RC_ERROR;
    }

    pedals_handler.throttle = throttle;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_brake(float brake) {
    if (brake < 0.0f || brake > 1.0f) {
        return PEDALS_RC_ERROR;
    }

    pedals_handler.brake = brake;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_set_brake_pressure(float brake_pressure) {
    if (brake_pressure < 0.0f || brake_pressure > PEDALS_MAX_BRAKE_PRESSURE_BAR) {
        return PEDALS_RC_ERROR;
    }

    pedals_handler.brake_pressure = brake_pressure;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_get_requested_throttle_torque(float *out) {
    if (out == NULL) {
        return PEDALS_RC_ERROR;
    }

    *out = PEDALS_MAX_TORQUE_NM * pedals_handler.throttle;

    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_is_brake_pressed(bool *out) {
    if (out == NULL) {
        return PEDALS_RC_ERROR;
    }

    *out = (pedals_handler.brake >= PEDALS_BRAKE_THRESHOLD_PERCENTAGE);

    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_get_throttle(float *out) {
    if (out == NULL) {
        return PEDALS_RC_ERROR;
    }
    *out = pedals_handler.throttle;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_get_brake(float *out) {
    if (out == NULL) {
        return PEDALS_RC_ERROR;
    }
    *out = pedals_handler.brake;
    return PEDALS_RC_OK;
}

enum PedalsReturnCode pedals_api_get_brake_pressure(float *out) {
    if (out == NULL) {
        return PEDALS_RC_ERROR;
    }
    *out = pedals_handler.brake_pressure;
    return PEDALS_RC_OK;
}