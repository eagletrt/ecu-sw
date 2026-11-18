#ifndef FLASHING_H
#define FLASHING_H

#include "watchdogs.h"
#include "can-primary.h"

constexpr uint32_t flashing_can_min_frame_id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECUFLASHINGTX;
constexpr uint32_t flashing_can_max_frame_id = CAN_PRIMARY_MESSAGE_FRAME_ID_STEERINGBOARDFLASHINGRX;

typedef void (*flashing_system_reset_callback)(void);

enum FlashingReturnCode : uint8_t {
    FLASHING_RC_OK,
    FLASHING_RC_NULL_POINTER,
    FLASHING_RC_ERROR,
};

struct FlashingHandler {
    struct WatchdogHandler *watchdog_handler;
    struct Watchdog watchdog;
    flashing_system_reset_callback system_reset;
    bool is_flashing;
    bool is_timed_out;
};

#endif // FLASHING_H
