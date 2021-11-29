/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <algorithm>
#include <ble/cs_BleCentral.h>
#include <ble/cs_Nordic.h>
#include <ble/cs_Stack.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_UuidConfig.h>
#include <common/cs_Handlers.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_Scanner.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <util/cs_Utils.h>

#define LOGStackDebug LOGnone


Stack::Stack() {
	_connectionKeepAliveTimerData = { {0} };
	_connectionKeepAliveTimerId = &_connectionKeepAliveTimerData;

	_connectionParams.min_conn_interval = MIN_CONNECTION_INTERVAL;
	_connectionParams.max_conn_interval = MAX_CONNECTION_INTERVAL;
	_connectionParams.slave_latency = SLAVE_LATENCY;
	_connectionParams.conn_sup_timeout = CONNECTION_SUPERVISION_TIMEOUT;
}

#define CS_STACK_LONG_WRITE_HEADER_SIZE 6

Stack::~Stack() {
	shutdown();
}

/**
 * Initialize SoftDevice, handlers, and brown-out.
 * In previous versions softdevice_ble_evt_handler_set could be called with a static function to be used as callback
 * function. Now, the macro NRF_SDH_BLE_OBSERVER ha been introduced instead.
 */
void Stack::init() {
	if (checkCondition(C_STACK_INITIALIZED, false)) return;
	LOGi("Init");

	// Check if SoftDevice is already running and disable it in that case (to restart later)
	uint8_t enabled;
	uint32_t nrfCode = sd_softdevice_is_enabled(&enabled);
	// Only return code is NRF_SUCCESS.
	APP_ERROR_CHECK(nrfCode);
	if (enabled) {
		LOGw("Softdevice already enabled");
		nrfCode = sd_softdevice_disable();
		// Only return code is NRF_SUCCESS.
		APP_ERROR_CHECK(nrfCode);
	}

	setInitialized(C_STACK_INITIALIZED);
}

/**
 * This function calls nrf_sdh_enable_request. Somehow, when called after a reboot. This works fine. However, when
 * called for the first time this leads to a hard fault. APP_ERROR_CHECK is not even called. The second time it
 * goes through the event that the Storage is initialized doesn't come through.
 */
void Stack::initSoftdevice() {

	// TODO: why is there app_sched_execute here?
	app_sched_execute();

	ret_code_t nrfCode;
	LOGi("Init softdevice");
	LOGd("Softdevice enabled=%u suspended=%u", nrf_sdh_is_enabled(), nrf_sdh_is_suspended());

	LOGd("nrf_sdh_enable_request");
	nrfCode = nrf_sdh_enable_request();
	switch (nrfCode) {
		case NRF_SUCCESS:
			// * @retval  NRF_SUCCESS                 The process is started.
			break;
		case NRF_ERROR_INVALID_STATE:
			// * @retval  NRF_ERROR_INVALID_STATE     The SoftDevice is already enabled.
			LOGw("Softdevice already enabled");
			return;
		default:
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}

	// TODO: remove this loop:
	// This is an asynchronous process, and should just return ERR_WAIT_FOR_SUCCESS, and wait for an event.
	while (!nrf_sdh_is_enabled()) {
		LOGe("Softdevice not enabled");
		// The status change has been deferred.
		app_sched_execute();
	}

	// Enable power fail detection:
	// If the supply voltage drops below threshold, an NRF_EVT_POWER_FAILURE_WARNING will be triggered.
	// TODO: move the power fail code to main cpp.
	nrfCode = sd_power_pof_enable(true);
	// Only return code is NRF_SUCCESS.
	APP_ERROR_CHECK(nrfCode);

	nrfCode = sd_power_pof_threshold_set(BROWNOUT_TRIGGER_THRESHOLD);
	APP_ERROR_CHECK(nrfCode);
}

/** Initialize or configure the BLE radio.
 *
 * We are using the S132 softdevice. Online documentation can be found at the Nordic website:
 *
 * - http://infocenter.nordicsemi.com/topic/com.nordic.infocenter.softdevices52/dita/softdevices/s130/s130api.html
 * - http://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.0.0/modules.html
 * - http://infocenter.nordicsemi.com/pdf/S132_SDS_v6.0.pdf
 *
 * The easiest is to follow the migration documents.
 *
 * - https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.0.0/migration.html?cp=4_0_0_1_9
 * - https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v14.0.0/migration.html?cp=4_0_3_1_7
 *
 * Set BLE parameters now in <third/nrf/sdk_config.h>:
 *   NRF_SDH_BLE_PERIPHERAL_LINK_COUNT = 1
 *   NRF_SDH_BLE_CENTRAL_LINK_COUNT = 0
 */
