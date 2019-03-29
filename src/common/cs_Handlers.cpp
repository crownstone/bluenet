/**
 * Author: Crownstone Team
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Handlers.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <ble/cs_Stack.h>

#ifdef __cplusplus
extern "C" {
#endif

#if BUILD_MESHING == 1
#include <rbc_mesh.h>
#endif

#define CROWNSTONE_STACK_OBSERVER_PRIO           0
#define CROWNSTONE_REQUEST_OBSERVER_PRIO         0
#define CROWNSTONE_SOC_OBSERVER_PRIO             0
#define CROWNSTONE_STATE_OBSERVER_PRIO           0

#define CROWNSTONE_BLE_OBSERVER_PRIO             1

/**
 * This handle is called by the Softdevice. It is run on the main thread through the app scheduler.
 * The purpose of this function is:
 * 	+ to propagate flash events towards the storage event handler
 * 	+ to propagate a power failure warning when a brown-out is impending
 * 	+ to propagate all events to the mesh event handler
 *
 * The sys_evt_dispatch function has been removed in SDK14 in favor of NRF_SDH_SOC_OBSERVER.
 * From this function storage_sys_evt_handler was called. It's now fs_sys_event_handler.
 */
void crownstone_soc_evt_handler(uint32_t evt_id, void * p_context) {
    //LOGd("SOC event: %i", evt_id);

    //fs_sys_event_handler(evt_id);

    switch(evt_id) {
	case NRF_EVT_FLASH_OPERATION_SUCCESS:
	    LOGnone("NRF_EVT_FLASH_OPERATION_SUCCESS, unhandled");
	    break;
	case NRF_EVT_FLASH_OPERATION_ERROR:
	    // Why is it unhandled here? If there is a call to a filesystem event handler, is it supposed to change
	    // evt_id as a side-effect?
	    LOGw("NRF_EVT_FLASH_OPERATION_ERROR, unhandled");
	    break;
	case NRF_EVT_POWER_FAILURE_WARNING: {
	    event_t event(CS_TYPE::EVT_BROWNOUT_IMPENDING, NULL, 0);
	    EventDispatcher::getInstance().dispatch(event);
	    break;
	}
	default: {
	    LOGd("Unhandled event: %i", evt_id);
	}
    }

#if BUILD_MESHING == 1
    Settings& settings = Settings::getInstance();
    if (settings.isInitialized() && settings.isSet(CONFIG_MESH_ENABLED)) {
		rbc_mesh_sd_evt_handler(evt_id);
		//storage_sys_evt_handler(sys_evt);
    }
#endif

}

NRF_SDH_SOC_OBSERVER(m_crownstone_soc_observer, CROWNSTONE_SOC_OBSERVER_PRIO, crownstone_soc_evt_handler, NULL);


/**
 * Called by the Softdevice handler on any BLE event. It is initialized from the SoftDevice using the app scheduler.
 * This means that the callback runs on the main thread.
 */

void ble_sdh_evt_dispatch(const ble_evt_t * p_ble_evt, void * p_context) {
#if BUILD_MESHING == 1
    if (State::getInstance().isTrue(CS_TYPE::CONFIG_MESH_ENABLED)) {
	rbc_mesh_ble_evt_handler(p_ble_evt);
    }
#endif
    Stack::getInstance().on_ble_evt(p_ble_evt);
}

NRF_SDH_BLE_OBSERVER(m_stack, CROWNSTONE_BLE_OBSERVER_PRIO, ble_sdh_evt_dispatch, NULL);

/*
static bool crownstone_request_evt_handler(nrf_sdh_req_evt_t request, void * p_context) {
	switch (request) {
	case NRF_SDH_EVT_ENABLE_REQUEST:
		LOGd("Enable request");
		break;
	case NRF_SDH_EVT_DISABLE_REQUEST:
		LOGd("Disable request");
		break;
	default:
		LOGd("Unknown request");
	}
	return true;
}*/

static void crownstone_state_handler(nrf_sdh_state_evt_t state, void * p_context) {
	switch (state) {
		case NRF_SDH_EVT_STATE_ENABLE_PREPARE:
			LOGd("Softdevice is about to be enabled");
			break;
		case NRF_SDH_EVT_STATE_ENABLED:
			LOGd("Softdevice is now enabled");
			break;
		case NRF_SDH_EVT_STATE_DISABLE_PREPARE:
			LOGd("Softdevice is about to be disabled");
			break;
		case NRF_SDH_EVT_STATE_DISABLED:
			LOGd("Softdevice is now disabled");
			break;
		default:
			LOGd("Unknown state change");
	}
}

/**
 * There seems to be an NULL pointer exception in the code at nrf_sdh.c::117, namely when calling
 * sdh_request_observer_notify.
 */
/*
NRF_SDH_REQUEST_OBSERVER(m_crownstone_request_observer, CROWNSTONE_REQUEST_OBSERVER_PRIO) =
{
	.handler   = crownstone_request_evt_handler,
	.p_context = NULL
};*/

NRF_SDH_STATE_OBSERVER(m_crownstone_state_handler, CROWNSTONE_STATE_OBSERVER_PRIO) =
{
	.handler   = crownstone_state_handler,
	.p_context = NULL
};

/**
 * The decoupled FDS event handler.
 */
void fds_evt_decoupled_handler(void * p_event_data, uint16_t event_size) {
	fds_evt_t* p_fds_evt = (fds_evt_t*) p_event_data;
	Storage::getInstance().handleFileStorageEvent(p_fds_evt);
}

/**
 * The FDS event handler is registered in the cs_Storage class.
 *
 * Decouple these events via the app scheduler.
 */
void fds_evt_handler(fds_evt_t const * const p_fds_evt) {
    app_sched_event_put(p_fds_evt, sizeof(*p_fds_evt), fds_evt_decoupled_handler);
}

#ifdef __cplusplus
}
#endif

