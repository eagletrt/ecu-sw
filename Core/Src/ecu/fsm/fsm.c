/*!
 * \file fsm.c
 * \date 2026-10-24
 * \authors Thomas Moretti [majorfrost173@gmail.com]
 *
 * \brief The FSM state routines, transition functions, the actual states and the transitions
 */

#include "fsm.h"

#include "as-driver-api.h"
#include "buzzer-api.h"
#include "can-communication-api.h"
#include "can-communication-router-api.h"
#include "inverters-api.h"
#include "logger-api.h"
#include "pedals-api.h"
#include "post-api.h"
#include "raspberry-api.h"
#include "vehicle-api.h"
#include "can-primary.h"
#include "identity-api.h"
#include "shutdown-api.h"
#include "tsac-api.h"
#include "temperatures-api.h"

#include "eagletrt-fsm-api.h"
#include "eagletrt-api.h"

/*!
 * \brief Static handler of the FSM
 */
EAGLETRT_STATIC struct FSMHandler fsm_handler;

/*!
 * \brief List of names of the various states
 */
const char *state_names[STATE_COUNT] = {
    "init",
    "fatal",
    "idle",
    "flash",
    "pause",
    "manual_wait_ts_precharge",
    "as_off",
    "wait_driver",
    "manual_wait_ts_discharge",
    "manual_wait_inv_enable",
    "driving",
    "manual_wait_inv_disable",
    "as_off_wait_ts_precharge",
    "as_ready",
    "as_ready_wait_inv_enable",
    "as_emergency",
    "as_r2d",
    "as_driving",
    "as_finished",
    "as_off_wait_ts_discharge",
    "as_ready_wait_inv_disable",
    "as_finished_wait_inv_disable",
    "as_finished_wait_ts_discharge",
    "as_emergency_wait_inv_disable",
    "as_emergency_wait_ts_discharge",
};

EAGLETRT_STATIC void prv_periodically_send(enum CanPrimaryEcufsmVehiclestatus vehicle_status, enum CanPrimaryEcufsmKrakenstatus kraken_status, uint32_t tick) {
    identity_api_periodically_send_state(vehicle_status, kraken_status, tick);
    identity_api_periodically_send_version(tick);
    identity_api_periodically_send_libcan_version(tick);
    temperatures_api_periodically_send_temperatures(tick);
    shutdown_api_periodically_send_voltages(tick);
}

EAGLETRT_STATIC void prv_drain_can_tx_buffers(void) {
    can_communication_api_process_tx(CAN_COMMUNICATION_NETWORK_PRIMARY);
    can_communication_api_process_tx(CAN_COMMUNICATION_NETWORK_SECONDARY);
    can_communication_api_process_tx(CAN_COMMUNICATION_NETWORK_INVERTER);
}

EAGLETRT_STATIC void prv_drain_can_rx_buffers(void) {
    can_communication_api_process_rx(CAN_COMMUNICATION_NETWORK_PRIMARY);
    can_communication_api_process_rx(CAN_COMMUNICATION_NETWORK_SECONDARY);
    can_communication_api_process_rx(CAN_COMMUNICATION_NETWORK_INVERTER);
}

EAGLETRT_STATIC void prv_step_inverters(void) {
    switch (inverters_api_step()) {
        case INVERTERS_RC_OK:
            break;
        case INVERTERS_RC_TX_ERROR:
            logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Inverter TX error");
            break;
        default:
            logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Inverter error");
            break;
    }
}

EAGLETRT_STATIC void prv_fsm_state_to_can_state(enum StateId state, enum CanPrimaryEcufsmVehiclestatus *vehicle_status, enum CanPrimaryEcufsmKrakenstatus *kraken_status) {
    switch (state) {
        case STATE_INIT:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_INIT;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_INIT;
            break;
        case STATE_FATAL:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_ERROR;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_FATAL;
            break;
        case STATE_IDLE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_IDLE;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_IDLE;
            break;
        case STATE_FLASH:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_IDLE; // idle as by rules, no flash is mentioned
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_FLASH;
            break;
        case STATE_PAUSE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_IDLE; // idle as by rules, no pause is mentioned
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_PAUSE;
            break;
        case STATE_MANUAL_WAIT_TS_PRECHARGE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_PRECHARGE;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_MANUAL_WAIT_TS_PRECHARGE;
            break;
        case STATE_AS_OFF:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_ASOFF;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_OFF;
            break;
        case STATE_WAIT_DRIVER:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_TSON;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_WAIT_DRIVER;
            break;
        case STATE_MANUAL_WAIT_TS_DISCHARGE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_DISCHARGE;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_MANUAL_WAIT_TS_DISCHARGE;
            break;
        case STATE_MANUAL_WAIT_INV_ENABLE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_TSON;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_MANUAL_WAIT_INV_ENABLE;
            break;
        case STATE_DRIVING:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_TSON;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_DRIVING;
            break;
        case STATE_MANUAL_WAIT_INV_DISABLE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_TSON;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_MANUAL_WAIT_INV_DISABLE;
            break;
        case STATE_AS_OFF_WAIT_TS_PRECHARGE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_PRECHARGE;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_OFF_WAIT_TS_PRECHARGE;
            break;
        case STATE_AS_READY:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_READY;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_READY;
            break;
        case STATE_AS_READY_WAIT_INV_ENABLE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_R2D;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_READY_WAIT_INV_ENABLE;
            break;
        case STATE_AS_EMERGENCY:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_EMERGENCY;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_EMERGENCY;
            break;
        case STATE_AS_R2D:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_R2D;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_R2D;
            break;
        case STATE_AS_DRIVING:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_DRIVING;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_DRIVING;
            break;
        case STATE_AS_FINISHED:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_FINISHED;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_FINISHED;
            break;
        case STATE_AS_OFF_WAIT_TS_DISCHARGE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_DISCHARGE;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_OFF_WAIT_TS_DISCHARGE;
            break;
        case STATE_AS_READY_WAIT_INV_DISABLE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_R2D;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_READY_WAIT_INV_DISABLE;
            break;
        case STATE_AS_FINISHED_WAIT_INV_DISABLE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_FINISHED;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_FINISHED_WAIT_INV_DISABLE;
            break;
        case STATE_AS_FINISHED_WAIT_TS_DISCHARGE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_FINISHED;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_FINISHED_WAIT_TS_DISCHARGE;
            break;
        case STATE_AS_EMERGENCY_WAIT_INV_DISABLE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_EMERGENCY;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_EMERGENCY_WAIT_INV_DISABLE;
            break;
        case STATE_AS_EMERGENCY_WAIT_TS_DISCHARGE:
            *vehicle_status = CAN_PRIMARY_ECUFSM_VEHICLESTATUS_AS_EMERGENCY;
            *kraken_status = CAN_PRIMARY_ECUFSM_KRAKENSTATUS_AS_EMERGENCY_WAIT_TS_DISCHARGE;
            break;
        default:
            logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Unknown state %d", state);
            break;
    }
}