void Stack::initRadio() {
	if (checkCondition(C_RADIO_INITIALIZED, false) || !checkCondition(C_STACK_INITIALIZED, true)) {
		APP_ERROR_HANDLER(NRF_ERROR_INVALID_STATE);
	}

	ret_code_t nrfCode;

	LOGi(FMT_INIT, "radio");
	// Enable BLE stack
	uint32_t ram_start = g_RAM_R1_BASE;
//	uint32_t ram_start = 0;
	LOGd("nrf_sdh_ble_default_cfg_set at %p", ram_start);
	// TODO: make a separate function, that tells you what to set RAM_R1_BASE to.
	// TODO: make a unit test for that.
	nrfCode = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	switch (nrfCode) {
		case NRF_SUCCESS:
			break;
		case NRF_ERROR_NO_MEM:
			LOGe("Unrecoverable, memory softdevice and app overlaps. RAM_R1_BASE should be: %p", ram_start);
		case NRF_ERROR_INVALID_LENGTH:
			LOGe("RAM, invalid length");
		default:
			APP_ERROR_HANDLER(nrfCode);
	}

	if (ram_start != g_RAM_R1_BASE) {
		LOGw("Application address is too high, memory is unused: %p", ram_start);
	}

	LOGd("nrf_sdh_ble_enable ram_start=%p", ram_start);
	nrfCode = nrf_sdh_ble_enable(&ram_start);
	switch (nrfCode) {
		case NRF_SUCCESS:
			LOGi("Softdevice enabled");
			break;
		case NRF_ERROR_INVALID_STATE:
			LOGe("BLE: invalid radio state");
		case NRF_ERROR_INVALID_ADDR:
			LOGe("BLE: invalid memory address");
		case NRF_ERROR_NO_MEM:
			// Read out ram_start, use that as RAM_R1_BASE, and adjust RAM_APPLICATION_AMOUNT.
			LOGe("BLE: no memory available, RAM_R1_BASE should be %p", ram_start);
		default:
			// Crash.
			APP_ERROR_HANDLER(nrfCode);
	}

#ifdef DEBUG
	// Version is not saved or shown yet
	ble_version_t version( { });
	version.company_id = 12;
	nrfCode = sd_ble_version_get(&version);
	APP_ERROR_CHECK(nrfCode);
	LOGd("BLE version: company=%u version=%u.%u", version.company_id, version.version_number, version.subversion_number);
#endif

	setInitialized(C_RADIO_INITIALIZED);

	updateConnParams();
}

void Stack::setClockSource(nrf_clock_lf_cfg_t clockSource) {
	_clockSource = clockSource;
}

void Stack::createCharacteristics() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) {
		APP_ERROR_HANDLER(NRF_ERROR_INVALID_STATE);
	}
	LOGd("Create characteristics");

	// Init buffers.
	CharacteristicReadBuffer::getInstance().alloc(g_MASTER_BUFFER_SIZE);
	CharacteristicWriteBuffer::getInstance().alloc(g_MASTER_BUFFER_SIZE);

	for (Service* svc: _services) {
		svc->createCharacteristics();
	}
}

void Stack::initServices() {
	if (!checkCondition(C_RADIO_INITIALIZED, true) || checkCondition(C_SERVICES_INITIALIZED, false)) {
		APP_ERROR_HANDLER(NRF_ERROR_INVALID_STATE);
	}
	LOGd("Init services");

	for (Service* svc : _services) {
		svc->init(this);
	}

	setInitialized(C_SERVICES_INITIALIZED);
}

void Stack::shutdown() {
	// 16-sep-2019 TODO: stop advertising
	// 02-11-2021 TODO: we don't support shutdown.
	setUninitialized(C_STACK_INITIALIZED);
}

Stack& Stack::addService(Service* svc) {
	_services.push_back(svc);
	return *this;
}

void Stack::updateMinConnectionInterval(uint16_t connectionInterval_1_25_ms) {
	if (_connectionParams.min_conn_interval != connectionInterval_1_25_ms) {
		_connectionParams.min_conn_interval = connectionInterval_1_25_ms;
		updateConnParams();
	}
}

void Stack::updateMaxConnectionInterval(uint16_t connectionInterval_1_25_ms) {
	if (_connectionParams.max_conn_interval != connectionInterval_1_25_ms) {
		_connectionParams.max_conn_interval = connectionInterval_1_25_ms;
		updateConnParams();
	}
}

