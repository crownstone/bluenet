/**
 * Author: Christopher Mason
 * Author: Anne van Rossum
 * License: TODO
 */

#include "ble/cs_Handlers.h"
#include <drivers/cs_Storage.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <pstorage.h>
#include <third/protocol/rbc_mesh.h>

#include <drivers/cs_Serial.h>

/** Function for dispatching a system event (not a BLE event) to all modules with a system event handler. This can also
 * be events related to the radio, for example the NRF_EVT_RADIO_BLOCKED (4) and NRF_EVT_RADIO_SESSION_IDLE (7) events
 * that are defined for the timeslot API.
 *
 * This function is called from the scheduler in the main loop after a BLE stack
 *   event has been received.
 *
 * @sys_evt   System event.
 */
void sys_evt_dispatch(uint32_t sys_evt) {

//	LOGi("Sys evt dispatch: %d", sys_evt);
    pstorage_sys_event_handler(sys_evt);

#if CHAR_MESHING==1
    rbc_mesh_sd_evt_handler(sys_evt);
    storage_sys_evt_handler(sys_evt);
//!    rbc_mesh_sd_evt_handler(sys_evt);
#endif

}

////! called by softdevice handler on a ble event
//void ble_evt_handler(ble_evt_t* p_ble_evt) {
//	Nrf51822BluetoothStack::getInstance().on_ble_evt(p_ble_evt);
//}


#ifdef __cplusplus
}
#endif

