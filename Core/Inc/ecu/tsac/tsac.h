#ifndef TSAC_H
#define TSAC_H

#include "can-primary.h"

enum TsacReturnCode {
    TSAC_RC_OK,           /*!< Operation completed successfully. */
    TSAC_RC_ERROR,        /*!< A generic or internal system error occurred. */
    TSAC_RC_NULL_POINTER, /*!< A null pointer was passed to an API function. */
};

#define TSAC_TIMEOUT_MS ((uint32_t)(can_primary_cycle_time_tsacstatus * 2.5)) /*!< Timeout duration for TSAC status messages in milliseconds. */

typedef uint32_t (*tsac_get_tick_callback)(void); /*!< Callback function type for retrieving the current system tick count. */

struct TsacHandler {
    /*! 
     * \brief Tractive system safety voltage threshold flag.
     * \details Evaluates to true if the DC link or Accumulator voltage exceeds 
     * the mandatory safety-critical 60V threshold. Used as a permissive block.
     */
    bool voltage_higher_than_60v;

    /*!
     * \brief Tractive system state to be requested to TSAC.
     * \details Set to true when the FSM requires the tractive system to be enabled.
     */
    bool ts_state_to_require;

    /*!
     * \brief Current status of the TSAC mainboard.
     */
    enum CanPrimaryTsacstatusMainboardstatus tsac_status;

    /*!
     * \brief Timestamp of the last received TSAC status message.
     */
    uint32_t last_status_received_tick;

    /*!
     * \brief Callback function to retrieve the current system tick count.
     */
    tsac_get_tick_callback get_tick;
};

#endif // TSAC_H
