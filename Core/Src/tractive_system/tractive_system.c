/*!
 * \file tractive_system.c
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Implementation of Tractive System (TS) state tracking and command execution.
 */

#include "eagletrt-api.h"
#include "tractive_system.h"
#include "can.h"

// using hcan1 as the default interface
#define CAN_INTERFACE &hcan1

/*!
 * \brief Global string array for debugging status names.
 */
const char *TS_state_names[] = { "OFF", "PRECHARGE", "ON", "FATAL", "UNKNOWN" };

EAGLETRT_STATIC struct TsHandler ts_handler;

/*!
 * \brief Raw status codes as defined by the car's CAN protocol.
 */
enum TSRawByte {
    TS_RAW_BYTE_OFF = 0,
    TS_RAW_BYTE_PRECHARGE = 1,
    TS_RAW_BYTE_ON = 2,
    TS_RAW_BYTE_FATAL = 3,
};

/*!
 * \brief CAN wrapper for TS commands.
 * \param command_value The specific instruction byte to be sent (e.g., TS_ON, TS_OFF).
 */
EAGLETRT_STATIC void prv_send_ts_command(enum TSCommand command) {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[1];
    uint32_t tx_mailbox;

    tx_header.StdId = 0x200; // to be changed
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 1;
    tx_header.TransmitGlobalTime = DISABLE;

    tx_data[0] = (uint8_t)command;

    if (HAL_CAN_AddTxMessage(CAN_INTERFACE, &tx_header, tx_data, &tx_mailbox) != HAL_OK) {
        // Log CAN TX error if necessary
    }
}

void TS_init() {
    ts_handler.status = TS_STATUS_UNKNOWN;
    ts_handler.request_on = false;
    ts_handler.request_off = false;

    // for safety send an off command
    prv_send_ts_command(TS_COMMAND_OFF);
}

void TS_update_from_can(uint8_t status_byte) {
    switch (status_byte) {
        case TS_RAW_BYTE_OFF:
            ts_handler.status = TS_STATUS_OFF;
            ts_handler.request_off = false;
            break;
        case TS_RAW_BYTE_ON:
            ts_handler.status = TS_STATUS_ON;
            ts_handler.request_on = false;
            break;
        case TS_RAW_BYTE_PRECHARGE:
            ts_handler.status = TS_STATUS_PRECHARGE;
            break;
        case TS_RAW_BYTE_FATAL:
            ts_handler.status = TS_STATUS_FATAL;
            break;
        default:
            ts_handler.status = TS_STATUS_UNKNOWN;
            break;
    }
}

enum TsStatus TS_get_status() {
    return ts_handler.status;
}

const char *TS_get_state_name(enum TsStatus status) {
    if (status > TS_STATUS_FATAL)
        return TS_state_names[0];
    return TS_state_names[status];
}

void TS_request_power_on() {
    // only allow a request if we are currently OFF or PRECHARGING
    if (ts_handler.status == TS_STATUS_OFF || ts_handler.status == TS_STATUS_PRECHARGE) {
        ts_handler.request_on = true;
        ts_handler.request_off = false;
        prv_send_ts_command(TS_COMMAND_ON);
    }
}

void TS_request_power_off() {
    // power OFF is allowed from any state for safety
    ts_handler.request_off = true;
    ts_handler.request_on = false;
    prv_send_ts_command(TS_COMMAND_OFF);
}

bool TS_is_power_on_requested() {
    return ts_handler.request_on;
}

bool TS_is_power_off_requested() {
    return ts_handler.request_off;
}

void TS_clear_request() {
    ts_handler.request_on = false;
    ts_handler.request_off = false;
}