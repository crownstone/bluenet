/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <algorithm>
#include <ble/cs_Nordic.h>
#include <ble/cs_Stack.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_UuidConfig.h>
#include <common/cs_Handlers.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_Scanner.h>
#include <storage/cs_State.h>
#include "structs/buffer/cs_CharacteristicReadBuffer.h"
#include "structs/buffer/cs_CharacteristicWriteBuffer.h"
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

void handle_discovery(ble_db_discovery_evt_t* event) {
	Stack::getInstance().onDiscoveryEvent(event);
}

/**
 * Initialize SoftDevice, handlers, and brown-out.
 * In previous versions softdevice_ble_evt_handler_set could be called with a static function to be used as callback
 * function. Now, the macro NRF_SDH_BLE_OBSERVER ha been introduced instead.
 */
void Stack::init() {
	if (checkCondition(C_STACK_INITIALIZED, false)) return;
	LOGi(FMT_INIT, "stack");

	ret_code_t ret_code;

	// Check if SoftDevice is already running and disable it in that case (to restart later)
	uint8_t enabled;
	ret_code = sd_softdevice_is_enabled(&enabled);
	APP_ERROR_CHECK(ret_code);
	if (enabled) {
		LOGw(MSG_BLE_SOFTDEVICE_RUNNING);
		ret_code = sd_softdevice_disable();
		APP_ERROR_CHECK(ret_code);
	}

	// now done through NRF_SDH_BLE_OBSERV
	//softdevice_ble_evt_handler_set(ble_evt_dispatch);
	//LOGd("Address of sdh_ble_observers", &sdh_ble_observers);

	// Enable power-fail comparator
	sd_power_pof_enable(true);
	// set threshold value, if power falls below threshold,
	// an NRF_EVT_POWER_FAILURE_WARNING will be triggered.
	sd_power_pof_threshold_set(BROWNOUT_TRIGGER_THRESHOLD);

	LOGd("Stack initialized");
	setInitialized(C_STACK_INITIALIZED);
}

/**
 * This function calls nrf_sdh_enable_request. Somehow, when called after a reboot. This works fine. However, when
 * called for the first time this leads to a hard fault. APP_ERROR_CHECK is not even called. The second time it
 * goes through the event that the Storage is initialized doesn't come through.
 */