void Stack::updateSlaveLatency(uint16_t slaveLatency) {
	if ( _connectionParams.slave_latency != slaveLatency ) {
		_connectionParams.slave_latency = slaveLatency;
		updateConnParams();
	}
}

void Stack::updateConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms) {
	if (_connectionParams.conn_sup_timeout != conSupTimeout_10_ms) {
		_connectionParams.conn_sup_timeout = conSupTimeout_10_ms;
		updateConnParams();
	}
}

void Stack::updateConnParams() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) {
		APP_ERROR_HANDLER(NRF_ERROR_INVALID_STATE);
	}
	uint32_t nrfCode = sd_ble_gap_ppcp_set(&_connectionParams);
	switch (nrfCode) {
		case NRF_SUCCESS:
			// * @retval ::NRF_SUCCESS Peripheral Preferred Connection Parameters set successfully.
			break;
		case NRF_ERROR_INVALID_ADDR:
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
		case NRF_ERROR_INVALID_PARAM:
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
			// This shouldn't happen: crash.
		default:
			// Crash.
			APP_ERROR_HANDLER(nrfCode);
	}
}



/** Utility function which logs unexpected state
 *
 */
bool Stack::checkCondition(condition_t condition, bool expectation) {
	bool field = false;
	switch (condition) {
		case C_STACK_INITIALIZED:
			field = isInitialized(C_STACK_INITIALIZED);
			if (expectation != field) {
				LOGw("Stack init=%u", field);
			}
			break;
		case C_RADIO_INITIALIZED:
			field = isInitialized(C_RADIO_INITIALIZED);
			if (expectation != field) {
				LOGw("Radio init=%u", field);
			}
			break;
		case C_SERVICES_INITIALIZED:
			field = isInitialized(C_SERVICES_INITIALIZED);
			if (expectation != field) {
				LOGw("Services init=%u", field);
			}
			break;
	}
	return field;
}


void Stack::startScanning() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) {
		return;
	}
	if (_scanning) {
		return;
	}

//	LOGi(FMT_START, "scanning");
	ble_gap_scan_params_t scanParams;
	scanParams.extended = 0;
	scanParams.report_incomplete_evts = 0;
	scanParams.active = 1;
	scanParams.filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL; // Scanning filter policy. See BLE_GAP_SCAN_FILTER_POLICIES
	scanParams.scan_phys = BLE_GAP_PHY_1MBPS;
	scanParams.timeout = BLE_GAP_SCAN_TIMEOUT_UNLIMITED; // Scan timeout in 10 ms units. See BLE_GAP_SCAN_TIMEOUT.
	scanParams.channel_mask[0] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[1] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[2] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[3] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[4] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_INTERVAL_625US, &scanParams.interval, sizeof(scanParams.interval));
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_WINDOW_625US, &scanParams.window, sizeof(scanParams.window));

	uint32_t nrfCode = sd_ble_gap_scan_start(&scanParams, &_scanBufferStruct);
	switch (nrfCode) {
		case NRF_SUCCESS:
			// * @retval ::NRF_SUCCESS Successfully initiated scanning procedure.
			break;
		case NRF_ERROR_INVALID_STATE:
			// * @retval ::NRF_ERROR_INVALID_STATE Invalid state to perform operation. Either:
			// *                                   - Scanning is already ongoing and p_scan_params was not NULL
			// *                                   - Scanning is not running and p_scan_params was NULL.
			// *                                   - The scanner has timed out when this function is called to continue scanning.
			// Since scan params is not NULL, we must already be scanning, so just update the cached state.
			_scanning = true;
			return;
		case NRF_ERROR_RESOURCES:
			// * @retval ::NRF_ERROR_RESOURCES Not enough BLE role slots available.
			// *                               Stop one or more currently active roles (Central, Peripheral or Broadcaster) and try again
			// Can happen? Let's not crash.
			return;
		case NRF_ERROR_INVALID_ADDR:
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
		case NRF_ERROR_INVALID_PARAM:
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied. See @ref ble_gap_scan_params_t.
			// This shouldn't happen: crash.
		case NRF_ERROR_NOT_SUPPORTED:
			// * @retval ::NRF_ERROR_NOT_SUPPORTED Unsupported parameters supplied. See @ref ble_gap_scan_params_t.
			// * @retval ::NRF_ERROR_NOT_SUPPORTED Unsupported PHYs supplied to the call.
			// This shouldn't happen: crash.
		case NRF_ERROR_INVALID_LENGTH:
			// * @retval ::NRF_ERROR_INVALID_LENGTH The provided buffer length is invalid. See @ref BLE_GAP_SCAN_BUFFER_MIN.
			// This shouldn't happen: crash.
		default:
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}

	_scanning = true;

	event_t event(CS_TYPE::EVT_SCAN_STARTED, NULL, 0);
	EventDispatcher::getInstance().dispatch(event);
}


