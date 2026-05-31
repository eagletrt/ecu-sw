/*!
 * \file as-driver.h
 * \author Dorijan Di Zepp
 * \date 2026-05-30
 * \brief Callback and struct definitions for the Autonomous System (AS) driver.
 * \details This module encapsulates the state variables, sensor values
 * and system-level status flags necessary to govern the autonomous mission.
 */
#ifndef AS_DRIVER_H
#define AS_DRIVER_H

#include <stdbool.h>

/*!
 * \brief Return codes for the AS driver functions and hardware callbacks.
 */
enum ASDriverReturnCode {
    AS_DRIVER_RC_OK,    /*!< Execution completed successfully. */
    AS_DRIVER_RC_ERROR, /*!< General execution or validation error. */
};

/*!
 * \brief Autonomous driving disciplines defined by the FS rulebook.
 */
enum ASDriverMission {
    AS_DRIVER_MISSION_NOT_SELECTED, /*!< Default uninitialized state. Vehicle must not actuate. */
    AS_DRIVER_MISSION_ACCELERATION, /*!< Straight line acceleration event. */
    AS_DRIVER_MISSION_SKIDPAD,      /*!< Figure-eight driving track layout event. */
    AS_DRIVER_MISSION_AUTOCROSS,    /*!< Single lap track layout event. */
    AS_DRIVER_MISSION_TRACKDRIVE,   /*!< Multi-lap endurance track layout event. */
    AS_DRIVER_MISSION_EBS_TEST,     /*!< Dedicated validation protocol for emergency braking. */
    AS_DRIVER_MISSION_INSPECTION,   /*!< Static system check and technical inspection mode. */
};

/*!
 * \brief Distinct air line channels regulating the Emergency Brake System (EBS).
 */
enum ASDriverAirLine {
    AS_DRIVER_AIR_LINE_1, /*!< Primary pneumatic actuation circuit channel. */
    AS_DRIVER_AIR_LINE_2, /*!< Secondary pneumatic actuation circuit channel. */
};

/*!
 * \brief Callback signature to handle mechanical/pneumatic pneumatic venting.
 * \param[in] line Target pneumatic circuit channel to trigger.
 * \retval AS_DRIVER_RC_OK if actuation is deployed.
 * \retval AS_DRIVER_RC_ERROR if actuation failed.
 */
typedef enum ASDriverReturnCode (*air_release_from_line_callback)(enum ASDriverAirLine);

/*!
 * \brief Braking system pressure profiles spanning both pneumatic and hydraulic circuits.
 */
struct ASDriverPressures {
    float brake_pressure_1_1; /*!< Pneumatic supply circuit pressure point 1.1 (bar). */
    float brake_pressure_1_2; /*!< Pneumatic supply circuit pressure point 1.2 (bar). */
    float brake_pressure_1_3; /*!< Pneumatic supply circuit pressure point 1.3 (bar). */
    float brake_pressure_1_4; /*!< Pneumatic supply circuit pressure point 1.4 (bar). */
    float brake_pressure_2_1; /*!< Hydraulic brake caliper line pressure point 2.1 (Front) (bar). */
    float brake_pressure_2_2; /*!< Hydraulic brake caliper line pressure point 2.2 (Rear) (bar). */
};

/*!
 * \brief Structural and mechanical actuator verification sensors.
 */
struct ASDriverMechanicalSensors {
    float bremsweg_1_1;   /*!< Linear travel feedback displacement sensor 1.1 (mm). */
    float bremsweg_1_2;   /*!< Linear travel feedback displacement sensor 1.2 (mm). */
    float bremskraft_3_1; /*!< Actuator force or structural load cell strain gauge (N). */
};

/*!
 * \brief Core state machine handler managing the active AS runtime context.
 */
struct ASDriverHandler {
    enum ASDriverMission as_mission;                     /*!< Current active driverless mission discipline. */
    struct ASDriverPressures pressures;                  /*!< Instantiated profile tracking system pressure data blocks. */
    struct ASDriverMechanicalSensors mechanical_sensors; /*!< Instantiated profile tracking structural displacement metrics. */

    air_release_from_line_callback release_air; /*!< Callback to command pneumatic relief valves. */

    bool is_asms_on;         /*!< Status of the Autonomous System Master Switch physical interface. */
    bool is_mission_started; /*!< Asserts if the vehicle has crossed the startup criteria boundary. */

    bool is_watchdog_worked; /*!< Feedback status indicating the watchdog correctly works after initial checkup. */
    bool is_watchdog_check;  /*!< Flag to indicate if the watchdog needs to be tested or has already been tested during initial checkup. */

    bool is_tsms_on;    /*!< Status of the Tractive System Master Switch physical interface. */
    bool is_sdc_closed; /*!< Monitored state of the high-voltage shutdown circuit physical logic line. */

    bool is_res_go;             /*!< Remote Emergency System active execution 'GO' command packet received. */
    bool is_res_emergency;      /*!< Asserted if the RES has sent an emergency command. */
    bool is_ebs_active;         /*!< Emergency Brake System tripped marker indicating forced pneumatic venting. */
    bool is_vehicle_standstill; /*!< Evaluated dynamics bit derived from wheel nodes to trigger AS_FINISHED. */
};

#endif