EAGLETRT_STATIC void prv_log_and_periodical_can(enum StateId state, uint32_t tick) {
    EAGLETRT_STATIC enum StateId last_logged = STATE_COUNT; // sentinel: nothing logged yet
    enum CanPrimaryEcufsmVehiclestatus vehicle_status;
    enum CanPrimaryEcufsmKrakenstatus kraken_status;
    prv_fsm_state_to_can_state(state, &vehicle_status, &kraken_status);
    if (state != last_logged) {
        identity_api_send_state(vehicle_status, kraken_status);
        logger_api_log(LOGGER_LEVEL_INFO, "FSM: entered %s state", state_names[state]);
        last_logged = state;
    }
    prv_periodically_send(vehicle_status, kraken_status, tick);
}

/*!
 * \brief Init state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_init(void *data) {
    prv_log_and_periodical_can(STATE_INIT, 0);

    // convert state data into POST struct configuration
    struct PostConfig *post_configuration = (struct PostConfig *)data;

    if (post_api_do_init(post_configuration) != POST_RC_OK) {
        // Error during POST initialization
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: POST failed. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
    } else {
        constexpr uint32_t frequency = 1000;
        constexpr float amplitude = 0.33F;
        constexpr uint32_t duration = 800;
        buzzer_api_set_frequency(BUZZER_TYPE_ASSI, frequency);
        buzzer_api_set_amplitude(BUZZER_TYPE_ASSI, amplitude);
        buzzer_api_set_duration(BUZZER_TYPE_ASSI, duration);
        buzzer_api_play_sync(BUZZER_TYPE_ASSI);
    }
}

/*!
 * \brief Fatal state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_fatal(void *data) {
    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL.");
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    // fatal state is a sink, no other operation should be made
    prv_log_and_periodical_can(STATE_FATAL, fsm_data.tick);

    // prevent the tractive system from being enabled in case of a fatal error
    shutdown_api_control_relay(false);
}

/*!
 * \brief Idle state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_idle(void *data) {
    EAGLETRT_STATIC uint32_t tson_first_press_tick = 0;

    constexpr uint32_t ready_buzzer_duration = 1000;

    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_IDLE, fsm_data.tick);

    prv_drain_can_rx_buffers();

    prv_step_inverters();

    // this logic does:
    // 1. If the TS ON button is pressed and this is the first time it is pressed, record the current tick.
    // 2. If the TS ON button is released, reset the recorded tick to 0.
    // 3. If the TS ON button has been pressed for more than 2 seconds, check if the voltage is lower than 60V. If it is, transition to the PRECHARGE state. If not, log an error and transition to the FATAL state
    if (vehicle_api_get_ts_on_button_pressed() && tson_first_press_tick == 0) {
        tson_first_press_tick = fsm_data.tick;
    } else if (!vehicle_api_get_ts_on_button_pressed()) {
        tson_first_press_tick = 0;
    } else if (fsm_data.tick - tson_first_press_tick > tson_required_press_time) {
        if (tsac_api_is_tsac_status_timeout()) {
            logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Aborting Precharge. TSAC status timeout!");
            shutdown_api_control_relay(false);
        } else if (!tsac_api_get_voltage_higher_than_60v()) {
            logger_api_log(LOGGER_LEVEL_INFO, "FSM: TS ON requested. Moving to PRECHARGE.");
            shutdown_api_control_relay(true);
            buzzer_api_set_duration(BUZZER_TYPE_ASSI, ready_buzzer_duration);
            buzzer_api_request(BUZZER_TYPE_ASSI);
            fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_PRECHARGE);
        } else {
            logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Aborting Precharge. DC Link already >60V!");
            fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        }
        tson_first_press_tick = 0;
    }

    tsac_api_periodically_require_tsac_status();

    prv_drain_can_tx_buffers();
}

/*!
 * \brief Flash state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_flash(void *data) {
    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_FLASH, fsm_data.tick);

    prv_drain_can_rx_buffers();
}

/*!
 * \brief Pause state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_pause(void *data) {
    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_PAUSE, fsm_data.tick);
}

/*!
 * \brief Manual wait TS precharge state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_manual_wait_ts_precharge(void *data) {
    // If the inverters never all reach "drive" within this window, stop waiting and
    // unwind back toward idle instead of hanging.
    constexpr uint32_t precharge_timeout_ms = 10000;
    EAGLETRT_STATIC uint32_t enter_tick = 0;
    EAGLETRT_STATIC bool precharge_started = false;

    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_MANUAL_WAIT_TS_PRECHARGE, fsm_data.tick);

    prv_drain_can_rx_buffers();

    prv_step_inverters();

    if (enter_tick == 0) {
        enter_tick = fsm_data.tick;
    }

    enum CanPrimaryTsacstatusMainboardstatus tsac_status = tsac_api_get_tsac_status();

    // Logic is:
    // 1. If the TSAC status times out, log an error and transition to the TS DISCHARGE state.
    // 2. If the shutdown is open, log an error and transition to the TS DISCHARGE state.
    // 3. If the TSAC status is TSON and the voltage is higher than 60V, log an info message and transition to the WAIT DRIVER state.
    // 4. If the TSAC status is ERROR, log an error and transition to the TS DISCHARGE state.
    // 5. If the precharge has not completed within the timeout, log an error and transition to the TS DISCHARGE state.
    // 6. If the TSAC status is AIRN_CHECK, PRECHARGE, or AIRP_CHECK, and precharge has not started yet, log an info message and set precharge_started to true.
    // 7. If precharge has started and the TSAC status is IDLE or INIT, log an error and transition to the TS DISCHARGE state.
    if (tsac_api_is_tsac_status_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Precharge failed. TSAC status timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (shutdown_api_get_state(SHUTDOWN_NAME_AFTER_ECU) != SHUTDOWN_STATE_CLOSED) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Aborting Precharge. Shutdown is open!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (tsac_api_get_tsac_status() == CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_TS_ON && tsac_api_get_voltage_higher_than_60v()) {
        logger_api_log(LOGGER_LEVEL_INFO, "FSM: Precharge completed. Moving to WAIT_DRIVER.");
        fsm_api_transition_to(&fsm_handler, STATE_WAIT_DRIVER);
    } else if (tsac_api_get_tsac_status() == CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_ERROR) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Precharge failed. TSAC reported error!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (fsm_data.tick - enter_tick > precharge_timeout_ms) {
        // The precharge did not happen within the timeout: give up and unwind.
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Precharge outside timeout.");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (tsac_status == CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_AIRN_CHECK ||
               tsac_status == CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_PRECHARGE ||
               tsac_status == CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_AIRP_CHECK) {
        if (!precharge_started) {
            logger_api_log(LOGGER_LEVEL_INFO, "FSM: TSAC precharge started.");
            precharge_started = true;
        }
    } else if (precharge_started &&
               (tsac_status == CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_IDLE ||
                tsac_status == CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_INIT)) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Precharge stopped by TSAC. Aborting.");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    }

    if (fsm_handler.requested_state != STATE_MANUAL_WAIT_TS_PRECHARGE) {
        buzzer_api_reset(BUZZER_TYPE_R2D);
        enter_tick = 0;
        precharge_started = false;
    }

    tsac_api_periodically_require_tsac_status();

    prv_drain_can_tx_buffers();
}

/*!
 * \brief AS off state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_off(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief Wait driver state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_wait_driver(void *data) {
    EAGLETRT_STATIC uint32_t tson_first_press_tick = 0;
    EAGLETRT_STATIC bool state_entered = false;
    EAGLETRT_STATIC bool ignore_button_until_release = false;

    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_WAIT_DRIVER, fsm_data.tick);

    prv_drain_can_rx_buffers();

    prv_step_inverters();

    bool button_pressed = vehicle_api_get_ts_on_button_pressed();
    if (!state_entered) {
        state_entered = true;
        ignore_button_until_release = button_pressed;
    }
    if (ignore_button_until_release && !button_pressed) {
        ignore_button_until_release = false;
    }
    bool button_active = button_pressed && !ignore_button_until_release;

    // This logic does:
    // 1. If the TSAC status times out, log an error and transition to the TS DISCHARGE state.
    // 2. If the TSAC status is not TSON, log an error and transition to the TS DISCHARGE state.
    // 3. If the TS ON button is pressed without the brake pedal being pressed, transition to the TS DISCHARGE state.
    // 4. If the TS ON button is pressed and this is the first time it is pressed, record the current tick.
    // 5. If the TS ON button is released, reset the recorded tick to 0.
    // 6. If the TS ON button has been pressed for more than 2 seconds and the brake pedal is pressed, transition to the INV ENABLE state.
    if (tsac_api_is_tsac_status_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: TSAC status timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (tsac_api_get_tsac_status() != CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_TS_ON) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: TSAC not in TSON!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (button_active && !pedals_api_is_brake_pressed()) {
        logger_api_log(LOGGER_LEVEL_INFO, "FSM: TS ON pressed without pedal. Moving to TS DISCHARGE.");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (button_active && tson_first_press_tick == 0) {
        tson_first_press_tick = fsm_data.tick;
    } else if (!button_active) {
        tson_first_press_tick = 0;
    } else if (fsm_data.tick - tson_first_press_tick > tson_required_press_time && pedals_api_is_brake_pressed()) {
        logger_api_log(LOGGER_LEVEL_INFO, "FSM: TS ON and pedal pressed. Moving to INV ENABLE.");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_ENABLE);
    }

    if (fsm_handler.requested_state != STATE_WAIT_DRIVER /* TODO: whu? && next_state != STATE_WAIT_DRIVER */) {
        tson_first_press_tick = 0;
        state_entered = false;
        ignore_button_until_release = false;
    }

    tsac_api_periodically_require_tsac_status();
    prv_drain_can_tx_buffers();
}

