/*!
 * \file controls-api.c
 * \date 2026-10-27
 * \authors Thomas Moretti [majorfrost173@gmail.com]
 *
 * \brief Implementation of the controls module.
 */

#include "controls-api.h"
#include "can-communication-api.h"

#include "eagletrt-api.h"
#include "can-primary-api.h"
#include <string.h>

EAGLETRT_STATIC struct ControlsHandler controls_handler;

enum ControlsReturnCode controls_api_init(void) {
    memset(&controls_handler, 0U, sizeof(struct ControlsHandler));

    return CONTROLS_RC_OK;
}

uint8_t controls_api_get_control_value(enum ControlsName control) {
    // If the specified control is not valid default to 0, otherwise return the current value of the control
    return (control >= CONTROLS_NAME_COUNT ? 0U : controls_handler.values[control]);
}

enum ControlsReturnCode controls_api_set_control_value(enum ControlsName control, uint8_t value) {
    if (control >= CONTROLS_NAME_COUNT) {
        return CONTROLS_RC_ERROR;
    }

    // If the specified control is the launch control then clamp to 1 (because it's a bool), otherwise clamp to the maximum allowed value
    controls_handler.values[control] = EAGLETRT_API_CLAMP(value, 0, (control == CONTROLS_NAME_LAUNCH_CONTROL) ? 1 : controls_api_max_level);

    return CONTROLS_RC_OK;
}

float controls_api_get_torque(enum EphorusWheel wheel) {
    return (wheel >= EPHORUS_WHEEL_COUNT ? .0F : controls_handler.requested_torque_nm[wheel]);
}

enum ControlsReturnCode controls_api_set_torque(enum EphorusWheel wheel, float torque) {
    if (wheel >= EPHORUS_WHEEL_COUNT) {
        return CONTROLS_RC_ERROR;
    }

    controls_handler.requested_torque_nm[wheel] = torque;

    return CONTROLS_RC_OK;
}

enum ControlsStatus controls_api_get_status(void) {
    return controls_handler.status;
}

enum ControlsReturnCode controls_api_set_status(enum ControlsStatus status) {
    if (status >= CONTROLS_STATUS_COUNT) {
        return CONTROLS_RC_ERROR;
    }

    controls_handler.status = status;

    return CONTROLS_RC_OK;
}

// TODO: move to timebase, use the right message
enum ControlsReturnCode controls_api_periodically_send_controls(uint32_t tick_ms) {
    if (tick_ms - controls_handler.last_tick_ms_can_send >= can_primary_cycle_time_controlmapsset) {
        controls_handler.last_tick_ms_can_send = tick_ms;

        union CanPrimaryMessages message = { 0 };
        message.controlmapsset = (struct CanPrimaryControlmapsset){
            .power = controls_api_get_control_value(CONTROLS_NAME_POWER),
            .regen = controls_api_get_control_value(CONTROLS_NAME_REGEN),
            .torquevectoring = controls_api_get_control_value(CONTROLS_NAME_TORQUE_VECTORING),
            .launchcontrol = controls_api_get_control_value(CONTROLS_NAME_LAUNCH_CONTROL),
        };
        struct CanCommunicationFrame frame;

        if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_CONTROLMAPSSET, &message, frame.data) != -1) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_CONTROLMAPSSET;
            frame.length = can_primary_byte_size_controlmapsset;
            EAGLETRT_API_UNUSED(can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame));
        }
    }

    return CONTROLS_RC_OK;
}
