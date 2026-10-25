/*!
 * \file fsm.h
 * \date 2026-10-24
 * \authors Thomas Moretti [majorfrost173@gmail.com]
 *
 * \brief The FSM state IDs and other data
 */

#ifndef FSM_H
#define FSM_H

#include <stdint.h>

/*!
 * \brief The amount of time (in ms) the TS ON button should be held pressed for
 */
constexpr uint32_t tson_required_press_time = 2000;

/*!
 * \brief The data needed by the various states of the FSM
 */
struct FsmData {
    uint32_t tick;
};

/*!
 * \brief The ID of all the states of the FSM
 */
enum StateId : uint8_t {
    STATE_INIT,
    STATE_FATAL,
    STATE_IDLE,
    STATE_FLASH,
    STATE_PAUSE,
    STATE_MANUAL_WAIT_TS_PRECHARGE,
    STATE_AS_OFF,
    STATE_WAIT_DRIVER,
    STATE_MANUAL_WAIT_TS_DISCHARGE,
    STATE_MANUAL_WAIT_INV_ENABLE,
    STATE_DRIVING,
    STATE_MANUAL_WAIT_INV_DISABLE,
    STATE_AS_OFF_WAIT_TS_PRECHARGE,
    STATE_AS_READY,
    STATE_AS_READY_WAIT_INV_ENABLE,
    STATE_AS_EMERGENCY,
    STATE_AS_R2D,
    STATE_AS_DRIVING,
    STATE_AS_FINISHED,
    STATE_AS_OFF_WAIT_TS_DISCHARGE,
    STATE_AS_READY_WAIT_INV_DISABLE,
    STATE_AS_FINISHED_WAIT_INV_DISABLE,
    STATE_AS_FINISHED_WAIT_TS_DISCHARGE,
    STATE_AS_EMERGENCY_WAIT_INV_DISABLE,
    STATE_AS_EMERGENCY_WAIT_TS_DISCHARGE,
    STATE_COUNT,
};

/*!
 * \brief Initialise the FSM
 */
void fsm_init();

/*!
 * \brief Run the FSM
 *
 * \param[in, out] data A pointer to some data
 */
void fsm_run(void *data);

#endif // FSM_H