void Stack::stopScanning() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) {
		return;
	}
	if (!_scanning) {
		return;
	}

	uint32_t nrfCode = sd_ble_gap_scan_stop();
	switch (nrfCode) {
		case NRF_SUCCESS:
			// * @retval ::NRF_SUCCESS Successfully stopped scanning procedure.
			break;
		case NRF_ERROR_INVALID_STATE:
			// * @retval ::NRF_ERROR_INVALID_STATE Not in the scanning state.
			// Already stopped, so just update the cached state.
			_scanning = false;
			return;
		default:
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}
	_scanning = false;

	event_t event(CS_TYPE::EVT_SCAN_STOPPED, NULL, 0);
	EventDispatcher::getInstance().dispatch(event);
}

bool Stack::isScanning() {
	return _scanning;
}

void Stack::setAesEncrypted(bool encrypted) {
	for (Service* svc : _services) {
		svc->setAesEncrypted(encrypted);
	}
}

void Stack::onBleEvent(const ble_evt_t * p_ble_evt) {
	if (p_ble_evt->header.evt_id != BLE_GAP_EVT_RSSI_CHANGED && p_ble_evt->header.evt_id != BLE_GAP_EVT_ADV_REPORT) {
		LOGStackDebug("BLE event $nordicEventTypeName(%u) (0x%X)", p_ble_evt->header.evt_id, p_ble_evt->header.evt_id);
	}

	switch (p_ble_evt->header.evt_id) {
		case BLE_EVT_USER_MEM_REQUEST: {
			onMemoryRequest(p_ble_evt->evt.gap_evt.conn_handle);
			break;
		}
		case BLE_EVT_USER_MEM_RELEASE: {
			onMemoryRelease(p_ble_evt->evt.gap_evt.conn_handle);
			break;
		}

		// ---- GAP events ---- //
		case BLE_GAP_EVT_CONNECTED: {
			onConnect(p_ble_evt);
			break;
		}
		case BLE_GAP_EVT_DISCONNECTED: {
			onDisconnect(p_ble_evt);
			break;
		}
		case BLE_GAP_EVT_TIMEOUT: {
			onGapTimeout(p_ble_evt->evt.gap_evt.params.timeout.src);
			break;
		}
		case BLE_GAP_EVT_RSSI_CHANGED: {
			break;
		}

		// ---- GATTS events ---- //
		case BLE_GATTS_EVT_TIMEOUT: {
			break;
		}
		case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: {
			break;
		}
		case BLE_GATTS_EVT_WRITE: {
			onWrite(p_ble_evt->evt.gatts_evt.params.write);
			break;
		}
		case BLE_GATTS_EVT_HVC: {
			break;
		}
		case BLE_GATTS_EVT_SYS_ATTR_MISSING: {
			// TODO: move to interrupt?
			LOGd("Set system attributes.");
			uint32_t nrfCode = sd_ble_gatts_sys_attr_set(_connectionHandle, NULL, 0, BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
			switch (nrfCode) {
				case NRF_SUCCESS:
					// * @retval ::NRF_SUCCESS Successfully set the system attribute information.
					break;
				case BLE_ERROR_INVALID_CONN_HANDLE:
					// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid Connection Handle.
					// This shouldn't happen, as the connection handle is only set in the main thread.
					// But we can safely ignore this case.
					LOGe("Invalid handle");
					break;
				case NRF_ERROR_INVALID_STATE:
					// * @retval ::NRF_ERROR_INVALID_STATE Invalid Connection State.
					// Can happen that we disconnected before we handle this event.
					break;
				case NRF_ERROR_INVALID_ADDR:
					// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
					// This shouldn't happen: crash.
				case NRF_ERROR_INVALID_PARAM:
					// * @retval ::NRF_ERROR_INVALID_PARAM Invalid flags supplied.
					// This shouldn't happen: crash.
				case NRF_ERROR_INVALID_DATA:
					// * @retval ::NRF_ERROR_INVALID_DATA Invalid data supplied, the data should be exactly the same as retrieved with @ref sd_ble_gatts_sys_attr_get.
					// This shouldn't happen: crash.
				case NRF_ERROR_NO_MEM:
					// * @retval ::NRF_ERROR_NO_MEM Not enough memory to complete operation.
					// This shouldn't happen: crash.
				default:
					// Crash
					APP_ERROR_HANDLER(nrfCode);
			}
			break;
		}
		case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
			onTxComplete(p_ble_evt);
			break;
		}
		default: {
			break;
		}
	}

	BleCentral::getInstance().onBleEvent(p_ble_evt);
}

