#ifndef LIGHTS_H
#define LIGHTS_H

enum LightsName {
    LIGHTS_NAME_BRAKE,
    LIGHTS_NAME_ASSI_RED,
    LIGHTS_NAME_ASSI_GREEN,
    LIGHTS_NAME_ASSI_BLUE,
    LIGHTS_NAME_ASSI_COUNT,
};

enum LightsReturnCode {
    LIGHTS_RC_OK,
    LIGHTS_RC_ERROR,
};

typedef enum LightsReturnCode (*lights_set_state_callback)(enum LightsName light_name, bool state);

struct LightsHandler {
    lights_set_state_callback set_state;
};

#endif // LIGHTS_H
