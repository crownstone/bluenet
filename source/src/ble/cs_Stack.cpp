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


Stack::Stack() :
//	_clock_source(defaultClockSource),
	_gap_conn_params({ }),
	_scanning(false),
	_conn_handle(BLE_CONN_HANDLE_INVALID),
	_radio_notify(0),
	_connectionKeepAliveTimerId(NULL)
{

	_connectionKeepAliveTimerData = { {0} };
	_connectionKeepAliveTimerId = &_connectionKeepAliveTimerData;

	_gap_conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
	_gap_conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
	_gap_conn_params.slave_latency = SLAVE_LATENCY;
	_gap_conn_params.conn_sup_timeout = CONNECTION_SUPERVISION_TIMEOUT;

	// default stack state (will be used in halt/resume)
	_stack_state.advertising = false;
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
	switch(ret_code) {
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
	switch(ret_code) {
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

	setInitialized(C_RADIO_INITIALIZED);

	updateConnParams();
}

void Stack::halt() {
//	_stack_state.advertising = _advertising;
//	if (_advertising) {
//		stopAdvertising();
//	}
}

void Stack::resume() {
//	if (_stack_state.advertising) {
//		_stack_state.advertising = false;
//		startAdvertising();
//	}
}

void Stack::setClockSource(nrf_clock_lf_cfg_t clockSource) {
	_clock_source = clockSource;
}

void Stack::createCharacteristics() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	LOGd("Create characteristics");

	// Init buffers.
	CharacteristicReadBuffer::getInstance().alloc(MASTER_BUFFER_SIZE);
	CharacteristicWriteBuffer::getInstance().alloc(MASTER_BUFFER_SIZE);

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
	if (_gap_conn_params.min_conn_interval != connectionInterval_1_25_ms) {
		_gap_conn_params.min_conn_interval = connectionInterval_1_25_ms;
		updateConnParams();
	}
}

void Stack::updateMaxConnectionInterval(uint16_t connectionInterval_1_25_ms) {
	if (_gap_conn_params.max_conn_interval != connectionInterval_1_25_ms) {
		_gap_conn_params.max_conn_interval = connectionInterval_1_25_ms;
		updateConnParams();
	}
}

void Stack::updateSlaveLatency(uint16_t slaveLatency) {
	if ( _gap_conn_params.slave_latency != slaveLatency ) {
		_gap_conn_params.slave_latency = slaveLatency;
		updateConnParams();
	}
}

void Stack::updateConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms) {
	if (_gap_conn_params.conn_sup_timeout != conSupTimeout_10_ms) {
		_gap_conn_params.conn_sup_timeout = conSupTimeout_10_ms;
		updateConnParams();
	}
}

void Stack::updateConnParams() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	ret_code_t ret_code = sd_ble_gap_ppcp_set(&_gap_conn_params);
	APP_ERROR_CHECK(ret_code);
}



/** Utility function which logs unexpected state
 *
 */
bool Stack::checkCondition(condition_t condition, bool expectation) {
	bool field = false;
	switch(condition) {
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
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	if (_scanning) {
		return;
	}

//	LOGi(FMT_START, "scanning");
	ble_gap_scan_params_t p_scan_params;
	p_scan_params.extended = 0;
	p_scan_params.report_incomplete_evts = 0;
	p_scan_params.active = 1;
	p_scan_params.filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL; // Scanning filter policy. See BLE_GAP_SCAN_FILTER_POLICIES
	p_scan_params.scan_phys = BLE_GAP_PHY_1MBPS;
	p_scan_params.timeout = BLE_GAP_SCAN_TIMEOUT_UNLIMITED; // Scan timeout in 10 ms units. See BLE_GAP_SCAN_TIMEOUT.
	p_scan_params.channel_mask[0] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	p_scan_params.channel_mask[1] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	p_scan_params.channel_mask[2] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	p_scan_params.channel_mask[3] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	p_scan_params.channel_mask[4] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_INTERVAL, &p_scan_params.interval, sizeof(p_scan_params.interval));
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_WINDOW, &p_scan_params.window, sizeof(p_scan_params.window));

	uint32_t retVal = sd_ble_gap_scan_start(&p_scan_params, &_scanBufferStruct);
	APP_ERROR_CHECK(retVal);
	//BLE_CALL(sd_ble_gap_scan_start, (&p_scan_params), (&scan_buffer_struct));
	_scanning = true;

	event_t event(CS_TYPE::EVT_SCAN_STARTED, NULL, 0);
	EventDispatcher::getInstance().dispatch(event);
}