struct cs_stack_scan_t {
	ble_gap_evt_adv_report_t advReport;
	uint8_t dataSize;
	uint8_t data[31]; // Same size as _scanBuffer
};

void csStackOnScan(const ble_gap_evt_adv_report_t* advReport) {
	scanned_device_t scan;
	memcpy(scan.address, advReport->peer_addr.addr, sizeof(scan.address)); // TODO: check addr_type and addr_id_peer
	scan.resolvedPrivateAddress = advReport->peer_addr.addr_id_peer;
	scan.addressType = advReport->peer_addr.addr_type;
	scan.rssi = advReport->rssi;
	scan.channel = advReport->ch_index;
	scan.dataSize = advReport->data.len;
	scan.data = advReport->data.p_data;

//	uint16_t type = *((uint16_t*)&(advReport->type));
//	const uint8_t* addr = scan.address;
//	const uint8_t* p = scan.data;
//	if (p[0] == 0x15 && p[1] == 0x16 && p[2] == 0x01 && p[3] == 0xC0 && p[4] == 0x05) {
////		if (p[1] == 0xFF && p[2] == 0xCD && p[3] == 0xAB) {
////		if (advReport->peer_addr.addr_type == BLE_GAP_ADDR_TYPE_PUBLIC && addr[5] == 0xE7 && addr[4] == 0x09 && addr[3] == 0x62) { // E7:09:62:02:91:3D
////		if (addr[5] == 0xE7 && addr[4] == 0x09 && addr[3] == 0x62) { // E7:09:62:02:91:3D
//		LOGi("Stack scan: address=%02X:%02X:%02X:%02X:%02X:%02X addrType=%u type=%u rssi=%i chan=%u", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], advReport->peer_addr.addr_type, type, scan.rssi, scan.channel);
//		LOGi("  adv_type=%u len=%u data=", type, scan.dataSize);
//		CsUtils::printArray(scan.data, scan.dataSize);
//	}
	event_t event(CS_TYPE::EVT_DEVICE_SCANNED, (void*)&scan, sizeof(scan));
	EventDispatcher::getInstance().dispatch(event);
}

void csStackOnScan(void * p_event_data, uint16_t event_size) {
	cs_stack_scan_t* scanEvent = (cs_stack_scan_t*)p_event_data;
	scanEvent->advReport.data.p_data = scanEvent->data;
	const ble_gap_evt_adv_report_t* advReport = &(scanEvent->advReport);
	csStackOnScan(advReport);
}

