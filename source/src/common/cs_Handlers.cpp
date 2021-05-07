/**
 * Author: Crownstone Team
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Handlers.h>
#include <drivers/cs_GpRegRet.h>
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
#define MESH_SOC_OBSERVER_PRIO                   0
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

// Change to LOGd to log the interrupt levels.
#define LOGInterruptLevel LOGnone


/**
 * The decoupled SOC event handler, should run in thread mode.
 */
void crownstone_soc_evt_handler_decoupled(void * p_event_data, uint16_t event_size) {
	LOGInterruptLevel("soc evt decoupled int=%u", BLEutil::getInterruptLevel());
	uint32_t evt_id = *(uint32_t*)p_event_data;
	switch (evt_id) {
		case NRF_EVT_POWER_FAILURE_WARNING: {
			event_t event(CS_TYPE::EVT_BROWNOUT_IMPENDING);
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		case NRF_EVT_FLASH_OPERATION_SUCCESS: {
			Storage::getInstance().handleFlashOperationSuccess();
			break;
		}
		case NRF_EVT_FLASH_OPERATION_ERROR: {
			Storage::getInstance().handleFlashOperationError();
			break;
		}
		default: {
			break;
		}
	}
}

void crownstone_soc_evt_handler(uint32_t evt_id, void * p_context) {
	LOGInterruptLevel("soc evt int=%u", BLEutil::getInterruptLevel());

	switch (evt_id) {
		case NRF_EVT_POWER_FAILURE_WARNING:
			// Set brownout flag, so next boot we know the cause.
			// Only works when we reset as well?
			// Only works when softdevice handler dispatch model is interrupt?
			GpRegRet::setFlag(GpRegRet::FLAG_BROWNOUT);
			// Fall through
		case NRF_EVT_FLASH_OPERATION_SUCCESS:
		case NRF_EVT_FLASH_OPERATION_ERROR: {
//			uint32_t gpregret_id = 0;
//			uint32_t gpregret_msk = CS_GPREGRET_BROWNOUT_RESET;
//			// NOTE: do not clear the gpregret register, this way
//			//   we can count the number of brownouts in the bootloader.
//			sd_power_gpregret_set(gpregret_id, gpregret_msk);
//			// Soft reset, because brownout can't be distinguished from hard reset otherwise.
//			sd_nvic_SystemReset();

#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
			uint32_t retVal = app_sched_event_put(&evt_id, sizeof(evt_id), crownstone_soc_evt_handler_decoupled);
			APP_ERROR_CHECK(retVal);
#else
			crownstone_soc_evt_handler_decoupled(&evt_id, sizeof(evt_id));
#endif
			break;
		}
		case NRF_EVT_RADIO_BLOCKED:
		case NRF_EVT_RADIO_CANCELED:
		case NRF_EVT_RADIO_SESSION_IDLE:
		case NRF_EVT_RADIO_SESSION_CLOSED:
			break;
		default: {
			LOGd("Unhandled event: %i", evt_id);
		}
	}
}
NRF_SDH_SOC_OBSERVER(m_crownstone_soc_observer, CROWNSTONE_SOC_OBSERVER_PRIO, crownstone_soc_evt_handler, NULL);


/**
 * Decoupled BLE event handler, should run in thread mode.
 */
void crownstone_sdh_ble_evt_handler_decoupled(const void * p_event_data, uint16_t event_size) {
	LOGInterruptLevel("sdh ble evt decoupled int=%u", BLEutil::getInterruptLevel());
	ble_evt_t* p_ble_evt = (ble_evt_t*) p_event_data;
	Stack::getInstance().onBleEvent(p_ble_evt);
}
void crownstone_sdh_ble_evt_handler_sched(void * p_event_data, uint16_t event_size) {
	crownstone_sdh_ble_evt_handler_decoupled(p_event_data, event_size);
}
/**
 * Called by the SoftDevice on any BLE event.
 * Some event should be handled on interrupt level, while most are decouple via app scheduler.
 */
void crownstone_sdh_ble_evt_handler(const ble_evt_t * p_ble_evt, void * p_context) {


	LOGInterruptLevel("sdh ble evt int=%u", BLEutil::getInterruptLevel());
	switch (p_ble_evt->header.evt_id) {
		case BLE_GAP_EVT_ADV_REPORT: {
#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
			Stack::getInstance().onBleEventInterrupt(p_ble_evt, true);
#else
			Stack::getInstance().onBleEventInterrupt(p_ble_evt, false);
#endif
			break;
		}
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
//			uint32_t retVal = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle, BLE_GATT_ATT_MTU_DEFAULT);
			uint32_t retVal = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
			APP_ERROR_CHECK(retVal);
			break;
		}
		case BLE_GAP_EVT_RSSI_CHANGED:
		case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
		case BLE_GAP_EVT_CONNECTED:
		case BLE_GAP_EVT_DISCONNECTED:
		case BLE_GAP_EVT_PASSKEY_DISPLAY:
		case BLE_GAP_EVT_TIMEOUT:
		case BLE_EVT_USER_MEM_REQUEST:
		case BLE_EVT_USER_MEM_RELEASE:
		case BLE_GATTS_EVT_WRITE:
		case BLE_GATTS_EVT_HVN_TX_COMPLETE:
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
		default: {
#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
			uint32_t retVal = app_sched_event_put(p_ble_evt, sizeof(ble_evt_t), crownstone_sdh_ble_evt_handler_sched);
			APP_ERROR_CHECK(retVal);
#else
			crownstone_sdh_ble_evt_handler_decoupled(p_ble_evt, sizeof(ble_evt_t));
#endif
			break;
		}
	}
}
NRF_SDH_BLE_OBSERVER(m_stack, CROWNSTONE_BLE_OBSERVER_PRIO, crownstone_sdh_ble_evt_handler, NULL);



