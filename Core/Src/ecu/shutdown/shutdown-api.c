#include "shutdown-api.h"
#include "eagletrt.h"
#include <string.h>

EAGLETRT_STATIC struct ShutdownHandler shutdown_handler = { 0 };

enum ShutdownReturnCode shutdown_api_init(shutdown_set_state_callback set_state_callback) {
    memset(&shutdown_handler, 0, sizeof(shutdown_handler));
    shutdown_handler.set_state = set_state_callback;
    return SHUTDOWN_RC_OK;
}

enum ShutdownReturnCode shutdown_api_set_state(enum ShutdownState state) {
    if (shutdown_handler.set_state == NULL || state >= SHUTDOWN_STATE_COUNT) {
        return SHUTDOWN_RC_ERROR;
    }
    return shutdown_handler.set_state(state);
}

enum ShutdownReturnCode shutdown_api_set_reding_state(enum ShutdownReading reading, enum ShutdownState state) {
    if (state >= SHUTDOWN_STATE_COUNT) {
        return SHUTDOWN_RC_ERROR; // Invalid state
    }

    switch (reading) {
        case SHUTDOWN_READING_BEFORE_ECU:
            shutdown_handler.state_before = state;
            break;
        case SHUTDOWN_READING_AFTER_ECU:
            shutdown_handler.state_after = state;
            break;
        default:
            return SHUTDOWN_RC_ERROR; // Invalid reading
    }
    return SHUTDOWN_RC_OK;
}

enum ShutdownState shutdown_api_get_reading_state(enum ShutdownReading reading) {
    switch (reading) {
        case SHUTDOWN_READING_BEFORE_ECU:
            return shutdown_handler.state_before;
        case SHUTDOWN_READING_AFTER_ECU:
            return shutdown_handler.state_after;
        default:
            return SHUTDOWN_STATE_OPEN; // Default to open if reading is invalid
    }
}