void Stack::stopScanning() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	if (!_scanning)
		return;

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

	if (p_ble_evt->header.evt_id !=  BLE_GAP_EVT_RSSI_CHANGED && p_ble_evt->header.evt_id != BLE_GAP_EVT_ADV_REPORT) {
		LOGd("BLE event $nordicEventTypeName(%u) (0x%X)", p_ble_evt->header.evt_id, p_ble_evt->header.evt_id);
	}

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED: {
		onConnected(p_ble_evt);
		event_t event(CS_TYPE::EVT_BLE_CONNECT, NULL, 0);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case BLE_EVT_USER_MEM_REQUEST: {
		// You can check which type is requested: p_ble_evt->evt.common_evt.params.user_mem_request.type
		// Currently only option is: BLE_USER_MEM_TYPE_GATTS_QUEUED_WRITES
		// See https://devzone.nordicsemi.com/f/nordic-q-a/33366/is-it-necessary-handle-ble_evt_user_mem_request-respectivly-is-it-required-to-support-prepared-writes
		// And https://devzone.nordicsemi.com/f/nordic-q-a/53074/ble_evt_user_mem_request-if-data_length-is-180-bytes-on-ios-but-not-for-android
		// And https://devzone.nordicsemi.com/f/nordic-q-a/50043/ble_evt_user_mem_request-patterns
		// Also see https://interrupt.memfault.com/blog/ble-throughput-primer
//		BLE_CALL(sd_ble_user_mem_reply, (p_ble_evt->evt.gap_evt.conn_handle, NULL));

		ble_user_mem_block_t memBlock;
		cs_data_t writeBuffer = CharacteristicWriteBuffer::getInstance().getBuffer(CS_CHAR_BUFFER_DEFAULT_OFFSET - CS_STACK_LONG_WRITE_HEADER_SIZE);
		memBlock.p_mem = writeBuffer.data;
		memBlock.len = writeBuffer.len;
		BLE_CALL(sd_ble_user_mem_reply, (p_ble_evt->evt.gap_evt.conn_handle, &memBlock));
		break;
	}
	case BLE_EVT_USER_MEM_RELEASE:
		// nothing to do
		break;
	case BLE_GAP_EVT_DISCONNECTED: {
		onDisconnected(p_ble_evt);
		event_t event(CS_TYPE::EVT_BLE_DISCONNECT, NULL, 0);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
	case BLE_GATTS_EVT_TIMEOUT:
	case BLE_GAP_EVT_RSSI_CHANGED:
		for (Service* svc : _services) {
			svc->on_ble_event(p_ble_evt);
		}
		break;
	case BLE_GATTS_EVT_WRITE: {
		resetConnectionAliveTimer();
		if (p_ble_evt->evt.gatts_evt.params.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {
			cs_data_t writeBuffer = CharacteristicWriteBuffer::getInstance().getBuffer(CS_CHAR_BUFFER_DEFAULT_OFFSET - CS_STACK_LONG_WRITE_HEADER_SIZE);
			uint16_t* header = (uint16_t*)(writeBuffer.data);
			for (Service* svc : _services) {
				// for a long write, don't have the service handle available to check for the correct
				// service, so we just go through all the services and characteristics until we find
				// the correct characteristic, then we return
				if (svc->on_write(p_ble_evt->evt.gatts_evt.params.write, header[0])) {
					return;
				}
			}
		}
		else {
			for (Service* svc : _services) {
				svc->on_write(p_ble_evt->evt.gatts_evt.params.write, p_ble_evt->evt.gatts_evt.params.write.handle);
			}
		}
		break;
	}

	case BLE_GATTS_EVT_HVC:
		for (Service* svc : _services) {
			// TODO: this check is probably be wrong
			if (svc->getHandle() == p_ble_evt->evt.gatts_evt.params.write.handle) {
				svc->on_ble_event(p_ble_evt);
				return;
			}
		}
		break;

	case BLE_GATTS_EVT_SYS_ATTR_MISSING:
		BLE_CALL(sd_ble_gatts_sys_attr_set, (_conn_handle, NULL, 0,
                BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS));
		break;

	case BLE_GAP_EVT_PASSKEY_DISPLAY: {
#ifdef PRINT_STACK_VERBOSE
		LOGd("PASSKEY: %.6s", p_ble_evt->evt.gap_evt.params.passkey_display.passkey);
#endif
		break;
	}

/*
	case BLE_GAP_EVT_ADV_REPORT: {
		const ble_gap_evt_adv_report_t* advReport = &(p_ble_evt->evt.gap_evt.params.adv_report);
		scanned_device_t scan;
		memcpy(scan.address, advReport->peer_addr.addr, sizeof(scan.address)); // TODO: Only valid when advReport->peer_addr.addr_id_peer == 1
//		uint8_t* scanData = new uint8_t[advReport->data.len];
//		memcpy(scanData, advReport->data.p_data, advReport->data.len);
		scan.rssi = advReport->rssi;
		scan.channel = advReport->ch_index;
		scan.dataSize = advReport->data.len;
		scan.data = advReport->data.p_data;
		uint16_t type = *((uint16_t*)&(advReport->type));

		const uint8_t* addr = scan.address;
		const uint8_t* p = scan.data;
		if (p[0] == 0x15 && p[1] == 0x16 && p[2] == 0x01 && p[3] == 0xC0 && p[4] == 0x05) {
//		if (p[1] == 0xFF && p[2] == 0xCD && p[3] == 0xAB) {
//		if (advReport->peer_addr.addr_type == BLE_GAP_ADDR_TYPE_PUBLIC && addr[5] == 0xE7 && addr[4] == 0x09 && addr[3] == 0x62) { // E7:09:62:02:91:3D
//		if (addr[5] == 0xE7 && addr[4] == 0x09 && addr[3] == 0x62) { // E7:09:62:02:91:3D
			LOGi("Stack scan: address=%02X:%02X:%02X:%02X:%02X:%02X addrType=%u type=%u rssi=%i chan=%u", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], advReport->peer_addr.addr_type, type, scan.rssi, scan.channel);
			LOGd("  adv_type=%u len=%u data=", type, scan.dataSize);
			BLEutil::printArray(scan.data, scan.dataSize);
		}
		event_t event(CS_TYPE::EVT_DEVICE_SCANNED, (void*)&scan, sizeof(scan));
		EventDispatcher::getInstance().dispatch(event);
//		free(scanData);
		break;
	}
*/
	case BLE_GAP_EVT_TIMEOUT:
		LOGw("Timeout!");
		// BLE_GAP_TIMEOUT_SRC_ADVERTISING does not exist anymore...
//		if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISING) {
//			_advertising = false;
//			// Advertising stops, see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
//		}
		break;

	case BLE_GATTS_EVT_HVN_TX_COMPLETE:
		onTxComplete(p_ble_evt);
		break;

	default:
		break;

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
	scan.addressType = (advReport->peer_addr.addr_type & 0x7F) & ((advReport->peer_addr.addr_id_peer & 0x01) << 7);
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


#if CONNECTION_ALIVE_TIMEOUT>0
static void connection_keep_alive_timeout(void* p_context) {
	LOGw("connection keep alive timeout!");
	Stack::getInstance().disconnect();
}
#endif

void Stack::startConnectionAliveTimer() {
#if CONNECTION_ALIVE_TIMEOUT>0
	Timer::getInstance().createSingleShot(_connectionKeepAliveTimerId, connection_keep_alive_timeout);
	Timer::getInstance().start(_connectionKeepAliveTimerId, MS_TO_TICKS(CONNECTION_ALIVE_TIMEOUT), NULL);
#endif
}

void Stack::stopConnectionAliveTimer() {
#if CONNECTION_ALIVE_TIMEOUT>0
	Timer::getInstance().stop(_connectionKeepAliveTimerId);
#endif
}

void Stack::resetConnectionAliveTimer() {
#if CONNECTION_ALIVE_TIMEOUT>0
	Timer::getInstance().reset(_connectionKeepAliveTimerId, MS_TO_TICKS(CONNECTION_ALIVE_TIMEOUT), NULL);
#endif
}

void Stack::onConnected(const ble_evt_t * p_ble_evt) {
	LOGd("Connection event");
	//ble_gap_evt_connected_t connected_evt = p_ble_evt->evt.gap_evt.params.connected;

	// 12-sep-2019 TODO: why do we do this? In the peripheral examples this is not done.
//	BLE_CALL(sd_ble_gap_conn_param_update, (p_ble_evt->evt.gap_evt.conn_handle, &_gap_conn_params));

//	ble_gap_addr_t* peerAddr = &p_ble_evt->evt.gap_evt.params.connected.peer_addr;
//	LOGi("Connection from: %02X:%02X:%02X:%02X:%02X:%02X", peerAddr->addr[5],
//	                       peerAddr->addr[4], peerAddr->addr[3], peerAddr->addr[2], peerAddr->addr[1],
//	                       peerAddr->addr[0]);

	_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
	_disconnectingInProgress = false;

	if (_callback_connected) {
		_callback_connected(p_ble_evt->evt.gap_evt.conn_handle);
	}
	for (Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}
	startConnectionAliveTimer();
}

void Stack::onDisconnected(const ble_evt_t * p_ble_evt) {
	//ble_gap_evt_disconnected_t disconnected_evt = p_ble_evt->evt.gap_evt.params.disconnected;
	_conn_handle = BLE_CONN_HANDLE_INVALID;
	if (_callback_connected) {
		_callback_disconnected(p_ble_evt->evt.gap_evt.conn_handle);
	}
	for (Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}
	stopConnectionAliveTimer();
}

void Stack::disconnect() {
	// Only disconnect when we are actually connected to something
	if (_conn_handle != BLE_CONN_HANDLE_INVALID && _disconnectingInProgress == false) {
		_disconnectingInProgress = true;
		LOGi("Forcibly disconnecting from device");
		// It seems like we're only allowed to use BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION.
		// This sometimes gives us an NRF_ERROR_INVALID_STATE (disconnection is already in progress)
		// NRF_ERROR_INVALID_STATE can safely be ignored, see: https://devzone.nordicsemi.com/question/81108/handling-nrf_error_invalid_state-error-code/
		// BLE_ERROR_INVALID_CONN_HANDLE can safely be ignored, see: https://devzone.nordicsemi.com/f/nordic-q-a/34353/error-0x3002/132078#132078
		uint32_t errorCode = sd_ble_gap_disconnect(_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
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
	return _conn_handle != BLE_CONN_HANDLE_INVALID;
}

void Stack::onTxComplete(const ble_evt_t * p_ble_evt) {
	for (Service* svc: _services) {
		svc->onTxComplete(&p_ble_evt->evt.common_evt);
	}
}

void Stack::setOnConnectCallback(const callback_connected_t& callback) {
	_callback_connected = callback;
}

void Stack::setOnDisconnectCallback(const callback_disconnected_t& callback) {
	_callback_disconnected = callback;
}

