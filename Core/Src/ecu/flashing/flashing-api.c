#include <string.h>

#include "flashing-api.h"
#include "watchdogs-api.h"

EAGLETRT_STATIC struct FlashingHandler flashing_handler;

EAGLETRT_STATIC void prv_flashing_on_timeout(void) {
    flashing_handler.is_timed_out = true;
}

enum FlashingReturnCode flashing_api_init(struct WatchdogHandler *watchdog_handler, flashing_system_reset_callback system_reset_callback) {
    if (watchdog_handler == NULL || system_reset_callback == NULL) {
        return FLASHING_RC_NULL_POINTER;
    }

    memset(&flashing_handler, 0U, sizeof(struct FlashingHandler));

    flashing_handler.watchdog_handler = watchdog_handler;
    flashing_handler.system_reset = system_reset_callback;

    constexpr uint32_t flashing_watchdog_timeout = 500U;
    watchdogs_api_init_watchdog(&flashing_handler.watchdog, flashing_watchdog_timeout, prv_flashing_on_timeout);

    return FLASHING_RC_OK;
}

enum FlashingReturnCode flashing_api_perform_reset(void) {
    if (flashing_handler.system_reset == NULL) {
        return FLASHING_RC_NULL_POINTER;
    }

    flashing_handler.system_reset();

    return FLASHING_RC_OK;
}

enum FlashingReturnCode flashing_api_restart_watchdog(void) {
    flashing_handler.is_timed_out = false;

    if (watchdogs_api_watchdog_restart(flashing_handler.watchdog_handler, &flashing_handler.watchdog) != WATCHDOG_RC_OK) {
        return FLASHING_RC_ERROR;
    }

    return FLASHING_RC_OK;
}

enum FlashingReturnCode flashing_api_pet_watchdog(void) {
    if (watchdogs_api_watchdog_pet(flashing_handler.watchdog_handler, &flashing_handler.watchdog) != WATCHDOG_RC_OK) {
        return FLASHING_RC_ERROR;
    }

    return FLASHING_RC_OK;
}

bool flashing_api_is_timed_out(void) {
    return flashing_handler.is_timed_out;
}

bool flashing_api_is_flashing(void) {
    return flashing_handler.is_flashing;
}

enum FlashingReturnCode flashing_api_set_flashing(bool flashing) {
    flashing_handler.is_flashing = flashing;

    return FLASHING_RC_OK;
}