/*!
 * \brief Manual wait TS discharge state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_manual_wait_ts_discharge(void *data) {
    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_MANUAL_WAIT_TS_DISCHARGE, fsm_data.tick);

    prv_drain_can_rx_buffers();

    prv_step_inverters();

    // The logic here is:
    // 1. If the TSAC voltage is higher than 60 V and the status isn't timed out, log a success and transition to the IDLE state.
    // 2. If the TSAC status times out, log an error, open the shutdown relay and transition to the IDLE state.
    if (!tsac_api_get_voltage_higher_than_60v() && !tsac_api_is_tsac_status_timeout()) {
        logger_api_log(LOGGER_LEVEL_INFO, "FSM: TS DISCHARGE completed. Moving to IDLE.");
        fsm_api_transition_to(&fsm_handler, STATE_IDLE);
    } else if (tsac_api_is_tsac_status_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: TS DISCHARGE failed. TSAC status timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_IDLE);
    }

    tsac_api_periodically_require_tsac_status();
    prv_drain_can_tx_buffers();
}

/*!
 * \brief Manual wait inverters enable state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_manual_wait_inv_enable(void *data) {
    // If the inverters never all reach "drive" within this window, stop waiting and
    // unwind back toward idle instead of hanging.
    constexpr uint32_t inverter_enable_timeout_ms = 2000;
    EAGLETRT_STATIC uint32_t enter_tick = 0;

    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_MANUAL_WAIT_INV_ENABLE, fsm_data.tick);

    // Latch the entry tick on the first cycle spent in this state.
    if (enter_tick == 0) {
        enter_tick = fsm_data.tick;
    }

    prv_drain_can_rx_buffers();
    // Keep commanding all four inverters toward their armed/drive state while we wait.
    inverters_api_arm(EPHORUS_WHEEL_FRONT_LEFT);
    inverters_api_arm(EPHORUS_WHEEL_FRONT_RIGHT);
    inverters_api_arm(EPHORUS_WHEEL_REAR_LEFT);
    inverters_api_arm(EPHORUS_WHEEL_REAR_RIGHT);

    prv_step_inverters();

    // Decision order (first match wins):
    //   1. If the TSAC status times out, abort and disarm.
    //   2. The tractive system must stay in TS_ON with a live TSAC link; otherwise
    //      abort the enable and disarm.
    //   3. If the inverter link has timed out, abort and disarm.
    //   4. Once every inverter reports "drive", play the R2D tone once and hand over
    //      to DRIVING when the tone (serviced by the main-loop buzzer poll) finishes.
    //   5. If the tone had already started (we were in drive) but the inverters fell
    //      back out of drive, abort and disarm.
    //   6. If the inverters simply never reach drive within the timeout, abort and
    //      disarm so we fall back toward idle rather than waiting forever.
    if (tsac_api_is_tsac_status_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: TSAC status timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (tsac_api_get_tsac_status() != CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_TS_ON) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: TSAC not in TSON!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (inverters_api_is_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Inverter comms timeout!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (inverters_api_is_all_in_drive()) {
        // Request the R2D tone the first time we see full drive, then wait for the
        // buzzer poll to report DONE.
        enum BuzzerPlayState r2d_state = buzzer_api_get_play_state(BUZZER_TYPE_R2D);
        switch (r2d_state) {
            case BUZZER_PLAY_STATE_IDLE: {
                buzzer_api_set_duration(BUZZER_TYPE_R2D, BUZZER_RD2_SOUND_DURATION_MS);
                buzzer_api_request(BUZZER_TYPE_R2D);
                break;
            }
            case BUZZER_PLAY_STATE_DONE: {
                logger_api_log(LOGGER_LEVEL_INFO, "FSM: INV ENABLE completed. Moving to DRIVING.");
                fsm_api_transition_to(&fsm_handler, STATE_DRIVING);
                break;
            }
            default: {
                // The R2D tone is still playing, so stay in this state until it finishes.
                break;
            }
        }
    } else if (buzzer_api_get_play_state(BUZZER_TYPE_R2D) != BUZZER_PLAY_STATE_IDLE) {
        // The R2D tone had already started (we had reached drive) but the inverters
        // left drive before it finished: abort the enable.
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Inverters left drive during enable!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (fsm_data.tick - enter_tick > inverter_enable_timeout_ms) {
        // The inverters never reached drive within the timeout: give up and unwind.
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Inverter enable timeout. Aborting.");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    }

    // On any transition out, silence the R2D tone and re-arm the entry timer.
    if (fsm_handler.requested_state != STATE_MANUAL_WAIT_INV_ENABLE) {
        buzzer_api_reset(BUZZER_TYPE_R2D);
        enter_tick = 0;
    }

    tsac_api_periodically_require_tsac_status();
    prv_drain_can_tx_buffers();
}

/*!
 * \brief Driving state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_driving(void *data) {
    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_DRIVING, fsm_data.tick);

    prv_drain_can_rx_buffers();

    prv_step_inverters();

    // Logic is:
    // 1. If the TS ON button is pressed, abort and disarm.
    // 2. If the inverter link has timed out, abort and disarm.
    // 3. If any inverter is not in drive, abort and disarm, and open the shutdown relay to discharge the TS.
    // 4. If the TSAC status times out, abort and disarm, and open the shutdown relay to discharge the TS.
    // 5. If the TSAC status is not in TSON, abort and disarm.
    // 6. If the pedals timeout, abort and disarm.
    // 7. If the shutdown relay is open, abort and disarm.
    // 8. Otherwise, read the requested torque from the pedals and command it to all four inverters.
    if (vehicle_api_get_ts_on_button_pressed()) {
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (inverters_api_is_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Inverter comms timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (!inverters_api_is_all_in_drive()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Not all inverters in drive!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (tsac_api_is_tsac_status_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: TSAC status timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (tsac_api_get_tsac_status() != CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_TS_ON) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: TSAC not in TSON!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (pedals_api_is_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Pedal timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else if (shutdown_api_get_state(SHUTDOWN_NAME_AFTER_ECU) != SHUTDOWN_STATE_CLOSED) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Shutdown is open!");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_INV_DISABLE);
    } else {
        float requested_torque = pedals_api_get_requested_throttle_torque();
        inverters_api_set_torque(EPHORUS_WHEEL_REAR_LEFT, requested_torque);
        inverters_api_set_torque(EPHORUS_WHEEL_REAR_RIGHT, requested_torque);
        inverters_api_set_torque(EPHORUS_WHEEL_FRONT_LEFT, requested_torque);
        inverters_api_set_torque(EPHORUS_WHEEL_FRONT_RIGHT, requested_torque);
    }

    tsac_api_periodically_require_tsac_status();
    prv_drain_can_tx_buffers();
}

/*!
 * \brief Manual wait inverters disable state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_manual_wait_inv_disable(void *data) {
    if (data == NULL) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: State data is NULL. Going to FATAL");
        fsm_api_transition_to(&fsm_handler, STATE_FATAL);
        return;
    }
    struct FsmData fsm_data = *(struct FsmData *)data;

    prv_log_and_periodical_can(STATE_MANUAL_WAIT_INV_DISABLE, fsm_data.tick);

    prv_drain_can_rx_buffers();

    prv_step_inverters();

    // The logic here is:
    // 1. If any inverter is not in drive, log a success and move to the TS DISCHARGE state.
    // 2. If the inverter link has timed out, log an error, open the shutdown relay and transition to the TS DISCHARGE state.
    // 3. Otherwise, command all four inverters to disarm.
    if (!inverters_api_is_all_in_drive()) {
        logger_api_log(LOGGER_LEVEL_INFO, "FSM: INV DISABLE completed. Moving to TS DISCHARGE.");
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else if (inverters_api_is_timeout()) {
        logger_api_log(LOGGER_LEVEL_ERROR, "FSM: Inverter comms timeout!");
        shutdown_api_control_relay(false);
        fsm_api_transition_to(&fsm_handler, STATE_MANUAL_WAIT_TS_DISCHARGE);
    } else {
        inverters_api_disarm(EPHORUS_WHEEL_FRONT_LEFT);
        inverters_api_disarm(EPHORUS_WHEEL_FRONT_RIGHT);
        inverters_api_disarm(EPHORUS_WHEEL_REAR_LEFT);
        inverters_api_disarm(EPHORUS_WHEEL_REAR_RIGHT);
    }

    tsac_api_periodically_require_tsac_status();
    prv_drain_can_tx_buffers();
}

/*!
 * \brief AS off wait TS precharge state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_off_wait_ts_precharge(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS ready state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_ready(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS ready wait inverters enable state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_ready_wait_inv_enable(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS emergency state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_emergency(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS R2D state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_r2d(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS driving state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_driving(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS finished state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_finished(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS off wait TS discharge state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_off_wait_ts_discharge(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS ready wait inverters disable state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_ready_wait_inv_disable(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS finished wait inverters disable state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_finished_wait_inv_disable(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS finished wait TS discharge state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_finished_wait_ts_discharge(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS emergency wait inverters disable state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_emergency_wait_inv_disable(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief AS emergency wait TS discharge state routine
 *
 * \param[in, out] data A pointer to some data
 */