void Stack::initSoftdevice() {

	app_sched_execute();

	ret_code_t ret_code;
	LOGi(FMT_INIT, "softdevice");
	if (nrf_sdh_is_enabled()) {
		LOGd("Softdevice is enabled");
	} else {
		LOGd("Softdevice is currently not enabled");
	}
	if (nrf_sdh_is_suspended()) {
		LOGd("Softdevice is suspended");
	} else {
		LOGd("Softdevice is currently not suspended");
	}
	LOGd("nrf_sdh_enable_request");
	ret_code = nrf_sdh_enable_request();
	if (ret_code == NRF_ERROR_INVALID_STATE) {
		LOGw("Softdevice, already initialized");
		return;
	}
	LOGd("Error: %i", ret_code);
	APP_ERROR_CHECK(ret_code);

	while (!nrf_sdh_is_enabled()) {
		LOGe("Softdevice, not enabled");
		// The status change has been deferred.
		app_sched_execute();
	}
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
	if (!checkCondition(C_STACK_INITIALIZED, true)) return;
	if (checkCondition(C_RADIO_INITIALIZED, false)) return;

	ret_code_t ret_code;

	LOGi(FMT_INIT, "radio");
	// Enable BLE stack
	uint32_t ram_start = g_RAM_R1_BASE;
//	uint32_t ram_start = 0;
	LOGd("nrf_sdh_ble_default_cfg_set at %p", ram_start);
	// TODO: make a separate function, that tells you what to set RAM_R1_BASE to.
	// TODO: make a unit test for that.
	ret_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	switch (ret_code) {
		case NRF_ERROR_NO_MEM:
			LOGe("Unrecoverable, memory softdevice and app overlaps. RAM_R1_BASE should be: %p", ram_start);
			break;
		case NRF_ERROR_INVALID_LENGTH:
			LOGe("RAM, invalid length");
			break;
	}
	APP_ERROR_CHECK(ret_code);
	if (ram_start != g_RAM_R1_BASE) {
		LOGw("Application address is too high, memory is unused: %p", ram_start);
	}

	LOGd("nrf_sdh_ble_enable ram_start=%p", ram_start);
	ret_code = nrf_sdh_ble_enable(&ram_start);
	switch (ret_code) {
		case NRF_ERROR_INVALID_STATE:
			LOGe("BLE: invalid radio state");
			break;
		case NRF_ERROR_INVALID_ADDR:
			LOGe("BLE: invalid memory address");
			break;
		case NRF_ERROR_NO_MEM:
			// Read out ram_start, use that as RAM_R1_BASE, and adjust RAM_APPLICATION_AMOUNT.
			LOGe("BLE: no memory available, RAM_R1_BASE should be %p", ram_start);
			break;
		case NRF_SUCCESS:
			LOGi("Softdevice enabled");
			break;
	}
	LOG_FLUSH();
	APP_ERROR_CHECK(ret_code);

	// Version is not saved or shown yet
	ble_version_t version( { });
	version.company_id = 12;
	ret_code = sd_ble_version_get(&version);
	APP_ERROR_CHECK(ret_code);

	_discoveryModule.discovery_in_progress = 0;
	_discoveryModule.discovery_pending = 0;
	_discoveryModule.conn_handle = BLE_CONN_HANDLE_INVALID;
	ret_code = ble_db_discovery_init(handle_discovery);
	APP_ERROR_CHECK(ret_code);

	setInitialized(C_RADIO_INITIALIZED);

	updateConnParams();
}

void Stack::setClockSource(nrf_clock_lf_cfg_t clockSource) {
	_clockSource = clockSource;
}

void Stack::createCharacteristics() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	LOGd("Create characteristics");

	// Init buffers.
	CharacteristicReadBuffer::getInstance().alloc(g_MASTER_BUFFER_SIZE);
	CharacteristicWriteBuffer::getInstance().alloc(g_MASTER_BUFFER_SIZE);

	for (Service* svc: _services) {
		svc->createCharacteristics();
	}
}

void Stack::initServices() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	if (checkCondition(C_SERVICES_INITIALIZED, false)) return;
	LOGd("Init services");

	for (Service* svc : _services) {
		svc->init(this);
	}

	setInitialized(C_SERVICES_INITIALIZED);
}

void Stack::shutdown() {
	// 16-sep-2019 TODO: stop advertising
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
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	ret_code_t ret_code = sd_ble_gap_ppcp_set(&_connectionParams);
	APP_ERROR_CHECK(ret_code);
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
				LOGw("Stack init %s", field ? "true" : "false");
			}
			break;
		case C_RADIO_INITIALIZED:
			field = isInitialized(C_RADIO_INITIALIZED);
			if (expectation != field) {
				LOGw("Radio init %s", field ? "true" : "false");
			}
			break;
		case C_SERVICES_INITIALIZED:
			field = isInitialized(C_SERVICES_INITIALIZED);
			if (expectation != field) {
				LOGw("Services init %s", field ? "true" : "false");
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
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_INTERVAL, &scanParams.interval, sizeof(scanParams.interval));
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_WINDOW, &scanParams.window, sizeof(scanParams.window));

	uint32_t retVal = sd_ble_gap_scan_start(&scanParams, &_scanBufferStruct);
	APP_ERROR_CHECK(retVal);
	//BLE_CALL(sd_ble_gap_scan_start, (&p_scan_params), (&scan_buffer_struct));
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

