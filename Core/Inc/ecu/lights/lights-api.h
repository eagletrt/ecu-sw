#ifndef LIGHTS_API_H
#define LIGHTS_API_H

#include "lights.h"

/*!
 * \brief Initializes the lights API with the provided callback function.
 *
 * \param set_state A callback function that will be used to set the state of the lights.
 *
 * \retval LIGHTS_RETURN_CODE_OK if the initialization was successful
 * \retval LIGHTS_RETURN_CODE_ERROR if there was an error
 */
enum LightsReturnCode lights_api_init(lights_set_state_callback set_state);

/*!
 * \brief Sets the state of a specific light.
 *
 * \param light_name The name of the light to set the state for.
 * \param state The desired state of the light (true for on, false for off).
 *
 * \retval LIGHTS_RETURN_CODE_OK if the light state was set successfully
 * \retval LIGHTS_RETURN_CODE_ERROR if there was an error setting the light state
 */
enum LightsReturnCode lights_api_set_light_state(enum LightsName light_name, bool state);

#endif // LIGHTS_API_H