void routine_as_emergency_wait_ts_discharge(void *data) {
    EAGLETRT_API_UNUSED(data);
}

/*!
 * \brief Start TS precharge transition function
 *
 * \param[in, out] data A pointer to some data
 */
void start_ts_precharge(void *data) {
    EAGLETRT_API_UNUSED(data);

    tsac_api_set_ts_state_to_require(true);
}

/*!
 * \brief Start TS discharge transition function
 *
 * \param[in, out] data A pointer to some data
 */
void start_ts_discharge(void *data) {
    EAGLETRT_API_UNUSED(data);

    tsac_api_set_ts_state_to_require(false);
}

/*!
 * \brief Start inverters enable transition function
 *
 * \param[in, out] data A pointer to some data
 */
void start_inv_enable(void *data) {
    EAGLETRT_API_UNUSED(data);

    inverters_api_arm(EPHORUS_WHEEL_FRONT_LEFT);
    inverters_api_arm(EPHORUS_WHEEL_FRONT_RIGHT);
    inverters_api_arm(EPHORUS_WHEEL_REAR_LEFT);
    inverters_api_arm(EPHORUS_WHEEL_REAR_RIGHT);
}

/*!
 * \brief Start inverters disable transition function
 *
 * \param[in, out] data A pointer to some data
 */
