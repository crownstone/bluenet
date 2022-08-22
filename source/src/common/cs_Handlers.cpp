/**
 * Author: Crownstone Team
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "common/cs_Handlers.h"

#include <ble/cs_Stack.h>
#include <drivers/cs_GpRegRet.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>

#ifdef __cplusplus
extern "C" {
#endif

#if BUILD_MESHING == 1
#include <nrf_mesh.h>
#endif

//#define CROWNSTONE_STACK_OBSERVER_PRIO           0
#define CROWNSTONE_SOC_OBSERVER_PRIO 0  // Flash events, power failure events, radio events, etc.
#define MESH_SOC_OBSERVER_PRIO 0
//#define CROWNSTONE_REQUEST_OBSERVER_PRIO         0 // SoftDevice enable/disable requests.
#define CROWNSTONE_STATE_OBSERVER_PRIO 0  // SoftDevice state events (enabled/disabled)
#define CROWNSTONE_BLE_OBSERVER_PRIO 1    // BLE events: connection, scans, rssi

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
#define LOGInterruptLevel LOGvv

/**
 * Scheduled SOC event handler callback.
 */
void crownstone_soc_evt_handler_decoupled(void* p_event_data, [[maybe_unused]] uint16_t event_size) {
	SocHandler::handleEventDecoupled(*reinterpret_cast<uint32_t*>(p_event_data));
}

void crownstone_soc_evt_handler(uint32_t evt_id, [[maybe_unused]] void* p_context) {
	SocHandler::handleEvent(evt_id);
}

NRF_SDH_SOC_OBSERVER(m_crownstone_soc_observer, CROWNSTONE_SOC_OBSERVER_PRIO, crownstone_soc_evt_handler, NULL);

/**
 * Scheduled BLE event handler callback.
 */
void crownstone_sdh_ble_evt_handler_decoupled(void* p_event_data, [[maybe_unused]] uint16_t event_size) {
	BleHandler::handleEventDecoupled(reinterpret_cast<const ble_evt_t*>(p_event_data));
}

void crownstone_sdh_ble_evt_handler(const ble_evt_t* p_ble_evt, [[maybe_unused]] void* p_context) {
	BleHandler::handleEvent(p_ble_evt);
}

/*
 * Registers a softdevice handler for BLE events.
 * The section .sdh_ble_observers1 will be used, it's just appending the priority 1 here.
 */
NRF_SDH_BLE_OBSERVER(m_stack, CROWNSTONE_BLE_OBSERVER_PRIO, crownstone_sdh_ble_evt_handler, NULL);

static void crownstone_sdh_state_evt_handler(nrf_sdh_state_evt_t state, [[maybe_unused]] void* p_context) {
	SdhStateHandler::handleEvent(state);
}

/*
 * This writes to a new static m_crownstone_state_handler object of type nrf_sdh_state_observer_t.
 * Using the macro means NRF_SDH_ENABLED is checked as well as the priority level available.
 * The section .sdh_state_observers0 will be used, it's just appending the priority 0 here.
 * The macro is written in the form of an assignment so we get a compilation error if the implementation changes.
 * More, detailed: it will be this:
 *
 *    m_crownstone_state_handler
 *            __attribute__ ((section(".sdh_state_observers0")))
 *            __attribute__ ((used)) =
 *        {
 *            .handler   = crownstone_sdh_state_evt_handler,
 *            .p_context = __null
 *        };
 * In the linker file we have previously defined where we want such sections using a wildcard construction:
 *     .sdh_state_observers :
 *     {
 *         KEEP(*(SORT(.sdh_state_observers*)))
 *     } > FLASH
 *
 * Here '.sdh_state_observers*' is a wildcard pattern, SORT is a shortcut for SORT_BY_NAME and will sort the sections
 * alphabetically.
 */
NRF_SDH_STATE_OBSERVER(m_crownstone_state_handler, CROWNSTONE_STATE_OBSERVER_PRIO) = {
		.handler = crownstone_sdh_state_evt_handler, .p_context = NULL};

#if BUILD_MESHING == 1
// From: ble_softdevice_support.c
void mesh_soc_evt_handler_decoupled(void* p_event_data, [[maybe_unused]] uint16_t event_size) {
	LOGInterruptLevel("mesh_soc_evt_handler_decoupled int=%u", CsUtils::getInterruptLevel());
	uint32_t evt_id = *(uint32_t*)p_event_data;
	nrf_mesh_on_sd_evt(evt_id);
}

