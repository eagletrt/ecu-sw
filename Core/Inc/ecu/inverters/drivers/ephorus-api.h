/*!
 * \file ephorus-api.h
 * \date 2026-07-31
 * \authors Alessandro Bridi [ale.bridi15@gmail.com]
 *
 * \brief Ephorus 3.1 inverter driver API.
 */

#ifndef EPHORUS_API_H
#define EPHORUS_API_H

#include "ephorus.h"

/*!
 * \brief Reset the driver: configure every wheel's frame ids, none active.
 *
 * \param handle Driver handle to initialize.
 *
 * \retval EPHORUS_RC_OK Initialization successful.
 * \retval EPHORUS_RC_NULL_POINTER handle was NULL.
 */
enum EphorusReturnCode ephorus_api_init(struct EphorusHandler *handle);

/*!
 * \brief Activate a wheel so it starts transmitting and decoding.
 *
 * \param handle Driver handle.
 * \param wheel  Wheel to activate.
 *
 * \retval EPHORUS_RC_OK on success.
 * \retval EPHORUS_RC_NULL_POINTER handle was NULL.
 * \retval EPHORUS_RC_INVALID_WHEEL wheel out of range.
 */
enum EphorusReturnCode ephorus_api_attach(struct EphorusHandler *handle, enum EphorusWheel wheel);

/*!
 * \brief Arm a wheel (clear latched errors, enable drive).
 *
 * \param handle Driver handle.
 * \param wheel  Wheel to arm.
 */
void ephorus_api_arm(struct EphorusHandler *handle, enum EphorusWheel wheel);

/*!
 * \brief Disarm a wheel (stop run request, disable drive).
 *
 * \param handle Driver handle.
 * \param wheel  Wheel to disarm.
 */
void ephorus_api_disarm(struct EphorusHandler *handle, enum EphorusWheel wheel);

/*!
 * \brief Command or release a wheel's run request (only while armed/unfaulted).
 *
 * \param handle Driver handle.
 * \param wheel  Wheel to command.
 * \param run    true to request run, false to release.
 */
void ephorus_api_set_run(struct EphorusHandler *handle, enum EphorusWheel wheel, bool run);

/*!
 * \brief Flip a wheel's run request.
 *
 * \param handle Driver handle.
 * \param wheel  Wheel to command.
 */
void ephorus_api_toggle_run(struct EphorusHandler *handle, enum EphorusWheel wheel);

/*!
 * \brief Set a wheel's signed torque request [Nm] (clamped to +/- EPHORUS_MAX_TORQUE_NM).
 *
 * \details Positive requests drive torque, negative requests brake/regen torque.
 *     The sign selects the torque window and the speed rail actually emitted (see
 *     \ref ephorus_api_build_setpoints).
 *
 * \param handle    Driver handle.
 * \param wheel     Wheel to command.
 * \param torque_nm Signed torque request in Nm.
 */
void ephorus_api_set_torque(struct EphorusHandler *handle, enum EphorusWheel wheel, float torque_nm);

/*!
 * \brief Serializes a wheel's setpoint frame from its current signed torque request.
 *
 * \details Translates the signed torque request into the inverter setpoints:
 *       - request > 0 : torquelimitpositive = T, torquelimitnegative = 0,
 *                       speedsetpoint = EPHORUS_DRIVE_SPEED_RPM;
 *       - request < 0 : torquelimitpositive = 0, torquelimitnegative = T,
 *                       speedsetpoint = 0;
 *       - request = 0 : both limits 0, speedsetpoint = 0.
 *     A wheel that is not armed/running (or is faulted) emits a zero request.
 *     Meant to be called once per EPHORUS_TX_PERIOD_MS (the Inverters module
 *     enforces that cadence).
 *
 * \param[in]  handle Driver handle.
 * \param[in]  wheel  Wheel to serialize.
 * \param[out] out_id CAN identifier to send under.
 * \param[out] data   Payload buffer (EPHORUS_FRAME_DATA_SIZE bytes).
 *
 * \retval EPHORUS_RC_OK Frame serialized.
 * \retval EPHORUS_RC_NULL_POINTER a pointer argument was NULL.
 * \retval EPHORUS_RC_INVALID_WHEEL wheel out of range.
 * \retval EPHORUS_RC_INACTIVE the wheel is not attached (nothing to send).
 * \retval EPHORUS_RC_SERIALIZE_ERROR libcan-sw failed to encode the frame.
 */
enum EphorusReturnCode ephorus_api_build_setpoints(struct EphorusHandler *handle, enum EphorusWheel wheel, uint32_t *out_id, uint8_t data[EPHORUS_FRAME_DATA_SIZE]);

/*!
 * \brief Decodes one received frame and routes it to the right wheel / general.
 *
 * \details Wheel-specific outbound frames update that wheel; the shared
 *     0x400/0x401 frames update the general telemetry and every active wheel's
 *     faults. Frames that are not part of the inverters network are ignored.
 *     When a new fault is latched the affected wheels' run request is cleared.
 *
 * \param handle Driver handle.
 * \param id     CAN identifier of the received frame.
 * \param data   Payload bytes (EPHORUS_FRAME_DATA_SIZE bytes).
 */
void ephorus_api_handle_frame(struct EphorusHandler *handle, uint32_t frame_id, const uint8_t data[EPHORUS_FRAME_DATA_SIZE]);

/*!
 * \brief Borrow a wheel's telemetry (valid while the handle lives).
 *
 * \param handle Driver handle.
 * \param wheel  Wheel to read.
 *
 * \return Pointer into the handle, or NULL if \p wheel is out of range.
 */
const struct EphorusWheelTelemetry *ephorus_api_wheel_telemetry(const struct EphorusHandler *handle, enum EphorusWheel wheel);

/*!
 * \brief Borrow the shared telemetry (valid while the handle lives).
 *
 * \param handle Driver handle.
 *
 * \return Pointer into the handle, or NULL if \p handle is NULL.
 */
const struct EphorusGeneralTelemetry *ephorus_api_general_telemetry(const struct EphorusHandler *handle);

/*!
 * \brief Name of an inverter state (static string, never NULL).
 *
 * \param state Inverter state to name.
 *
 * \return Static string with the name of \p state.
 */
const char *ephorus_api_state_name(enum EphorusState state);

/*!
 * \brief Name of a per-wheel fault bit (static string, never NULL).
 *
 * \param fault_bit Fault bit to name.
 *
 * \return Static string with the name of \p fault_bit.
 */
const char *ephorus_api_wheel_fault_name(uint32_t fault_bit);

/*!
 * \brief Name of a shared fault bit (static string, never NULL).
 *
 * \param fault_bit Fault bit to name.
 */
const char *ephorus_api_general_fault_name(uint32_t fault_bit);

#endif // EPHORUS_API_H