void start_inv_disable(void *data) {
    EAGLETRT_API_UNUSED(data);

    inverters_api_disarm(EPHORUS_WHEEL_FRONT_LEFT);
    inverters_api_disarm(EPHORUS_WHEEL_FRONT_RIGHT);
    inverters_api_disarm(EPHORUS_WHEEL_REAR_LEFT);
    inverters_api_disarm(EPHORUS_WHEEL_REAR_RIGHT);
}

/*!
 * \brief List of the possible transitions from init
 */
struct Transition init_transitions[] = {
    { .to = STATE_IDLE, .function = NULL },
    { .to = STATE_FATAL, .function = NULL },
};

/*!
 * \brief List of the possible transitions from fatal
 */
struct Transition fatal_transitions[] = {
    { .to = STATE_FATAL, .function = NULL },
};

/*!
 * \brief List of the possible transitions from idle
 */
struct Transition idle_transitions[] = {
    { .to = STATE_FATAL, .function = NULL },
    { .to = STATE_IDLE, .function = NULL },
    { .to = STATE_FLASH, .function = NULL },
    { .to = STATE_PAUSE, .function = NULL },
    { .to = STATE_MANUAL_WAIT_TS_PRECHARGE, .function = start_ts_precharge },
    { .to = STATE_AS_OFF, .function = NULL },
};