//	LOGi(FMT_STOP, "scanning");
	BLE_CALL(sd_ble_gap_scan_stop, ());
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
		LOGi("BLE event $nordicEventTypeName(%u) (0x%X)", p_ble_evt->header.evt_id, p_ble_evt->header.evt_id);
	}

	ble_db_discovery_on_ble_evt(p_ble_evt, &_discoveryModule);

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
			BLE_CALL(sd_ble_gatts_sys_attr_set, (_connectionHandle, NULL, 0,
					BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS));
			break;
		}
		case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
			onTxComplete(p_ble_evt);
			break;
		}

		// ---- GATTC events ---- //
		case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
			const ble_gattc_evt_t& gattcEvent = p_ble_evt->evt.gattc_evt;
			const ble_gattc_evt_exchange_mtu_rsp_t& mtuEvent = gattcEvent.params.exchange_mtu_rsp;
			LOGi("MTU=%u", mtuEvent.server_rx_mtu);
			_writeMtu = mtuEvent.server_rx_mtu;
			break;
		}
		case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP: {
//			ble_gattc_evt_t gattcEvent = p_ble_evt->evt.gattc_evt;
//			ble_gattc_evt_prim_srvc_disc_rsp_t response = gattcEvent.params.prim_srvc_disc_rsp;
//			LOGi("Primary services: num=%u", response.count);
//			for (uint16_t i=0; i<response.count; ++i) {
//				LOGi("uuid=0x%X", response.services[i].uuid.uuid);
//			}
			break;
		}
		case BLE_GATTC_EVT_READ_RSP: {
			const ble_gattc_evt_t& gattcEvent = p_ble_evt->evt.gattc_evt;
			const ble_gattc_evt_read_rsp_t& readResponse = gattcEvent.params.read_rsp;
			_log(SERIAL_INFO, false, "Read offset=%u len=%u", readResponse.offset, readResponse.len);
			_logArray(SERIAL_INFO, true, readResponse.data, readResponse.len);

			// Not sure how to know if we need another read (has something to do with ATT_MTU). So just keep on trying until len = 0.
//			if (readResponse.len == ATT_MTU - 1) {
			if (readResponse.len != 0) {
				// Continue long read.
				uint32_t retCode = sd_ble_gattc_read(gattcEvent.conn_handle, readResponse.handle, readResponse.offset + readResponse.len);
				if (retCode != NRF_SUCCESS) {
					LOGw("Failed to continue long read. retCode=%u", retCode);
				}
			}
			break;
		}
		case BLE_GATTC_EVT_WRITE_RSP: {
			const ble_gattc_evt_t& gattcEvent = p_ble_evt->evt.gattc_evt;
			__attribute__((unused)) const ble_gattc_evt_write_rsp_t& writeResponse = gattcEvent.params.write_rsp;
			LOGi("Write response offset=%u len=%u", writeResponse.offset, writeResponse.len);
			break;
		}
		default: {
			break;
		}
	}
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
//		BLEutil::printArray(scan.data, scan.dataSize);
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

				uint32_t retVal = app_sched_event_put(&scan, sizeof(scan), csStackOnScan);
				APP_ERROR_CHECK(retVal);
			}
			else {
				// Handle scan immediately, since we're already on thread level.
				csStackOnScan(&(p_ble_evt->evt.gap_evt.params.adv_report));
			}

			// If ble_gap_adv_report_type_t::status is set to BLE_GAP_ADV_DATA_STATUS_INCOMPLETE_MORE_DATA,
			//      not all fields in the advertising report may be available.
			// Else, scanning will be paused. To continue scanning, call sd_ble_gap_scan_start.

			// Resume scanning: ignore _scanning state as this is executed in an interrupt. Rely on return value instead.
			uint32_t retVal = sd_ble_gap_scan_start(NULL, &_scanBufferStruct);
			switch (retVal) {
				case NRF_ERROR_INVALID_STATE:
					break;
				default:
					APP_ERROR_CHECK(retVal);
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
	BLE_CALL(sd_ble_user_mem_reply, (connectionHandle, &memBlock));
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
			sd_ble_gap_rssi_start(_connectionHandle, 0, 0);
		}

		switch (p_ble_evt->evt.gap_evt.params.connected.role) {
			case BLE_GAP_ROLE_PERIPH: {
				LOGi("onConnect as peripheral");
				break;
			}
			case BLE_GAP_ROLE_CENTRAL: {
				LOGi("onConnect as central");
				assert(_connectionIsOutgoing == true, "Central role whith no outgoing connection?");
				break;
			}
			default: {
				LOGw("onConnect with unknown role");
				break;
			}
		}

		if (_connectionIsOutgoing) {
			onOutgoingConnected();
		}
		else {
			onIncomingConnected(p_ble_evt);
		}
}

