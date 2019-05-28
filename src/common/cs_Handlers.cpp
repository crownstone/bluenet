/**
 * Author: Crownstone Team
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Handlers.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <ble/cs_Stack.h>
#include <third/nrf/sdk_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if BUILD_MESHING == 1
#include <nrf_mesh.h>
#endif

//#define CROWNSTONE_STACK_OBSERVER_PRIO           0
#define CROWNSTONE_SOC_OBSERVER_PRIO             0 // Flash events, power failure events, radio events, etc.
//#define CROWNSTONE_REQUEST_OBSERVER_PRIO         0 // SoftDevice enable/disable requests.
#define CROWNSTONE_STATE_OBSERVER_PRIO           0 // SoftDevice state events (enabled/disabled)
#define CROWNSTONE_BLE_OBSERVER_PRIO             1 // BLE events: connection, scans, rssi

/**
 * Define handlers of events.
 *
 * SoftDevice handlers can be executed via app scheduler or directly in the interrupt, see NRF_SDH_DISPATCH_MODEL in
 * sdk_config.h. Also see: "SoftDevice Handler" in SDK common libraries.
 *
 * Mesh handlers are executed in thread mode, this is defined in the init parameters, mesh_stack_init_params_t.
 *
 * FDS uses a SoftDevice handler too, so this probably has the same interrupt level as the SoftDevice handlers.
 *
 * I think we should keep the SDH at interrupt level, since SDK libraries also execute at that level.
 * Then, the events can always be passed on via the scheduler to get it on the main thread.
 */



void cs_brownout_handler(void * p_event_data, uint16_t event_size) {
	event_t event(CS_TYPE::EVT_BROWNOUT_IMPENDING, NULL, 0);
	EventDispatcher::getInstance().dispatch(event);
}

void crownstone_soc_evt_handler(uint32_t evt_id, void * p_context) {
    //LOGd("SOC event: %i", evt_id);

    //fs_sys_event_handler(evt_id);

    switch(evt_id) {
	case NRF_EVT_FLASH_OPERATION_SUCCESS:
	    LOGnone("NRF_EVT_FLASH_OPERATION_SUCCESS, unhandled");
	    break;
	case NRF_EVT_FLASH_OPERATION_ERROR:
	    // This is handled by fstorage / FDS.
	    LOGnone("NRF_EVT_FLASH_OPERATION_ERROR, unhandled");
	    break;
	case NRF_EVT_POWER_FAILURE_WARNING: {
		uint32_t retVal = app_sched_event_put(NULL, 0, cs_brownout_handler);
		APP_ERROR_CHECK(retVal);
	    break;
	}
	case NRF_EVT_RADIO_BLOCKED: {
		break;
	}
	default: {
	    LOGd("Unhandled event: %i", evt_id);
	}
    }
}
NRF_SDH_SOC_OBSERVER(m_crownstone_soc_observer, CROWNSTONE_SOC_OBSERVER_PRIO, crownstone_soc_evt_handler, NULL);


/**
 * Decoupled BLE events.
 */
void cs_ble_handler(void * p_event_data, uint16_t event_size) {
	ble_evt_t* p_ble_evt = (ble_evt_t*) p_event_data;
	Stack::getInstance().onBleEvent(p_ble_evt);
}

/**
 * Called by the SoftDevice on any BLE event.
 * Some event should be handled on interrupt level, while most are decouple via app scheduler.
 */
void ble_sdh_evt_dispatch(const ble_evt_t * p_ble_evt, void * p_context) {
	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_ADV_REPORT:
		Stack::getInstance().onBleEventInterrupt(p_ble_evt);
		break;
	case BLE_GAP_EVT_CONNECTED:
	case BLE_GAP_EVT_DISCONNECTED:
	case BLE_GAP_EVT_PASSKEY_DISPLAY:
	case BLE_GAP_EVT_TIMEOUT:
	case BLE_EVT_USER_MEM_REQUEST:
	case BLE_EVT_USER_MEM_RELEASE:
	case BLE_GATTS_EVT_WRITE:
	case BLE_GATTS_EVT_HVN_TX_COMPLETE:
	case BLE_GATTS_EVT_SYS_ATTR_MISSING: {
		uint32_t retVal = app_sched_event_put(p_ble_evt, sizeof(ble_evt_t), cs_ble_handler);
		APP_ERROR_CHECK(retVal);
		break;
	}
	case BLE_GAP_EVT_RSSI_CHANGED:
	case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
		break;
	case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
		ble_gap_phys_t phys;
		phys.rx_phys = BLE_GAP_PHY_AUTO;
		phys.tx_phys = BLE_GAP_PHY_AUTO;
		uint32_t retVal = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
		APP_ERROR_CHECK(retVal);
		break;
	}
	case BLE_GATTC_EVT_TIMEOUT: {
		// Disconnect on GATT Client timeout event.
		uint32_t retVal = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		APP_ERROR_CHECK(retVal);
		break;
	}
	case BLE_GATTS_EVT_TIMEOUT: {
		// Disconnect on GATT Server timeout event.
		uint32_t retVal = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		APP_ERROR_CHECK(retVal);
		break;
	}
	case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
		uint32_t retVal = sd_ble_gap_data_length_update(p_ble_evt->evt.gatts_evt.conn_handle, NULL, NULL);
		APP_ERROR_CHECK(retVal);
		break;
	}
	case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
//		uint32_t retVal = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle, BLE_GATT_ATT_MTU_DEFAULT);
		uint32_t retVal = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
		APP_ERROR_CHECK(retVal);
		break;
	}
	default:
		break;
	}
}
NRF_SDH_BLE_OBSERVER(m_stack, CROWNSTONE_BLE_OBSERVER_PRIO, ble_sdh_evt_dispatch, NULL);



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
NRF_SDH_STATE_OBSERVER(m_crownstone_state_handler, CROWNSTONE_STATE_OBSERVER_PRIO) =
{
	.handler   = crownstone_state_handler,
	.p_context = NULL
};

#if BUILD_MESHING == 1
// From: ble_softdevice_support.c
#define MESH_SOC_OBSERVER_PRIO 0
static void mesh_soc_evt_handler(uint32_t evt_id, void * p_context)
{
    nrf_mesh_on_sd_evt(evt_id);
}
NRF_SDH_SOC_OBSERVER(m_mesh_soc_observer, MESH_SOC_OBSERVER_PRIO, mesh_soc_evt_handler, NULL);
#endif

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
	uint32_t retVal = app_sched_event_put(p_fds_evt, sizeof(*p_fds_evt), fds_evt_decoupled_handler);
	APP_ERROR_CHECK(retVal);
}

#ifdef __cplusplus
}
#endif

