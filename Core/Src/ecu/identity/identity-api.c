#include "identity-api.h"
#include "can-primary-api.h"
#include "can-version.h"
#include "can-communication-api.h"
#include "eagletrt-api.h"
#include <time.h>

EAGLETRT_STATIC struct IdentityHandler identity_handler;

enum IdentityReturnCode identity_api_init(void) {
    struct tm timeinfo;
    strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &timeinfo);
    identity_handler = (struct IdentityHandler){
        .build_time = mktime(&timeinfo),
        .last_tick_ms_status = 0,
        .last_tick_ms_version = 0,
        .last_tick_ms_libcan_version = 0,
    };
    return IDENTITY_RC_OK;
}

enum IdentityReturnCode identity_api_send_state(enum CanPrimaryEcufsmVehiclestatus vehicle_status, enum CanPrimaryEcufsmKrakenstatus kraken_status) {
    union CanPrimaryMessages message;
    message.ecufsm = (struct CanPrimaryEcufsm){
        .vehiclestatus = vehicle_status,
        .krakenstatus = kraken_status,
    };
    struct CanCommunicationFrame frame;
    if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_ECUFSM, &message, frame.data) != -1) {
        frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECUFSM;
        frame.length = can_primary_byte_size_ecufsm;
        EAGLETRT_API_UNUSED(can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame));
    }
    return IDENTITY_RC_OK;
}

enum IdentityReturnCode identity_api_periodically_send_state(enum CanPrimaryEcufsmVehiclestatus vehicle_status, enum CanPrimaryEcufsmKrakenstatus kraken_status, uint32_t tick_ms) {
    if (tick_ms - identity_handler.last_tick_ms_status >= can_primary_cycle_time_ecufsm) {
        identity_handler.last_tick_ms_status = tick_ms;

        union CanPrimaryMessages message;
        message.ecufsm = (struct CanPrimaryEcufsm){
            .vehiclestatus = vehicle_status,
            .krakenstatus = kraken_status,
        };
        struct CanCommunicationFrame frame;
        if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_ECUFSM, &message, frame.data) != -1) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECUFSM;
            frame.length = can_primary_byte_size_ecufsm;
            EAGLETRT_API_UNUSED(can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame));
        }
    }
    return IDENTITY_RC_OK;
}

enum IdentityReturnCode identity_api_periodically_send_version(uint32_t tick_ms) {
    if (tick_ms - identity_handler.last_tick_ms_version >= can_primary_cycle_time_ecuversion) {
        identity_handler.last_tick_ms_version = tick_ms;

        // version
        union CanPrimaryMessages message;
        message.ecuversion = (struct CanPrimaryEcuversion){
            .major = IDENTITY_VERSION_MAJOR,
            .minor = IDENTITY_VERSION_MINOR,
            .patch = IDENTITY_VERSION_PATCH,
        };
        struct CanCommunicationFrame frame;
        if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_ECUVERSION, &message, frame.data) != -1) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECUVERSION;
            frame.length = can_primary_byte_size_ecuversion;
            EAGLETRT_API_UNUSED(can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame));
        }

        message.ecuversioninfo = (struct CanPrimaryEcuversioninfo){
            .buildtime = identity_handler.build_time,
            .commithash = IDENTITY_VERSION_INFO_COMMIT_HASH,
            .dirty = IDENTITY_VERSION_DIRTY,
        };
        if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_ECUVERSIONINFO, &message, frame.data) != -1) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECUVERSIONINFO;
            frame.length = can_primary_byte_size_ecuversioninfo;
            EAGLETRT_API_UNUSED(can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame));
        }
    }
    return IDENTITY_RC_OK;
}

enum IdentityReturnCode identity_api_periodically_send_libcan_version(uint32_t tick_ms) {
    if (tick_ms - identity_handler.last_tick_ms_libcan_version >= can_primary_cycle_time_eculibcanversion) {
        identity_handler.last_tick_ms_libcan_version = tick_ms;

        // libcan version
        union CanPrimaryMessages message;
        message.eculibcanversion = (struct CanPrimaryEculibcanversion){
            .major = can_version_major,
            .minor = can_version_minor,
            .patch = can_version_patch,
        };
        struct CanCommunicationFrame frame;
        if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_ECULIBCANVERSION, &message, frame.data) != -1) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECULIBCANVERSION;
            frame.length = can_primary_byte_size_eculibcanversion;
            EAGLETRT_API_UNUSED(can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame));
        }

        message.eculibcanversioninfo = (struct CanPrimaryEculibcanversioninfo){
            .commithash = 0,
            .dirty = 0,
            .generationtime = can_generation_time,
        };
        if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_ECULIBCANVERSIONINFO, &message, frame.data) != -1) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECULIBCANVERSIONINFO;
            frame.length = can_primary_byte_size_eculibcanversioninfo;
            EAGLETRT_API_UNUSED(can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame));
        }
    }
    return IDENTITY_RC_OK;
}
