#include <string.h>
#include "lights-api.h"
#include "eagletrt.h"

EAGLETRT_STATIC struct LightsHandler lights_handler;

enum LightsReturnCode lights_api_init(lights_set_state_callback set_state) {
    if (set_state == NULL) {
        return LIGHTS_RC_ERROR;
    }

    memset(&lights_handler, 0, sizeof(lights_handler));
    lights_handler.set_state = set_state;
    return LIGHTS_RC_OK;
}

enum LightsReturnCode lights_api_set_light_state(enum LightsName light_name, bool state) {
    if (lights_handler.set_state == NULL) {
        return LIGHTS_RC_ERROR;
    }

    return lights_handler.set_state(light_name, state);
}
