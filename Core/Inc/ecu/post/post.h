/*!
 * \file post.h
 * \author Dorijan Di Zepp
 * \date 2026-06-25
 * \brief This file defines Power-On Self-Test (POST) structures for system initialization.
 */

#ifndef POST_H
#define POST_H

#include "as-driver.h"
#include "buzzer.h"
#include "can-communication.h"
#include "inverters.h"
#include "lights.h"
#include "raspberry.h"
#include "shutdown.h"
#include "pedals.h"
#include "tsac.h"
#include "vehicle.h"

/*!
 * \brief Return codes for the post module APIs
 */
enum PostReturnCode {
    POST_RC_OK,    /*!< POST completed successfully. */
    POST_RC_ERROR, /*!< POST encountered an error. */
};

/*!
 * \brief The configuration passed to the POST module.
 */
struct PostConfig {
    /* --- AS Driver callbacks --- */
    air_release_from_line_callback as_air_release; /*!< Functional safety callback required to initialize the AS driver state. */

    /* --- Buzzer callbacks --- */
    buzzer_on_callback buzzer_on_ptrs[BUZZER_TYPE_COUNT];       /*!< Array of function pointers to activate specific buzzer variations. */
    buzzer_off_callback buzzer_off_ptrs[BUZZER_TYPE_COUNT];     /*!< Array of function pointers to deactivate specific buzzer variations. */
    buzzer_delay_callback buzzer_delay_ptrs[BUZZER_TYPE_COUNT]; /*!< Array of function pointers to execute timed buzzer operations. */
    buzzer_tick_callback buzzer_tick_ptrs[BUZZER_TYPE_COUNT];   /*!< Array of function pointers to poll or update the buzzer tick timers. */

    /* --- CAN Communication configurations --- */
    struct CanCommunicationNetworkConfig can_networks[CAN_COMMUNICATION_NETWORK_COUNT]; /*!< Explicit configuration blocks for the system's physical CAN networks. */

    /* --- RaspberryPi callbacks --- */
    raspberry_pin_control_callback raspberry_pin_control;  /*!< Callback to control physical hardware state pins on the Raspberry Pi interface. */
    enum RaspberryControlPinState raspberry_initial_state; /*!< Dynamic initialization state requested for the RaspberryPi at boot. */

    /* --- Shutdown callbacks --- */
    shutdown_control_relay_callback shutdown_control_relay; /*!< Callback to set the system's shutdown state. */

    /* --- Pedals callbacks --- */
    pedals_get_tick_callback pedals_get_tick; /*!< Callback to retrieve the current tick count from the pedals module. */

    /* --- TSAC callbacks --- */
    tsac_get_tick_callback tsac_get_tick; /*!< Callback to retrieve the current tick count from the TSAC module. */

    /* --- Inverters callbacks --- */
    inverters_get_tick_callback inverters_get_tick; /*!< Callback to retrieve the current tick count for the inverters RX timeout. */

    /* --- Vehicle callbacks --- */
    vehicle_ptt_control_callback vehicle_ptt_control; /*!< Callback to drive the PTT output pin. */

    /* --- Lights callbacks --- */
    lights_set_state_callback lights_set_state; /*!< Callback to set the state of the vehicle's lights. */
};

#endif