static void mesh_soc_evt_handler(uint32_t evt_id, [[maybe_unused]] void* p_context) {
	LOGInterruptLevel("mesh_soc_evt_handler int=%u", CsUtils::getInterruptLevel());
#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
	uint32_t retVal = app_sched_event_put(&evt_id, sizeof(evt_id), mesh_soc_evt_handler_decoupled);
	APP_ERROR_CHECK(retVal);
#else
	nrf_mesh_on_sd_evt(evt_id);
#endif
}

NRF_SDH_SOC_OBSERVER(m_mesh_soc_observer, MESH_SOC_OBSERVER_PRIO, mesh_soc_evt_handler, NULL);
#endif  // BUILD_MESHING == 1

/**
 * Scheduled FDS event handler callback.
 */
void fds_evt_handler_decoupled(void* p_event_data, [[maybe_unused]] uint16_t event_size) {
	LOGInterruptLevel("fds_evt_handler_decoupled int=%u", CsUtils::getInterruptLevel());
	Storage::getInstance().handleFileStorageEvent(reinterpret_cast<const fds_evt_t*>(p_event_data));
}

/**
 * The FDS event handler is registered in the cs_Storage class.
 */
void fds_evt_handler(const fds_evt_t* p_fds_evt) {
	LOGInterruptLevel("fds_evt_handler int=%u", CsUtils::getInterruptLevel());
	// For some reason, we already got fds event init, before app_sched_execute() was called.
	// So for now, just always put fds events on the app scheduler.
	//#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
	uint32_t retVal = app_sched_event_put(p_fds_evt, sizeof(*p_fds_evt), fds_evt_handler_decoupled);
	APP_ERROR_CHECK(retVal);
	//#else
	//	fds_evt_handler_decoupled(p_fds_evt, sizeof(*p_fds_evt));
	//#endif
}

#ifdef __cplusplus
}
#endif

void SocHandler::handleEvent(uint32_t event) {
	LOGInterruptLevel("handleEvent int=%u", CsUtils::getInterruptLevel());
	switch (event) {
		case NRF_EVT_POWER_FAILURE_WARNING: {
			// Set brownout flag, so next boot we know the cause.
			// Only works when we reset as well?
			// Only works when softdevice handler dispatch model is interrupt?
			GpRegRet::setFlag(GpRegRet::FLAG_BROWNOUT);
			[[fallthrough]];
		}
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
			uint32_t retVal = app_sched_event_put(&event, sizeof(event), crownstone_soc_evt_handler_decoupled);
			APP_ERROR_CHECK(retVal);
#else
			handleEventDecoupled(event);
#endif
			break;
		}
		case NRF_EVT_RADIO_BLOCKED:
		case NRF_EVT_RADIO_CANCELED:
		case NRF_EVT_RADIO_SESSION_IDLE:
		case NRF_EVT_RADIO_SESSION_CLOSED: {
			break;
		}
		default: {
			LOGd("Unhandled event: %u", event);
		}
	}
}