static void crownstone_sdh_state_evt_handler(nrf_sdh_state_evt_t state, void * p_context) {
	LOGInterruptLevel("sdh state evt int=%u", BLEutil::getInterruptLevel());
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
	.handler   = crownstone_sdh_state_evt_handler,
	.p_context = NULL
};

#if BUILD_MESHING == 1
// From: ble_softdevice_support.c
void mesh_soc_evt_handler_decoupled(void * p_event_data, uint16_t event_size) {
	LOGInterruptLevel("mesh soc evt decoupled int=%u", BLEutil::getInterruptLevel());
	uint32_t evt_id = *(uint32_t*)p_event_data;
	nrf_mesh_on_sd_evt(evt_id);
}

static void mesh_soc_evt_handler(uint32_t evt_id, void * p_context) {
	LOGInterruptLevel("mesh soc evt int=%u", BLEutil::getInterruptLevel());
#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
	uint32_t retVal = app_sched_event_put(&evt_id, sizeof(evt_id), mesh_soc_evt_handler_decoupled);
	APP_ERROR_CHECK(retVal);
#else
	nrf_mesh_on_sd_evt(evt_id);
#endif
}
NRF_SDH_SOC_OBSERVER(m_mesh_soc_observer, MESH_SOC_OBSERVER_PRIO, mesh_soc_evt_handler, NULL);
#endif

/**
 * The decoupled FDS event handler, should run in thread mode.
 */
void fds_evt_handler_decoupled(const void * p_event_data, uint16_t event_size) {
	LOGInterruptLevel("fds evt decoupled int=%u", BLEutil::getInterruptLevel());
	fds_evt_t* p_fds_evt = (fds_evt_t*) p_event_data;
	Storage::getInstance().handleFileStorageEvent(p_fds_evt);
}
void fds_evt_handler_sched(void * p_event_data, uint16_t event_size) {
	fds_evt_handler_decoupled(p_event_data, event_size);
}

/**
 * The FDS event handler is registered in the cs_Storage class.
 */
void fds_evt_handler(fds_evt_t const * const p_fds_evt) {
	LOGInterruptLevel("fds evt int=%u", BLEutil::getInterruptLevel());
	// For some reason, we already got fds event init, before app_sched_execute() was called.
	// So for now, just always put fds events on the app scheduler.
//#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
	uint32_t retVal = app_sched_event_put(p_fds_evt, sizeof(*p_fds_evt), fds_evt_handler_sched);
	APP_ERROR_CHECK(retVal);
//#else
//	fds_evt_handler_decoupled(p_fds_evt, sizeof(*p_fds_evt));
//#endif
}

#ifdef __cplusplus
}
#endif

