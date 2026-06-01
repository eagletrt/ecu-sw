/*!
 * \file post.h
 * \author Dorijan Di Zepp
 * \date 2026-06-01
 * \brief This file defines Power-On Self-Test (POST) structures for system initialization.
 */

#ifndef POST_H
#define POST_H

#include "buzzer.h"
#include "inverters.h"
#include "pedals.h"
#include "raspberry.h"
#include "tractive-system.h"

/*!
 * \brief Return codes for the post module APIs
 */
enum PostReturnCode {
    POST_RC_OK,    /*!< POST completed successfully. */
    POST_RC_ERROR, /*!< POST encountered an error. */
};
//TODO: add callbacks to initialize LOGGER and AS DRIVER (if required)

/*!
 * \brief The configuration passed to the POST module.
 */
struct PostConfig {
    /* --- Buzzer Subsystem Callbacks --- */
    buzzer_on_callback buzzer_on_ptrs[BUZZER_TYPE_COUNT];       /*!< Array of function pointers to activate specific buzzer variations. */
    buzzer_off_callback buzzer_off_ptrs[BUZZER_TYPE_COUNT];     /*!< Array of function pointers to deactivate specific buzzer variations. */
    buzzer_delay_callback buzzer_delay_ptrs[BUZZER_TYPE_COUNT]; /*!< Array of function pointers to execute timed buzzer operations. */
    buzzer_tick_callback buzzer_tick_ptrs[BUZZER_TYPE_COUNT];   /*!< Array of function pointers to poll or update the buzzer tick timers. */

    /* --- Inverters Subsystem Callbacks --- */
    inverters_send_drive_command_callback inverters_send_drive_command; /*!< Callback to dispatch runtime status or drive states to the inverters. */
    inverters_set_torque_callback inverters_set_torque;                 /*!< Callback to update commanded reference torque targets on the inverters. */

    /* --- Raspberry Pi Interface Callbacks --- */
    raspberry_pin_control_callback raspberry_pin_control; /*!< Callback to control physical hardware state pins on the Raspberry Pi interface. */

    /* --- Tractive System (TS) Callbacks --- */
    ts_command_callback ts_send_command; /*!< Callback to transmit high-voltage tractive system orchestration commands. */
};

#endif