void SocHandler::handleEventDecoupled(uint32_t event) {
	LOGInterruptLevel("handleEventDecoupled int=%u", CsUtils::getInterruptLevel());
	switch (event) {
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

void BleHandler::handleEvent(const ble_evt_t* event) {
	LOGInterruptLevel("handleEvent int=%u event=%u", CsUtils::getInterruptLevel(), event->header.evt_id);
	switch (event->header.evt_id) {
		case BLE_GAP_EVT_ADV_REPORT: {
#if NRF_SDH_DISPATCH_MODEL == NRF_SDH_DISPATCH_MODEL_INTERRUPT
			Stack::getInstance().onBleEventInterrupt(event, true);
#else
			Stack::getInstance().onBleEventInterrupt(event, false);
#endif
			break;
		}
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
			BleHandler::handlePhyRequest(event->evt.gap_evt.conn_handle, event->evt.gap_evt.params.phy_update_request);
			break;
		}
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
			BleHandler::handleDataLengthRequest(
					event->evt.gap_evt.conn_handle,
					event->evt.gap_evt.params.data_length_update_request);
			break;
		}
		case BLE_GATTC_EVT_TIMEOUT: {
			// Disconnect on GATT Client timeout event.
			BleHandler::disconnect(event->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			break;
		}
		case BLE_GATTS_EVT_TIMEOUT: {
			// Disconnect on GATT Server timeout event.
			BleHandler::disconnect(event->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			break;
		}
		case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
			BleHandler::handleMtuRequest(
					event->evt.gatts_evt.conn_handle,
					event->evt.gatts_evt.params.exchange_mtu_request);
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
			uint32_t retVal = app_sched_event_put(event, sizeof(ble_evt_t), crownstone_sdh_ble_evt_handler_decoupled);
			// We don't want to miss any event.
			APP_ERROR_CHECK(retVal);
#else
			BleHandler::handleEventDecoupled(event);
#endif
			break;
		}
	}
}

void BleHandler::handleEventDecoupled(const ble_evt_t* event) {
	LOGInterruptLevel("handleEventDecoupled int=%u", CsUtils::getInterruptLevel());
	Stack::getInstance().onBleEvent(event);
}

void BleHandler::handlePhyRequest(
		uint16_t connectionHandle, [[maybe_unused]] const ble_gap_evt_phy_update_request_t& request) {
	ble_gap_phys_t phys;
	phys.rx_phys     = BLE_GAP_PHY_AUTO;
	phys.tx_phys     = BLE_GAP_PHY_AUTO;
	uint32_t nrfCode = sd_ble_gap_phy_update(connectionHandle, &phys);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			break;
		}
		case NRF_ERROR_INVALID_STATE: {
			// * @retval ::NRF_ERROR_INVALID_STATE No link has been established.
			// This can happen, when a phone connect, and disconnect quickly after.
			// This event is queued, but by the time we process it, the device already disconnected.
			break;
		}
		case NRF_ERROR_BUSY: {
			// * @retval ::NRF_ERROR_BUSY Procedure is already in progress or not allowed at this time. Process pending
			// events and wait for the pending procedure to complete and retry. This can happen: when a request is done,
			// the event is queued, but we haven't processed the event yet. The device does another request, so another
			// event is queued. Then, when the second event is being handled, we just replied to the first event.
			break;
		}
		case BLE_ERROR_INVALID_CONN_HANDLE: {
			// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied.
			// This shouldn't happen, but can maybe happen if the device already disconnected.
			break;
		}
		case NRF_ERROR_INVALID_ADDR: {
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
			[[fallthrough]];
		}
		case NRF_ERROR_INVALID_PARAM: {
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
			// This shouldn't happen: crash.
			[[fallthrough]];
		}
		case NRF_ERROR_NOT_SUPPORTED: {
			// * @retval ::NRF_ERROR_NOT_SUPPORTED Unsupported PHYs supplied to the call.
			// This shouldn't happen: crash.
			[[fallthrough]];
		}
		default: {
			APP_ERROR_HANDLER(nrfCode);
		}
	}
}

void BleHandler::handleDataLengthRequest(
		uint16_t connectionHandle, [[maybe_unused]] const ble_gap_evt_data_length_update_request_t& request) {
	uint32_t nrfCode = sd_ble_gap_data_length_update(connectionHandle, NULL, NULL);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			break;
		}
		case NRF_ERROR_INVALID_STATE: {
			// * @retval ::NRF_ERROR_INVALID_STATE No link has been established.
			// This can happen, when a phone connect, and disconnect quickly after.
			// This event is queued, but by the time we process it, the device already disconnected.
			break;
		}
		case NRF_ERROR_BUSY: {
			// * @retval ::NRF_ERROR_BUSY Peer has already initiated a Data Length Update Procedure. Process the
			// *                          pending @ref BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST event to respond.
			// This can happen: when a request is done, the event is queued, but we haven't processed the event yet.
			// The device does another request, so another event is queued.
			// Then, when the second event is being handled, we just replied to the first event.
			break;
		}
		case BLE_ERROR_INVALID_CONN_HANDLE: {
			// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle parameter supplied.
			// This shouldn't happen, but can maybe happen if the device already disconnected.
			break;
		}
		case NRF_ERROR_INVALID_ADDR: {
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
			[[fallthrough]];
		}
		case NRF_ERROR_INVALID_PARAM: {
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameters supplied.
			// This shouldn't happen: crash
			[[fallthrough]];
		}
		case NRF_ERROR_NOT_SUPPORTED: {
			// * @retval ::NRF_ERROR_NOT_SUPPORTED The requested parameters are not supported by the SoftDevice. Inspect
			// *                                   p_dl_limitation to see which parameter is not supported.
			// This shouldn't happen: crash
			[[fallthrough]];
		}
		case NRF_ERROR_RESOURCES: {
			// * @retval ::NRF_ERROR_RESOURCES The connection event length configured for this link is not sufficient
			// for the requested parameters.
			// *                               Use @ref sd_ble_cfg_set with @ref BLE_CONN_CFG_GAP to increase the
			// connection event length.
			// *                               Inspect p_dl_limitation to see where the limitation is.
			// This shouldn't happen: crash
			[[fallthrough]];
		}
		default: {
			APP_ERROR_HANDLER(nrfCode);
		}
	}
}