/*!
 * \brief List of the possible transitions from flash
 */
struct Transition flash_transitions[] = {
    { .to = STATE_IDLE, .function = NULL },
    { .to = STATE_FLASH, .function = NULL },
};

/*!
 * \brief List of the possible transitions from pause
 */
struct Transition pause_transitions[] = {
    { .to = STATE_IDLE, .function = NULL },
    { .to = STATE_PAUSE, .function = NULL },
};

/*!
 * \brief List of the possible transitions from manual wait TS precharge
 */
struct Transition manual_wait_ts_precharge_transitions[] = {
    { .to = STATE_MANUAL_WAIT_TS_PRECHARGE, .function = NULL },
    { .to = STATE_WAIT_DRIVER, .function = NULL },
    { .to = STATE_MANUAL_WAIT_TS_DISCHARGE, .function = start_ts_discharge }
};

/*!
 * \brief List of the possible transitions from AS off
 */
struct Transition as_off_transitions[] = {
    { .to = STATE_IDLE, .function = NULL },
    { .to = STATE_FATAL, .function = NULL },
    { .to = STATE_AS_OFF, .function = NULL },
    { .to = STATE_AS_OFF_WAIT_TS_PRECHARGE, .function = start_ts_precharge },
};

/*!
 * \brief List of the possible transitions from wait driver
 */
struct Transition wait_driver_transitions[] = {
    { .to = STATE_WAIT_DRIVER, .function = NULL },
    { .to = STATE_MANUAL_WAIT_TS_DISCHARGE, .function = start_ts_discharge },
    { .to = STATE_MANUAL_WAIT_INV_ENABLE, .function = start_inv_enable },
};

/*!
 * \brief List of the possible transitions from manual wait TS discharge
 */
struct Transition manual_wait_ts_discharge_transitions[] = {
    { .to = STATE_IDLE, .function = NULL },
    { .to = STATE_MANUAL_WAIT_TS_DISCHARGE, .function = NULL },
};

/*!
 * \brief List of the possible transitions from manual wait inverters enable
 */
struct Transition manual_wait_inv_enable_transitions[] = {
    { .to = STATE_MANUAL_WAIT_INV_ENABLE, .function = NULL },
    { .to = STATE_DRIVING, .function = NULL },
    { .to = STATE_MANUAL_WAIT_INV_DISABLE, .function = start_inv_disable },
};

/*!
 * \brief List of the possible transitions from driving
 */
struct Transition driving_transitions[] = {
    { .to = STATE_DRIVING, .function = NULL },
    { .to = STATE_MANUAL_WAIT_INV_DISABLE, .function = start_inv_disable },
};

/*!
 * \brief List of the possible transitions from manual wait inverters disable
 */
struct Transition manual_wait_inv_disable_transitions[] = {
    { .to = STATE_MANUAL_WAIT_TS_DISCHARGE, .function = start_ts_discharge },
    { .to = STATE_MANUAL_WAIT_INV_DISABLE, .function = NULL },
};

/*!
 * \brief List of the possible transitions from AS off wait TS precharge
 */
struct Transition as_off_wait_ts_precharge_transitions[] = {
    { .to = STATE_AS_OFF_WAIT_TS_PRECHARGE, .function = NULL },
    { .to = STATE_AS_READY, .function = NULL },
    { .to = STATE_AS_OFF_WAIT_TS_DISCHARGE, .function = start_ts_discharge },
};

/*!
 * \brief List of the possible transitions from AS ready
 */
struct Transition as_ready_transitions[] = {
    { .to = STATE_AS_READY, .function = NULL },
    { .to = STATE_AS_READY_WAIT_INV_ENABLE, .function = start_inv_enable },
    { .to = STATE_AS_EMERGENCY, .function = NULL },
};

/*!
 * \brief List of the possible transitions from AS ready wait inverters enable
 */
struct Transition as_ready_wait_inv_enable_transitions[] = {
    { .to = STATE_AS_READY_WAIT_INV_ENABLE, .function = NULL },
    { .to = STATE_AS_R2D, .function = NULL },
    { .to = STATE_AS_READY_WAIT_INV_DISABLE, .function = start_inv_disable },
};

/*!
 * \brief List of the possible transitions from AS emergency
 */
