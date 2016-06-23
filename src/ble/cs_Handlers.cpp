/**
 * Author: Christopher Mason
 * Author: Anne van Rossum
 * License: TODO
 */

#include "ble/cs_Handlers.h"

#include <storage/cs_Settings.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <pstorage.h>
#include <third/protocol/rbc_mesh.h>


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

    Settings& settings = Settings::getInstance();
    if (settings.isInitialized() && settings.isSet(CONFIG_MESH_ENABLED)) {
		rbc_mesh_sd_evt_handler(sys_evt);
		storage_sys_evt_handler(sys_evt);
    }

}

#ifdef __cplusplus
}
#endif