void BleHandler::handleMtuRequest(
		uint16_t connectionHandle, [[maybe_unused]] const ble_gatts_evt_exchange_mtu_request_t& request) {
	//	uint32_t nrfCode = sd_ble_gatts_exchange_mtu_reply(connectionHandle, BLE_GATT_ATT_MTU_DEFAULT);
	uint32_t nrfCode = sd_ble_gatts_exchange_mtu_reply(connectionHandle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			break;
		}
		case NRF_ERROR_INVALID_STATE: {
			// * @retval ::NRF_ERROR_INVALID_STATE Invalid Connection State or no ATT_MTU exchange request pending.
			// This can happen, when a phone connect, and disconnect quickly after.
			// This event is queued, but by the time we process it, the device already disconnected.
			break;
		}
		case NRF_ERROR_TIMEOUT: {
			// * @retval ::NRF_ERROR_TIMEOUT There has been a GATT procedure timeout. No new GATT procedure can be
			// performed without reestablishing the connection. This doesn't look like an error we should crash at.
			break;
		}
		case BLE_ERROR_INVALID_CONN_HANDLE: {
			// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid Connection Handle.
			// This shouldn't happen, but can maybe happen if the device already disconnected.
			break;
		}
		case NRF_ERROR_INVALID_PARAM: {
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid Server RX MTU size supplied.
			// This shouldn't happen, crash.
			[[fallthrough]];
		}
		default: {
			APP_ERROR_HANDLER(nrfCode);
		}
	}
}

void BleHandler::disconnect(uint16_t connectionHandle, uint8_t reason) {
	uint32_t nrfCode = sd_ble_gap_disconnect(connectionHandle, reason);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			break;
		}
		case NRF_ERROR_INVALID_STATE: {
			// * @retval ::NRF_ERROR_INVALID_STATE Disconnection in progress or link has not been established.
			// This can happen, when a phone connect, and disconnect quickly after.
			// This event is queued, but by the time we process it, the device already disconnected.
			break;
		}
		case BLE_ERROR_INVALID_CONN_HANDLE: {
			// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied.
			// This shouldn't happen, but can maybe happen if the device already disconnected.
			break;
		}
		case NRF_ERROR_INVALID_PARAM:
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
			// This shouldn't happen: crash.
			[[fallthrough]];
		default: {
			APP_ERROR_HANDLER(nrfCode);
		}
	}
}

void SdhStateHandler::handleEvent(const nrf_sdh_state_evt_t& event) {
	LOGInterruptLevel("handleEvent int=%u", CsUtils::getInterruptLevel());
	switch (event) {
		case NRF_SDH_EVT_STATE_ENABLE_PREPARE: {
			LOGd("Softdevice is about to be enabled");
			break;
		}
		case NRF_SDH_EVT_STATE_ENABLED: {
			LOGd("Softdevice is now enabled");
			break;
		}
		case NRF_SDH_EVT_STATE_DISABLE_PREPARE: {
			LOGd("Softdevice is about to be disabled");
			break;
		}
		case NRF_SDH_EVT_STATE_DISABLED: {
			LOGd("Softdevice is now disabled");
			break;
		}
		default: {
			LOGw("Softdevice is in unknown state: %u", event);
			break;
		}
	}
}
