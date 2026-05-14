/*!
 * \file driver.h
 * \author Dorijan Di Zepp
 * \date 2026-05-14
 * \brief Management of driver selection, readiness checks and 
 * continuous safety monitoring.
 */

#ifndef DRIVER_H
#define DRIVER_H

/*!
 * \brief Return codes for the driver module APIs.
 */
enum DriverReturnCode {
    DRIVER_RC_OK,    /*!< Operation completed successfully */
    DRIVER_RC_ERROR, /*!< The requested operation failed or returned a negative outcome */
};

/*!
 * \brief Supported driver types for vehicle control.
 */
enum DriverType {
    DRIVER_TYPE_MANUAL, /*!< Human driver is selected. */
    DRIVER_TYPE_AS,     /*!< Autonomous System (AS) is selected. */
    DRIVER_TYPE_COUNT,  /*!< Sentinel for range validation. */
};

/*!
 * \brief Callback signature to determine if the driver is ready.
 * This function is typically polled while the car is in a pre-drive state 
 * (e.g. IDLE) to determine if transition to R2D is permitted.
 * \retval DRIVER_RC_OK    Driver is ready; all checks passed.
 * \retval DRIVER_RC_ERROR Driver is not ready; checks failed.
 */
typedef enum DriverReturnCode (*driver_wait_for_driver_callback)(void);

/*!
 * \brief Callback signature for continuous safety monitoring.
 * This function is invoked repeatedly during driving states to ensure 
 * operational conditions remain within safety limits.
 * \retval DRIVER_RC_OK    Conditions are safe; continue operation.
 * \retval DRIVER_RC_ERROR Safety violation detected; vehicle should be stopped.
 */
typedef enum DriverReturnCode (*driver_continuous_check_callback)(void);

/*!
 * \brief Handler for the driver control module.
 */
struct DriverHandler {
    enum DriverType driver_type; /*!< Selected driver type. */

    driver_wait_for_driver_callback wait_for_driver; /*!< Readiness check callback. */

    driver_continuous_check_callback continuous_check; /*!< Safety monitoring callback. */
};

#endif