void Stack::onConnectionTimeout() {
	LOGd("onConnectionTimeout");
	if (_connectionIsOutgoing) {
		event_t event(CS_TYPE::EVT_OUTGOING_DISCONNECTED);
		event.dispatch();
		_connectionIsOutgoing = false;
	}
	else {
		LOGw("No outgoing connection");
	}
}

void Stack::onDisconnect(const ble_evt_t * p_ble_evt) {
	_connectionHandle = BLE_CONN_HANDLE_INVALID;

	if (_connectionIsOutgoing) {
		onOutgoingDisconnected();
	}
	else {
		onIncomingDisconnected(p_ble_evt);
	}
	_connectionIsOutgoing = false;
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

	event_t event(CS_TYPE::EVT_BLE_CONNECT);
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

void Stack::onOutgoingConnected() {
	LOGi("Connected to device");

	event_t event(CS_TYPE::EVT_OUTGOING_CONNECTED);
	event.dispatch();

	LOGi("Start discovery");
	uint32_t retCode;

	// We have to tell the discovery module what services we're interested in _before_ the discovery.
	ble_uuid_t serviceUuid;
	UUID uuid;
	cs_ret_code_t csRetCode;

	// Setup service.
	csRetCode = uuid.fromFullUuid(SETUP_UUID);
	serviceUuid = uuid.getUuid();
	LOGi("setup service uuid=0x%X type=%u", serviceUuid.uuid, serviceUuid.type);
	retCode = ble_db_discovery_evt_register(&serviceUuid);

	// Crownstone service.
	csRetCode = uuid.fromFullUuid(CROWNSTONE_UUID);
	if (csRetCode != ERR_SUCCESS) {
		LOGe("Failed to get UUID err=%u", csRetCode);
	}
	serviceUuid = uuid.getUuid();
	LOGi("crownstone service uuid=0x%X type=%u", serviceUuid.uuid, serviceUuid.type);
	retCode = ble_db_discovery_evt_register(&serviceUuid);

	// Secure DFU service
	csRetCode = uuid.fromShortUuid(0xFE59);
	serviceUuid = uuid.getUuid();
	retCode = ble_db_discovery_evt_register(&serviceUuid);

	// Device information service.
	csRetCode = uuid.fromShortUuid(0x180A);
	serviceUuid = uuid.getUuid();
	retCode = ble_db_discovery_evt_register(&serviceUuid);

	retCode = ble_db_discovery_start(&_discoveryModule, _connectionHandle);
	if (retCode != NRF_SUCCESS) {
		LOGe("Failed to start discovery retCode=%u", retCode);
		disconnect();
	}
}

void Stack::onOutgoingDisconnected() {
	LOGi("Disconnected from device");

	event_t event(CS_TYPE::EVT_OUTGOING_DISCONNECTED);
	event.dispatch();
}

void Stack::onDiscoveryEvent(ble_db_discovery_evt_t* event) {
	uint32_t retCode;
	switch (event->evt_type) {
		case BLE_DB_DISCOVERY_COMPLETE: {
			LOGi("Discovery found uuid=0x%04X type=%u characteristicCount=%u", event->params.discovered_db.srv_uuid.uuid, event->params.discovered_db.srv_uuid.type, event->params.discovered_db.char_count);
			if (event->params.discovered_db.srv_uuid.type >= BLE_UUID_TYPE_VENDOR_BEGIN) {
				ble_uuid128_t fullUuid;
				uint8_t uuidSize = 0;
				retCode = sd_ble_uuid_encode(&(event->params.discovered_db.srv_uuid), &uuidSize, fullUuid.uuid128);
				if (retCode == NRF_SUCCESS && uuidSize == sizeof(fullUuid)) {
#if CS_SERIAL_NRF_LOG_ENABLED == 0
					LOGd("Full uuid: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
							fullUuid.uuid128[15],
							fullUuid.uuid128[14],
							fullUuid.uuid128[13],
							fullUuid.uuid128[12],
							fullUuid.uuid128[11],
							fullUuid.uuid128[10],
							fullUuid.uuid128[9],
							fullUuid.uuid128[8],
							fullUuid.uuid128[7],
							fullUuid.uuid128[6],
							fullUuid.uuid128[5],
							fullUuid.uuid128[4],
							fullUuid.uuid128[3],
							fullUuid.uuid128[2],
							fullUuid.uuid128[1],
							fullUuid.uuid128[0]);
#endif
				}
				else {
					LOGw("Failed to get full uuid");
				}
			}
			break;
		}
		case BLE_DB_DISCOVERY_SRV_NOT_FOUND: {
			LOGi("Discovery not found uuid=0x%04X type=%u", event->params.discovered_db.srv_uuid.uuid, event->params.discovered_db.srv_uuid.type);
			break;
		}
		case BLE_DB_DISCOVERY_ERROR: {
			LOGw("Discovery error %u", event->params.err_code);
			break;
		}
		case BLE_DB_DISCOVERY_AVAILABLE: {
			// A bug prevents this event from ever firing. It is fixed in SDK 16.0.0.
			// Instead, we should be done when the number of registered uuids equals the number of received BLE_DB_DISCOVERY_COMPLETE + BLE_DB_DISCOVERY_SRV_NOT_FOUND events.
			// See https://devzone.nordicsemi.com/f/nordic-q-a/20846/getting-service-count-from-database-discovery-module
			// We apply a similar patch to the SDK.
			LOGi("Discovery done");

			// According to the doc, the "services" struct is only for internal usage.
			// But it seems to store all the discovered services and characteristics.
			uint16_t fwVersionHandle = BLE_GATT_HANDLE_INVALID;
			uint16_t controlHandle = BLE_GATT_HANDLE_INVALID;

			for (uint8_t s = 0; s < _discoveryModule.discoveries_count; ++s) {
				LOGi("service: uuidType=%u uuid=0x%04X", _discoveryModule.services[s].srv_uuid.type, _discoveryModule.services[s].srv_uuid.uuid);
				for (uint8_t c = 0; c < _discoveryModule.services[s].char_count; ++c) {
					ble_gatt_db_char_t* characteristic = &(_discoveryModule.services[s].charateristics[c]);
					LOGi("    char: cccd_handle=0x%X uuidType=%u uuid=0x%04X handle_value=0x%X handle_decl=0x%X",
							characteristic->cccd_handle,
							characteristic->characteristic.uuid.type,
							characteristic->characteristic.uuid.uuid,
							characteristic->characteristic.handle_value,
							characteristic->characteristic.handle_decl);
					if (characteristic->characteristic.uuid.type == BLE_UUID_TYPE_BLE && characteristic->characteristic.uuid.uuid == 0x2A26) {
						fwVersionHandle = characteristic->characteristic.handle_value;
					}
					if (characteristic->characteristic.uuid.type >= BLE_UUID_TYPE_VENDOR_BEGIN && characteristic->characteristic.uuid.uuid == 0x000E) {
						controlHandle = characteristic->characteristic.handle_value;
					}
				}
			}

			if (fwVersionHandle != BLE_GATT_HANDLE_INVALID) {
//				LOGi("Read fw version");
//				// Triggers event BLE_GATTC_EVT_READ_RSP.
//				retCode = sd_ble_gattc_read(_connectionHandle, fwVersionHandle, 0);
//				if (retCode != NRF_SUCCESS) {
//					LOGw("Failed to read fw version retCode=%u", retCode);
//				}
			}

			if (controlHandle != BLE_GATT_HANDLE_INVALID) {
//				for (uint16_t i = 0; i < sizeof(_writeBuf); ++i) {
//					_writeBuf[i] = i;
//				}
//
//				ble_gattc_write_params_t writeParams;
//				writeParams.write_op = BLE_GATT_OP_WRITE_REQ; // Write with response (ack). Triggers BLE_GATTC_EVT_WRITE_RSP.
//				writeParams.flags = 0; // Ignored for write_req
//				writeParams.handle = controlHandle;
//				writeParams.offset = 0;
//				writeParams.len = sizeof(_writeBuf);
//				writeParams.p_value = _writeBuf;
//
//				// We can't write more than MTU.
//				// For long writes, use BLE_GATT_OP_PREP_WRITE_REQ followed by BLE_GATT_OP_EXEC_WRITE_REQ.
//				if (writeParams.len > _writeMtu) {
//					writeParams.len = _writeMtu;
//				}
//
//				retCode = sd_ble_gattc_write(_connectionHandle, &writeParams);
//				if (retCode != NRF_SUCCESS) {
//					LOGw("Failed to write. retCode=%u", retCode);
//				}
			}
			break;
		}
	}
	LOGd("Discovery progress=%u pending=%u numServicesHandled=%u numServices=%u",
			_discoveryModule.discovery_in_progress,
			_discoveryModule.discovery_pending,
			_discoveryModule.discoveries_count,
			_discoveryModule.srv_count);
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
		uint32_t errorCode = sd_ble_gap_disconnect(_connectionHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		switch (errorCode) {
		case BLE_ERROR_INVALID_CONN_HANDLE:
		case NRF_ERROR_INVALID_STATE:
			break;
		default:
			APP_ERROR_CHECK(errorCode);
			break;
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


cs_ret_code_t Stack::connect(const device_address_t& address, uint16_t timeoutMs) {
	if (isConnected()) {
		LOGi("Already connected");
		return ERR_BUSY;
	}

	LOGw("Not ready for release");
	return ERR_NOT_IMPLEMENTED;

	event_t connectEvent(CS_TYPE::EVT_OUTGOING_CONNECT_START);
	connectEvent.dispatch();

	if (connectEvent.result.returnCode != ERR_SUCCESS) {
		LOGe("Unable to start connecting: err=%u", connectEvent.result.returnCode);
		event_t disconnectEvent(CS_TYPE::EVT_OUTGOING_DISCONNECTED);
		disconnectEvent.dispatch();
		return connectEvent.result.returnCode;
	}

	ble_gap_addr_t gapAddress;
	memcpy(gapAddress.addr, address.address, sizeof(address.address));
	gapAddress.addr_id_peer = 0;
	gapAddress.addr_type = address.addressType;

	ble_gap_scan_params_t scanParams;
	scanParams.extended = 0;
	scanParams.report_incomplete_evts = 0;
	scanParams.active = 1;
	scanParams.filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL; // Scanning filter policy. See BLE_GAP_SCAN_FILTER_POLICIES
	scanParams.scan_phys = BLE_GAP_PHY_1MBPS;
	scanParams.timeout = timeoutMs / 10; // This acts as connection timeout.
	scanParams.channel_mask[0] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[1] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[2] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[3] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[4] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.interval = 160;
	scanParams.window = 80;

	if (scanParams.timeout < BLE_GAP_SCAN_TIMEOUT_MIN) {
		scanParams.timeout = BLE_GAP_SCAN_TIMEOUT_MIN;
	}

	uint32_t errCode = sd_ble_gap_connect(&gapAddress, &scanParams, &_connectionParams, APP_BLE_CONN_CFG_TAG);
	if (errCode == NRF_SUCCESS) {
		LOGi("Connecting..");
		_connectionIsOutgoing = true;
		// We will get either BLE_GAP_EVT_CONNECTED or BLE_GAP_EVT_TIMEOUT.
		return ERR_SUCCESS;
	}
	else {
		LOGe("Connect err=%u", errCode);
		return ERR_UNSPECIFIED;
	}
}

