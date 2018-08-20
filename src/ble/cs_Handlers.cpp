/**
 * Author: Crownstone Team
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Handlers.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>

#ifdef __cplusplus
extern "C" {
#endif

#if BUILD_MESHING == 1
#include <rbc_mesh.h>
#endif

/**
 * This handle is called by the Softdevice. It is run on the main thread through the app scheduler.
 * The purpose of this function is:
 * 	+ to propagate flash events towards the storage event handler
 * 	+ to propagate a power failure warning when a brown-out is impending
 * 	+ to propagate all events to the mesh event handler
 */
void sys_evt_dispatch(uint32_t sys_evt) {

    switch(sys_evt) {
	case NRF_EVT_FLASH_OPERATION_SUCCESS:
	case NRF_EVT_FLASH_OPERATION_ERROR:
	    LOGw("Storage event, unhandled");
	    break;
	case NRF_EVT_POWER_FAILURE_WARNING: {
	    event_t event(CS_TYPE::EVT_BROWNOUT_IMPENDING, NULL, 0);
	    EventDispatcher::getInstance().dispatch(event);
	    break;
	}
    }

#if BUILD_MESHING == 1
    Settings& settings = Settings::getInstance();
    if (settings.isInitialized() && settings.isSet(CONFIG_MESH_ENABLED)) {
		rbc_mesh_sd_evt_handler(sys_evt);
		//storage_sys_evt_handler(sys_evt);
    }
#endif

}

#ifdef __cplusplus
}
#endif

