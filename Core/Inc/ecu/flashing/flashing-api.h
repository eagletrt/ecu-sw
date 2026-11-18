#ifndef FLASHING_API_H
#define FLASHING_API_H

#include "flashing.h"

enum FlashingReturnCode flashing_api_init(struct WatchdogHandler *watchdog_handler, flashing_system_reset_callback system_reset_callback);

enum FlashingReturnCode flashing_api_perform_reset(void);

enum FlashingReturnCode flashing_api_restart_watchdog(void);

enum FlashingReturnCode flashing_api_pet_watchdog(void);

bool flashing_api_is_timed_out(void);

bool flashing_api_is_flashing(void);

enum FlashingReturnCode flashing_api_set_flashing(bool flashing);

#endif // FLASHING_API_H