void Stack::onBleEventInterrupt(const ble_evt_t * p_ble_evt, bool isInterrupt) {
	switch (p_ble_evt->header.evt_id) {
		case BLE_GAP_EVT_ADV_REPORT: {
			const uint16_t status = p_ble_evt->evt.gap_evt.params.adv_report.type.status;
			if (status != BLE_GAP_ADV_DATA_STATUS_COMPLETE) {
				LOGw("adv report status=%u", status);
				break;
			}

			if (isInterrupt) {
				// Handle scan via app scheduler. The app scheduler copies the data.
				// But, since the payload data is a pointer, that will not be copied.
				// So copy the payload data as well.
				// This copy of advReport + payload data will be copied again by the scheduler.
				// TODO: use multiple scan buffers?
				cs_stack_scan_t scan;
				memcpy(&(scan.advReport), &(p_ble_evt->evt.gap_evt.params.adv_report), sizeof(ble_gap_evt_adv_report_t));
				scan.dataSize = p_ble_evt->evt.gap_evt.params.adv_report.data.len;
				memcpy(scan.data, p_ble_evt->evt.gap_evt.params.adv_report.data.p_data, scan.dataSize);
				scan.advReport.data.p_data = NULL; // This pointer can't be set now, as the data is copied by scheduler.

				uint16_t schedulerSpace = app_sched_queue_space_get();
				if (schedulerSpace > SCHED_QUEUE_SIZE - SCHEDULER_QUEUE_ALMOST_FULL) {
					uint32_t nrfCode = app_sched_event_put(&scan, sizeof(scan), csStackOnScan);
					APP_ERROR_CHECK(nrfCode);
				}
			}
			else {
				// Handle scan immediately, since we're already on thread level.
				csStackOnScan(&(p_ble_evt->evt.gap_evt.params.adv_report));
			}

			// If ble_gap_adv_report_type_t::status is set to BLE_GAP_ADV_DATA_STATUS_INCOMPLETE_MORE_DATA,
			//      not all fields in the advertising report may be available.
			// Else, scanning will be paused. To continue scanning, call sd_ble_gap_scan_start.

			// Resume scanning: ignore _scanning state as this is executed in an interrupt. Rely on return value instead.
			uint32_t nrfCode = sd_ble_gap_scan_start(NULL, &_scanBufferStruct);
			switch (nrfCode) {
				case NRF_SUCCESS:
					// * @retval ::NRF_SUCCESS Successfully initiated scanning procedure.
					break;
				case NRF_ERROR_INVALID_STATE:
					// * @retval ::NRF_ERROR_INVALID_STATE Invalid state to perform operation. Either:
					// *                                   - Scanning is already ongoing and p_scan_params was not NULL
					// *                                   - Scanning is not running and p_scan_params was NULL.
					// *                                   - The scanner has timed out when this function is called to continue scanning.
					// Since scan params is NULL, we must not be scanning.
					break;
				case NRF_ERROR_RESOURCES:
					// * @retval ::NRF_ERROR_RESOURCES Not enough BLE role slots available.
					// *                               Stop one or more currently active roles (Central, Peripheral or Broadcaster) and try again
					// Can happen? Let's not crash.
					LOGw("No resources to continue scanning");
					break;
				case NRF_ERROR_INVALID_ADDR:
					// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
					// This shouldn't happen: crash.
				case NRF_ERROR_INVALID_PARAM:
					// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied. See @ref ble_gap_scan_params_t.
					// This shouldn't happen: crash.
				case NRF_ERROR_NOT_SUPPORTED:
					// * @retval ::NRF_ERROR_NOT_SUPPORTED Unsupported parameters supplied. See @ref ble_gap_scan_params_t.
					// * @retval ::NRF_ERROR_NOT_SUPPORTED Unsupported PHYs supplied to the call.
					// This shouldn't happen: crash.
				case NRF_ERROR_INVALID_LENGTH:
					// * @retval ::NRF_ERROR_INVALID_LENGTH The provided buffer length is invalid. See @ref BLE_GAP_SCAN_BUFFER_MIN.
					// This shouldn't happen: crash.
				default:
					// Crash
					APP_ERROR_HANDLER(nrfCode);
			}
			break;
		}
	}
}

static void connection_keep_alive_timeout(void* p_context) {
	LOGw("connection keep alive timeout!");
	Stack::getInstance().disconnect();
}

void Stack::startConnectionAliveTimer() {
	Timer::getInstance().createSingleShot(_connectionKeepAliveTimerId, connection_keep_alive_timeout);
	Timer::getInstance().start(_connectionKeepAliveTimerId, MS_TO_TICKS(g_CONNECTION_ALIVE_TIMEOUT), NULL);
}

void Stack::stopConnectionAliveTimer() {
	Timer::getInstance().stop(_connectionKeepAliveTimerId);
}

void Stack::resetConnectionAliveTimer() {
	Timer::getInstance().reset(_connectionKeepAliveTimerId, MS_TO_TICKS(g_CONNECTION_ALIVE_TIMEOUT), NULL);
}