struct Transition as_emergency_transitions[] = {
    { .to = STATE_AS_EMERGENCY, .function = NULL },
    { .to = STATE_AS_EMERGENCY_WAIT_INV_DISABLE, .function = start_inv_disable },
};

/*!
 * \brief List of the possible transitions from AS R2D
 */
struct Transition as_r2d_transitions[] = {
    { .to = STATE_AS_EMERGENCY, .function = NULL },
    { .to = STATE_AS_DRIVING, .function = NULL },
};

/*!
 * \brief List of the possible transitions from AS driving
 */
struct Transition as_driving_transitions[] = {
    { .to = STATE_AS_DRIVING, .function = NULL },
    { .to = STATE_AS_EMERGENCY, .function = NULL },
    { .to = STATE_AS_FINISHED, .function = NULL },
};

/*!
 * \brief List of the possible transitions from AS finished
 */
struct Transition as_finished_transitions[] = {
    { .to = STATE_AS_FINISHED, .function = NULL },
    { .to = STATE_AS_FINISHED_WAIT_INV_DISABLE, .function = start_inv_disable },
};

/*!
 * \brief List of the possible transitions from AS off
 */
struct Transition as_off_wait_ts_discharge_transitions[] = {
    { .to = STATE_AS_OFF, .function = NULL },
    { .to = STATE_AS_OFF_WAIT_TS_DISCHARGE, .function = NULL },
};

/*!
 * \brief List of the possible transitions from AS ready wait inverters disable
 */
struct Transition as_ready_wait_inv_disable_transitions[] = {
    { .to = STATE_AS_EMERGENCY, .function = NULL },
    { .to = STATE_AS_READY_WAIT_INV_DISABLE, .function = NULL },
};

/*!
 * \brief List of the possible transitions from AS finished wait inverters disable
 */
struct Transition as_finished_wait_inv_disable_transitions[] = {
    { .to = STATE_AS_FINISHED_WAIT_INV_DISABLE, .function = NULL },
    { .to = STATE_AS_FINISHED_WAIT_TS_DISCHARGE, .function = start_ts_discharge },
};

/*!
 * \brief List of the possible transitions from AS finished wait TS discharge
 */
struct Transition as_finished_wait_ts_discharge_transitions[] = {
    { .to = STATE_AS_FINISHED, .function = NULL },
    { .to = STATE_AS_FINISHED_WAIT_TS_DISCHARGE, .function = NULL },
};

/*!
 * \brief List of the possible transitions from AS emergency wait inverters disable
 */
struct Transition as_emergency_wait_inv_disable_transitions[] = {
    { .to = STATE_AS_EMERGENCY_WAIT_INV_DISABLE, .function = NULL },
    { .to = STATE_AS_EMERGENCY_WAIT_TS_DISCHARGE, .function = start_ts_discharge },
};

/*!
 * \brief List of the possible transitions from AS emergency wait TS discharge
 */
struct Transition as_emergency_wait_ts_discharge_transitions[] = {
    { .to = STATE_AS_EMERGENCY, .function = NULL },
    { .to = STATE_AS_EMERGENCY_WAIT_TS_DISCHARGE, .function = NULL },
};

/*!
 * \brief List of all the states and information about each of them
 */
