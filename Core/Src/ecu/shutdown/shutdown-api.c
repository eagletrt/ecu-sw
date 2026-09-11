#include "shutdown-api.h"
#include "eagletrt.h"
#include <string.h>

EAGLETRT_STATIC struct ShutdownHandler shutdown_handler;

enum ShutdownReturnCode shutdown_api_init(shutdown_control_relay_callback control_relay_callback) {
    if (control_relay_callback == NULL) {
        return SHUTDOWN_RC_ERROR; // Invalid callback
    }
    shutdown_handler.control_relay = control_relay_callback;
    memset(shutdown_handler.voltages, 0, sizeof(shutdown_handler.voltages));
    return SHUTDOWN_RC_OK;
}

enum ShutdownReturnCode shutdown_api_control_relay(bool relay_state) {
    if (shutdown_handler.control_relay == NULL) {
        return SHUTDOWN_RC_ERROR; // Callback not set
    }
    return shutdown_handler.control_relay(relay_state);
}

enum ShutdownReturnCode shutdown_api_set_voltage(enum ShutdownName name, float voltage) {
    if (name >= SHUTDOWN_NAME_COUNT) {
        return SHUTDOWN_RC_ERROR; // Invalid reading position
    }
    shutdown_handler.voltages[name] = voltage;
    return SHUTDOWN_RC_OK;
}

enum ShutdownState shutdown_api_get_state(enum ShutdownName name) {

    if (name >= SHUTDOWN_NAME_COUNT) {
        return SHUTDOWN_STATE_ERROR; // Invalid reading position
    }
    float voltage = shutdown_handler.voltages[name];
    if (voltage < SHUTDOWN_VOLTAGE_THRESHOLD_LOWER) {
        return SHUTDOWN_STATE_OPEN;
    }
    if (voltage > SHUTDOWN_VOLTAGE_THRESHOLD_UPPER) {
        return SHUTDOWN_STATE_CLOSED;
    }
    return SHUTDOWN_STATE_ERROR; // Implausible state
}

float shutdown_api_get_voltage(enum ShutdownName name) {
    if (name >= SHUTDOWN_NAME_COUNT) {
        return 0.0F; // Invalid reading position
    }
    return shutdown_handler.voltages[name];
}