void Stack::onMemoryRequest(uint16_t connectionHandle) {
	// See: https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.1.1/group___b_l_e___g_a_t_t_s___q_u_e_u_e_d___w_r_i_t_e___b_u_f___n_o_a_u_t_h___m_s_c.html
	// You can check which type is requested: p_ble_evt->evt.common_evt.params.user_mem_request.type
	// Currently only option is: BLE_USER_MEM_TYPE_GATTS_QUEUED_WRITES
	// See https://devzone.nordicsemi.com/f/nordic-q-a/33366/is-it-necessary-handle-ble_evt_user_mem_request-respectivly-is-it-required-to-support-prepared-writes
	// And https://devzone.nordicsemi.com/f/nordic-q-a/53074/ble_evt_user_mem_request-if-data_length-is-180-bytes-on-ios-but-not-for-android
	// And https://devzone.nordicsemi.com/f/nordic-q-a/50043/ble_evt_user_mem_request-patterns
	// Also see https://interrupt.memfault.com/blog/ble-throughput-primer
//		BLE_CALL(sd_ble_user_mem_reply, (connectionHandle, NULL));

	ble_user_mem_block_t memBlock;
	cs_data_t writeBuffer = CharacteristicWriteBuffer::getInstance().getBuffer(CS_CHAR_BUFFER_DEFAULT_OFFSET - CS_STACK_LONG_WRITE_HEADER_SIZE);
	memBlock.p_mem = writeBuffer.data;
	memBlock.len = writeBuffer.len;
	uint32_t nrfCode = sd_ble_user_mem_reply(connectionHandle, &memBlock);
	switch (nrfCode) {
		case NRF_SUCCESS:
			// * @retval ::NRF_SUCCESS Successfully queued a response to the peer.
			break;
		case NRF_ERROR_BUSY:
			// * @retval ::NRF_ERROR_BUSY The stack is busy, process pending events and retry.
			LOGw("Can't reply to mem request: busy");
			break;
		case NRF_ERROR_INVALID_STATE:
			// * @retval ::NRF_ERROR_INVALID_STATE Invalid Connection state or no user memory request pending.
			// This can happen: the phone disconnected before we handle this event.
			break;
		case BLE_ERROR_INVALID_CONN_HANDLE:
			// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid Connection Handle.
			// This shouldn't happen, as the connection handle is only set in the main thread.
			// But we can safely ignore this case.
			LOGe("Invalid handle");
			break;
		case NRF_ERROR_INVALID_ADDR:
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
		case NRF_ERROR_INVALID_LENGTH:
			// * @retval ::NRF_ERROR_INVALID_LENGTH Invalid user memory block length supplied.
			// This shouldn't happen: crash.
		default:
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}
}

void Stack::onMemoryRelease(uint16_t connectionHandle) {
	// No need to do anything
}

