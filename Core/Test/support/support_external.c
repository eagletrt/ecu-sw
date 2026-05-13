/*!
 * \file support_external.c
 * \author Dorijan Di Zepp
 * \date 2026-05-13
 * \brief Global variable stubs for unit testing.
 *
 * \details In the production environment, certain global variables are defined in main.c.
 * Since main.c is excluded from unit tests to avoid hardware dependencies,
 * this file provides the necessary memory definitions for the 
 * linker to resolve 'extern' references used by the modules under test.
 */

#include "ecu_fsm.h"

/*!
 * \brief The global state of the Finite State Machine (FSM).
 * \details This variable represents the current operational state 
 * of the vehicle (e.g., INIT, IDLE, R2D). Modules like driver-api.c use 
 * this variable to verify if certain actions (like re-initialization) 
 * are permitted in the current context.
 * In tests, this can be manually manipulated to simulate different car conditions.
 */
state_t current_state = STATE_INIT;