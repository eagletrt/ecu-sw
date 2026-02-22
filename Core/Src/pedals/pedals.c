/*!
 * \file pedals.c
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Implementation of pedals logic with rx monitoring.
 */

#include "pedals.h"
#include "main.h"
#include <string.h>
#include "eagletrt-api.h"

// change them once the frame IDs are known
#define CAN_ID_PEDALS_THROTTLE 0x123
#define CAN_ID_PEDALS_BRAKE 0x124

EAGLETRT_STATIC struct PedalsHandler pedals_handler;

void pedals_init() {
    memset(&pedals_handler, 0, sizeof(struct PedalsHandler));
    pedals_handler.is_available = false; // assume disconnected until next can frame
}

void pedals_update_from_can(uint32_t id, uint8_t *data) {
    if (data == NULL)
        return;

    // at the moment we assume we have two different can frames
    bool updated = false;

    if (id == CAN_ID_PEDALS_THROTTLE) {
        float th = (float)data[0];
        if (th >= 0.0f && th <= 100.0f) {
            pedals_handler.throttle_pct = th;
            updated = true;
        }
    } else if (id == CAN_ID_PEDALS_BRAKE) {
        float bk = (float)data[0];
        uint16_t raw_ps = (uint16_t)((data[1] << 8) | data[2]);
        float bk_ps = (float)raw_ps;

        if (bk >= 0.0f && bk <= 100.0f) {
            pedals_handler.brake_pct = bk;
            updated = true;
        }
        if (bk_ps <= PEDALS_MAX_BK_PRESSURE) {
            pedals_handler.brake_pressure = bk_ps;
        }
    }

    if (updated) {
        pedals_handler.last_rx_tick = HAL_GetTick();
        pedals_handler.is_available = true;
    }
}

void pedals_rx_timeout() {
    if ((HAL_GetTick() - pedals_handler.last_rx_tick) > PEDALS_TIMEOUT_MS) {
        pedals_handler.is_available = false;
        pedals_handler.throttle_pct = 0.0f; // force zero throttle for safety
    }
}

float pedals_get_requested_throttle_torque() {
    // if pedals are offline, torque must be exactly 0
    if (!pedals_handler.is_available) {
        return 0.0f;
    }

    return PEDALS_MAX_TORQUE * (pedals_handler.throttle_pct / 100.0f);
}

bool pedals_is_brake_pressed() {
    // If communication is lost, assume the brake is NOT pressed
    if (!pedals_handler.is_available) {
        return false;
    }

    return (pedals_handler.brake_pct >= PEDALS_BRAKE_THRESHOLD_PCT);
}

const struct PedalsHandler *pedals_get_values() {
    return &pedals_handler;
}
