/**
 * @file
 * System event handlers 
 *
 * @authors Crownstone Team
 * @copyright Crownstone B.V.
 * @date Apr 24, 2015
 * @license LGPLv3+, Apache License, or MIT, your choice
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
 * + pstorage: NRF_EVT_FLASH_OPERATION_SUCCESS and NRF_EVT_FLASH_OPERATION_ERROR
 * + mesh: storage events and timeslot events (see above)
 * + all: NRF_EVT_POWER_FAILURE_WARNING just before a brownout
 *
 * This function is called from the scheduler in the main loop after a BLE stack event has been received.
 *
 * @param sys_evt                      System event.
 */
void sys_evt_dispatch(uint32_t sys_evt);

//void ble_evt_handler(ble_evt_t* p_ble_evt);

#ifdef __cplusplus
}
#endif