void Stack::onWrite(const ble_gatts_evt_write_t& writeEvt) {
	if (isDisconnecting()) {
		LOGw("Discard write: disconnect in progress.");
		return;
	}

	resetConnectionAliveTimer();

	if (writeEvt.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {
		cs_data_t writeBuffer = CharacteristicWriteBuffer::getInstance().getBuffer(CS_CHAR_BUFFER_DEFAULT_OFFSET - CS_STACK_LONG_WRITE_HEADER_SIZE);
		uint16_t* header = (uint16_t*)(writeBuffer.data);
		for (Service* svc : _services) {
			// for a long write, don't have the service handle available to check for the correct
			// service, so we just go through all the services and characteristics until we find
			// the correct characteristic, then we return
			if (svc->on_write(writeEvt, header[0])) {
				return;
			}
		}
	}
	else {
		for (Service* svc : _services) {
			svc->on_write(writeEvt, writeEvt.handle);
		}
	}
}



void Stack::onConnect(const ble_evt_t * p_ble_evt) {
	_connectionHandle = p_ble_evt->evt.gap_evt.conn_handle;
	_disconnectingInProgress = false;

		if (g_ENABLE_RSSI_FOR_CONNECTION) {
			uint32_t nrfCode = sd_ble_gap_rssi_start(_connectionHandle, 0, 0);
			switch (nrfCode) {
				case NRF_SUCCESS:
					// * @retval ::NRF_SUCCESS                   Successfully activated RSSI reporting.
					break;
				case NRF_ERROR_INVALID_STATE:
					// * @retval ::NRF_ERROR_INVALID_STATE       RSSI reporting is already ongoing.
					// Ignore this error.
					break;
				case BLE_ERROR_INVALID_CONN_HANDLE:
					// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied.
					// This shouldn't happen, as the connection handle is only set in the main thread.
					// But we can safely ignore this case.
					LOGe("Invalid handle: %u", _connectionHandle);
					break;
				default:
					// Crash
					APP_ERROR_HANDLER(nrfCode);
			}
		}

		switch (p_ble_evt->evt.gap_evt.params.connected.role) {
			case BLE_GAP_ROLE_PERIPH: {
				LOGi("onConnect as peripheral");
				_connectionIsOutgoing = false;
				break;
			}
			case BLE_GAP_ROLE_CENTRAL: {
				LOGi("onConnect as central");
				_connectionIsOutgoing = true;
				break;
			}
			default: {
				LOGw("onConnect with unknown role");
				break;
			}
		}

		if (_connectionIsOutgoing) {
		}
		else {
			LOGStackDebug("adv: handle=%u data=%p scanResponse=%p",
					p_ble_evt->evt.gap_evt.params.connected.adv_handle,
					p_ble_evt->evt.gap_evt.params.connected.adv_data.adv_data.p_data,
					p_ble_evt->evt.gap_evt.params.connected.adv_data.scan_rsp_data.p_data);
			onIncomingConnected(p_ble_evt);
		}
}

void Stack::onConnectionTimeout() {
	LOGd("onConnectionTimeout");
}

void Stack::onDisconnect(const ble_evt_t * p_ble_evt) {
	_connectionHandle = BLE_CONN_HANDLE_INVALID;

	if (_connectionIsOutgoing) {
	}
	else {
		onIncomingDisconnected(p_ble_evt);
	}
}

void Stack::onGapTimeout(uint8_t src) {
	switch (src) {
		case BLE_GAP_TIMEOUT_SRC_CONN: {
			onConnectionTimeout();
			break;
		}
		case BLE_GAP_TIMEOUT_SRC_SCAN: {
			LOGi("Scan timeout");
			break;
		}
		default: {
			LOGi("GAP timeout");
			break;
		}
	}
}


void Stack::onIncomingConnected(const ble_evt_t * p_ble_evt) {
	LOGi("Device connected");

	for (Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}

	startConnectionAliveTimer();

	const ble_gap_evt_connected_t& connectedData = p_ble_evt->evt.gap_evt.params.connected;

	TYPIFY(EVT_BLE_CONNECT) eventData = {
		.connectionHandle    = p_ble_evt->evt.gap_evt.conn_handle,
		.advertisementHandle = connectedData.adv_handle,
		.advertisementBuffer = cs_data_t(connectedData.adv_data.adv_data.p_data, connectedData.adv_data.adv_data.len),
		.scanResponseBuffer  = cs_data_t(connectedData.adv_data.scan_rsp_data.p_data, connectedData.adv_data.scan_rsp_data.len)
	};

	event_t event(CS_TYPE::EVT_BLE_CONNECT, &eventData, sizeof(eventData));
	event.dispatch();
}

void Stack::onIncomingDisconnected(const ble_evt_t * p_ble_evt) {
	LOGi("Device disconnected");

	for (Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}

	stopConnectionAliveTimer();

	event_t event(CS_TYPE::EVT_BLE_DISCONNECT);
	event.dispatch();
}

void Stack::disconnect() {
	// Only disconnect when we are actually connected to something
	if (_connectionHandle != BLE_CONN_HANDLE_INVALID && _disconnectingInProgress == false) {
		_disconnectingInProgress = true;
		LOGi("Forcibly disconnecting from device");
		// It seems like we're only allowed to use BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION.
		// This sometimes gives us an NRF_ERROR_INVALID_STATE (disconnection is already in progress)
		// NRF_ERROR_INVALID_STATE can safely be ignored, see: https://devzone.nordicsemi.com/question/81108/handling-nrf_error_invalid_state-error-code/
		// BLE_ERROR_INVALID_CONN_HANDLE can safely be ignored, see: https://devzone.nordicsemi.com/f/nordic-q-a/34353/error-0x3002/132078#132078
		uint32_t nrfCode = sd_ble_gap_disconnect(_connectionHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		switch (nrfCode) {
			case NRF_SUCCESS:
				// * @retval ::NRF_SUCCESS The disconnection procedure has been started successfully.
				break;
			case NRF_ERROR_INVALID_STATE:
				// * @retval ::NRF_ERROR_INVALID_STATE Disconnection in progress or link has not been established.
				// This can happen, when we disconnected in the meantime.
				break;
			case BLE_ERROR_INVALID_CONN_HANDLE:
				// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied.
				// This shouldn't happen, as the connection handle is only set in the main thread.
				// But it was ignored before.
				LOGe("Invalid handle");
				break;
			case NRF_ERROR_INVALID_PARAM:
				// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
				// This shouldn't happen: crash.
			default:
				// Crash
				APP_ERROR_HANDLER(nrfCode);
		}
	}
}

bool Stack::isDisconnecting() {
	return _disconnectingInProgress;
};

bool Stack::isConnected() {
	return _connectionHandle != BLE_CONN_HANDLE_INVALID;
}

bool Stack::isConnectedPeripheral() {
	return isConnected() && !_connectionIsOutgoing;
}

void Stack::onTxComplete(const ble_evt_t * p_ble_evt) {
	for (Service* svc: _services) {
		svc->onTxComplete(&p_ble_evt->evt.common_evt);
	}
}


