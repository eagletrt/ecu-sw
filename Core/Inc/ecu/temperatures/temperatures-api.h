#ifndef TEMPERATURES_API_H
#define TEMPERATURES_API_H

#include "temperatures.h"
#include <stdint.h>

/*!
 * \brief Initializes the temperatures API.
 *
 * \return Returns TEMPERATURES_OK if the initialization was successful, or an error code otherwise.
 */
enum TemperaturesReturnCode temperatures_api_init(void);

/*!
 * \brief Updates the temperatures API.
 *
 * \param temperature_name The name of the temperature to update.
 * \param temperature_value The new value of the temperature.
 *
 * \return Returns TEMPERATURES_OK if the update was successful, or an error code otherwise.
 */
enum TemperaturesReturnCode temperatures_api_update(enum TemperaturesName temperature_name, float temperature_value);

/*!
 * \brief Periodically sends the temperatures to the server.
 *
 * \param tick The current tick count.
 *
 * \return Returns TEMPERATURES_OK if the temperatures were sent successfully, or an error code otherwise.
 */
enum TemperaturesReturnCode temperatures_api_periodically_send_temperatures(uint32_t tick);

#endif // TEMPERATURES_API_H
