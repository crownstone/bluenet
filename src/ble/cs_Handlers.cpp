/**
 * Author: Christopher Mason
 * Author: Anne van Rossum
 * License: TODO
 */

#include "ble/cs_Handlers.h"

#include <storage/cs_Settings.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>

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

    if ((sys_evt == NRF_EVT_FLASH_OPERATION_SUCCESS) ||
            (sys_evt == NRF_EVT_FLASH_OPERATION_ERROR)) {
    	pstorage_sys_event_handler(sys_evt);
    }

    Settings& settings = Settings::getInstance();
    if (settings.isInitialized() && settings.isSet(CONFIG_MESH_ENABLED)) {
		rbc_mesh_sd_evt_handler(sys_evt);
		storage_sys_evt_handler(sys_evt);
    }

    switch(sys_evt) {
    case NRF_EVT_POWER_FAILURE_WARNING: {
    	EventDispatcher::getInstance().dispatch(EVT_BROWNOUT_IMPENDING);
    }
    }

}

#ifdef __cplusplus
}
#endif

