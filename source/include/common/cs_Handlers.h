/**
 * @file
 * System event handlers
 *
 * @authors Crownstone Team
 * @copyright Crownstone B.V.
 * @date Apr 24, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <ble/cs_Nordic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Function for dispatching a system event (not a BLE event).
 *
 * This function to dispach a system events goes to all modules with a system event handler. This can also
 * be events related to the radio, for example the NRF_EVT_RADIO_BLOCKED (4) and NRF_EVT_RADIO_SESSION_IDLE (7) events
 * that are defined for the timeslot API.
 *
 * Events are send to:
 * + mesh: storage events and timeslot events (see above)
 * + all: NRF_EVT_POWER_FAILURE_WARNING just before a brownout
 *
 * This function is called from the scheduler in the main loop after a BLE stack event has been received.
 *
 * @param sys_evt                      System event.
 */
//void crownstone_soc_evt_handler(uint32_t evt_id, void * p_context);

void fds_evt_handler(const fds_evt_t * p_fds_evt);


#ifdef __cplusplus
}
#endif


class SocHandler {
public:
	/**
	 * Handle SOC events.
	 * Can be called from interrupt.
	 */
	static void handleEvent(uint32_t event);

	/**
	 * Handle SOC events.
	 * Must be called from main thread.
	 */
	static void handleEventDecoupled(uint32_t event);
};

class BleHandler {
public:

	/**
	 * Handle BLE events.
	 * Can be called from interrupt.
	 */
	static void handleEvent(const ble_evt_t* event);

	/**
	 * Handle BLE events.
	 * Must be called from main thread.
	 */
	static void handleEventDecoupled(const ble_evt_t* event);

	static void handlePhyRequest(uint16_t connectionHandle, const ble_gap_evt_phy_update_request_t& request);
	static void handleDataLengthRequest(uint16_t connectionHandle, const ble_gap_evt_data_length_update_request_t& request);
	static void handleMtuRequest(uint16_t connectionHandle, const ble_gatts_evt_exchange_mtu_request_t& request);
	static void disconnect(uint16_t connectionHandle, uint8_t reason);
};

class SdhStateHandler {
public:
	/**
	 * Handle SDH state events.
	 * Can be called from interrupt.
	 */
	static void handleEvent(const nrf_sdh_state_evt_t& event);
};