const struct State states[] = {
    [STATE_INIT] = {
        .id = STATE_INIT,
        .function = routine_init,
        .next_default = STATE_IDLE,
        .transitions = init_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(init_transitions),
    },
    [STATE_FATAL] = {
        .id = STATE_FATAL,
        .function = routine_fatal,
        .next_default = STATE_FATAL,
        .transitions = fatal_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(fatal_transitions),
    },
    [STATE_IDLE] = {
        .id = STATE_IDLE,
        .function = routine_idle,
        .next_default = STATE_IDLE,
        .transitions = idle_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(idle_transitions),
    },
    [STATE_FLASH] = {
        .id = STATE_FLASH,
        .function = routine_flash,
        .next_default = STATE_IDLE,
        .transitions = flash_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(flash_transitions),
    },
    [STATE_PAUSE] = {
        .id = STATE_PAUSE,
        .function = routine_pause,
        .next_default = STATE_IDLE,
        .transitions = pause_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(pause_transitions),
    },
    [STATE_MANUAL_WAIT_TS_PRECHARGE] = {
        .id = STATE_MANUAL_WAIT_TS_PRECHARGE,
        .function = routine_manual_wait_ts_precharge,
        .next_default = STATE_MANUAL_WAIT_TS_PRECHARGE,
        .transitions = manual_wait_ts_precharge_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(manual_wait_ts_precharge_transitions),
    },
    [STATE_AS_OFF] = {
        .id = STATE_AS_OFF,
        .function = routine_as_off,
        .next_default = STATE_AS_OFF,
        .transitions = as_off_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_off_transitions),
    },
    [STATE_WAIT_DRIVER] = {
        .id = STATE_WAIT_DRIVER,
        .function = routine_wait_driver,
        .next_default = STATE_WAIT_DRIVER,
        .transitions = wait_driver_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(wait_driver_transitions),
    },
    [STATE_MANUAL_WAIT_TS_DISCHARGE] = {
        .id = STATE_MANUAL_WAIT_TS_DISCHARGE,
        .function = routine_manual_wait_ts_discharge,
        .next_default = STATE_MANUAL_WAIT_TS_DISCHARGE,
        .transitions = manual_wait_ts_discharge_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(manual_wait_ts_discharge_transitions),
    },
    [STATE_MANUAL_WAIT_INV_ENABLE] = {
        .id = STATE_MANUAL_WAIT_INV_ENABLE,
        .function = routine_manual_wait_inv_enable,
        .next_default = STATE_MANUAL_WAIT_INV_ENABLE,
        .transitions = manual_wait_inv_enable_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(manual_wait_inv_enable_transitions),
    },
    [STATE_DRIVING] = {
        .id = STATE_DRIVING,
        .function = routine_driving,
        .next_default = STATE_DRIVING,
        .transitions = driving_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(driving_transitions),
    },
    [STATE_MANUAL_WAIT_INV_DISABLE] = {
        .id = STATE_MANUAL_WAIT_INV_DISABLE,
        .function = routine_manual_wait_inv_disable,
        .next_default = STATE_MANUAL_WAIT_INV_DISABLE,
        .transitions = manual_wait_inv_disable_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(manual_wait_inv_disable_transitions),
    },
    [STATE_AS_OFF_WAIT_TS_PRECHARGE] = {
        .id = STATE_AS_OFF_WAIT_TS_PRECHARGE,
        .function = routine_as_off_wait_ts_precharge,
        .next_default = STATE_AS_OFF_WAIT_TS_PRECHARGE,
        .transitions = as_off_wait_ts_precharge_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_off_wait_ts_precharge_transitions),
    },
    [STATE_AS_READY] = {
        .id = STATE_AS_READY,
        .function = routine_as_ready,
        .next_default = STATE_AS_READY,
        .transitions = as_ready_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_ready_transitions),
    },
    [STATE_AS_READY_WAIT_INV_ENABLE] = {
        .id = STATE_AS_READY_WAIT_INV_ENABLE,
        .function = routine_as_ready_wait_inv_enable,
        .next_default = STATE_AS_READY_WAIT_INV_ENABLE,
        .transitions = as_ready_wait_inv_enable_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_ready_wait_inv_enable_transitions),
    },
    [STATE_AS_EMERGENCY] = {
        .id = STATE_AS_EMERGENCY,
        .function = routine_as_emergency,
        .next_default = STATE_AS_EMERGENCY,
        .transitions = as_emergency_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_emergency_transitions),
    },
    [STATE_AS_R2D] = {
        .id = STATE_AS_R2D,
        .function = routine_as_r2d,
        .next_default = STATE_AS_DRIVING,
        .transitions = as_r2d_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_r2d_transitions),
    },
    [STATE_AS_DRIVING] = {
        .id = STATE_AS_DRIVING,
        .function = routine_as_driving,
        .next_default = STATE_AS_DRIVING,
        .transitions = as_driving_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_driving_transitions),
    },
    [STATE_AS_FINISHED] = {
        .id = STATE_AS_FINISHED,
        .function = routine_as_finished,
        .next_default = STATE_AS_FINISHED,
        .transitions = as_finished_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_finished_transitions),
    },
    [STATE_AS_OFF_WAIT_TS_DISCHARGE] = {
        .id = STATE_AS_OFF_WAIT_TS_DISCHARGE,
        .function = routine_as_off_wait_ts_discharge,
        .next_default = STATE_AS_OFF_WAIT_TS_DISCHARGE,
        .transitions = as_off_wait_ts_discharge_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_off_wait_ts_discharge_transitions),
    },
    [STATE_AS_READY_WAIT_INV_DISABLE] = {
        .id = STATE_AS_READY_WAIT_INV_DISABLE,
        .function = routine_as_ready_wait_inv_disable,
        .next_default = STATE_AS_READY_WAIT_INV_DISABLE,
        .transitions = as_ready_wait_inv_disable_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_ready_wait_inv_disable_transitions),
    },
    [STATE_AS_FINISHED_WAIT_INV_DISABLE] = {
        .id = STATE_AS_FINISHED_WAIT_INV_DISABLE,
        .function = routine_as_finished_wait_inv_disable,
        .next_default = STATE_AS_FINISHED_WAIT_INV_DISABLE,
        .transitions = as_finished_wait_inv_disable_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_finished_wait_inv_disable_transitions),
    },
    [STATE_AS_FINISHED_WAIT_TS_DISCHARGE] = {
        .id = STATE_AS_FINISHED_WAIT_TS_DISCHARGE,
        .function = routine_as_finished_wait_ts_discharge,
        .next_default = STATE_AS_FINISHED_WAIT_TS_DISCHARGE,
        .transitions = as_finished_wait_ts_discharge_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_finished_wait_ts_discharge_transitions),
    },
    [STATE_AS_EMERGENCY_WAIT_INV_DISABLE] = {
        .id = STATE_AS_EMERGENCY_WAIT_INV_DISABLE,
        .function = routine_as_emergency_wait_inv_disable,
        .next_default = STATE_AS_EMERGENCY_WAIT_INV_DISABLE,
        .transitions = as_emergency_wait_inv_disable_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_emergency_wait_inv_disable_transitions),
    },
    [STATE_AS_EMERGENCY_WAIT_TS_DISCHARGE] = {
        .id = STATE_AS_EMERGENCY_WAIT_TS_DISCHARGE,
        .function = routine_as_emergency_wait_ts_discharge,
        .next_default = STATE_AS_EMERGENCY_WAIT_TS_DISCHARGE,
        .transitions = as_emergency_wait_ts_discharge_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(as_emergency_wait_ts_discharge_transitions),
    },
};

void fsm_init() {
    fsm_api_init(&fsm_handler, states, STATE_COUNT, STATE_INIT);
}

void fsm_run(void *data) {
    fsm_api_run_state(&fsm_handler, data);
}
