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
#include <cfg/cs_UuidConfig.h>
#include <common/cs_Handlers.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_Scanner.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include <util/cs_Utils.h>


//#define PRINT_STACK_VERBOSE

// Define test pin to enable gpio debug.
//#define TEST_PIN 19

Stack::Stack() :
	_appearance(BLE_APPEARANCE_GENERIC_TAG), //_clock_source(defaultClockSource),
	_tx_power_level(TX_POWER), _sec_mode({ }),
	_interval(defaultAdvertisingInterval_0_625_ms), _timeout(defaultAdvertisingTimeout_seconds),
	_gap_conn_params( { }),
	_advertising(false),
	_advertisingConfigured(false),
	_scanning(false),
	_conn_handle(BLE_CONN_HANDLE_INVALID),
	_radio_notify(0),
	_dm_initialized(false),
	_lowPowerTimeoutId(NULL),
	_secReqTimerId(NULL),
	_connectionKeepAliveTimerId(NULL),
	_adv_handle(BLE_GAP_ADV_SET_HANDLE_NOT_SET),
	_advInterleaveCounter(0),
	_adv_manuf_data(NULL),
	_serviceData(NULL)
{

	_lowPowerTimeoutData = { {0} };
	_lowPowerTimeoutId = &_lowPowerTimeoutData;
	_secReqTimerData = { {0} };
	_secReqTimerId = &_secReqTimerData;
	_connectionKeepAliveTimerData = { {0} };
	_connectionKeepAliveTimerId = &_connectionKeepAliveTimerData;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&_sec_mode);

	_gap_conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
	_gap_conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
	_gap_conn_params.slave_latency = SLAVE_LATENCY;
	_gap_conn_params.conn_sup_timeout = CONNECTION_SUPERVISION_TIMEOUT;

	// default stack state (will be used in halt/resume)
	_stack_state.advertising = false;
}

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

	_adv_data.adv_data.p_data = NULL;
	_adv_data.adv_data.len = 0;

	_adv_data.scan_rsp_data.p_data = NULL;
	_adv_data.scan_rsp_data.len = 0;

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
	uint32_t ram_start = RAM_R1_BASE;
//	uint32_t ram_start = 0;
	_conn_cfg_tag = 1;
	LOGd("nrf_sdh_ble_default_cfg_set at %p", ram_start);
	// TODO: make a seperate function, that tells you what to set RAM_R1_BASE to.
	// TODO: make a unit test for that.
	ret_code = nrf_sdh_ble_default_cfg_set(_conn_cfg_tag, &ram_start);
	switch(ret_code) {
		case NRF_ERROR_NO_MEM:
			LOGe("Unrecoverable, memory softdevice and app overlaps. RAM_R1_BASE should be: %p", ram_start);
			break;
		case NRF_ERROR_INVALID_LENGTH:
			LOGe("RAM, invalid length");
			break;
	}
	APP_ERROR_CHECK(ret_code);
	if (ram_start != RAM_R1_BASE) {
		LOGw("Application address is too high, memory is unused: %p", ram_start);
	}

	LOGd("nrf_sdh_ble_enable");
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
	NRF_LOG_FLUSH();
	APP_ERROR_CHECK(ret_code);


	// Version is not saved or shown yet
	ble_version_t version( { });
	version.company_id = 12;
	ret_code = sd_ble_version_get(&version);
	APP_ERROR_CHECK(ret_code);

	std::string device_name = _device_name.empty() ? "not set..." : _device_name;

	LOGv("sd_ble_gap_device_name_set");
	ret_code = sd_ble_gap_device_name_set(&_sec_mode, (uint8_t*) device_name.c_str(), device_name.length());
	APP_ERROR_CHECK(ret_code);

	ret_code = sd_ble_gap_appearance_set(_appearance);
	APP_ERROR_CHECK(ret_code);

	// BLE address with which we will broadcast, store in object field
	ret_code = sd_ble_gap_addr_get(&_connectable_address);
	APP_ERROR_CHECK(ret_code);
	ret_code = sd_ble_gap_addr_get(&_nonconnectable_address);
	APP_ERROR_CHECK(ret_code);
	// have non-connectable address one value higher than connectable one
	_nonconnectable_address.addr[0] += 0x1;

	setInitialized(C_RADIO_INITIALIZED);

	updateConnParams();
	LOGw("Update TX Power not possible yet");
	//updateTxPowerLevel();
}

void Stack::halt() {
	_stack_state.advertising = _advertising;
	if (_advertising) {
		stopAdvertising();
	}
}

void Stack::resume() {
	if (_stack_state.advertising) {
		_stack_state.advertising = false;
		startAdvertising();
	}
}

void Stack::setAppearance(uint16_t appearance) {
	_appearance = appearance;
}

void Stack::setDeviceName(const std::string& deviceName) {
	_device_name = deviceName;
}

void Stack::setClockSource(nrf_clock_lf_cfg_t clockSource) {
	_clock_source = clockSource;
}

void Stack::setAdvertisingInterval(uint16_t advertisingInterval) {
	if (advertisingInterval < 0x0020 || advertisingInterval > 0x4000) {
		LOGw("Invalid advertising interval");
		return;
	}
	_interval = advertisingInterval;
}

void Stack::updateAdvertisingInterval(uint16_t advertisingInterval, bool apply) {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	LOGd("Update advertising interval");

	setAdvertisingInterval(advertisingInterval);
	configureAdvertisementParameters();

	// Apply new interval.
	if (apply && _advertising) {
		stopAdvertising();
		startAdvertising();
	}
}

/**
 * Programming by side effect! Other functions assume _device_name to be set even if sd_ble_gap_device_name_set has
 * not been called yet.
 */
void Stack::updateDeviceName(const std::string& deviceName) {
	_device_name = deviceName;

	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	LOGd("Set device name to %s", _device_name.c_str());

	std::string name = _device_name.empty() ? "not set..." : deviceName;

	uint32_t ret_code;
	ret_code = sd_ble_gap_device_name_set(&_sec_mode, (uint8_t*) name.c_str(),
			name.length());
	APP_ERROR_CHECK(ret_code);
}

void Stack::updateAppearance(uint16_t appearance) {
	_appearance = appearance;
	BLE_CALL(sd_ble_gap_appearance_set, (_appearance));
}

void Stack::createCharacteristics() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	LOGd("Create characteristics");
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
	stopAdvertising();

	for (Service* svc : _services) {
		svc->stopAdvertising();
	}

	setUninitialized(C_STACK_INITIALIZED);
}

Stack& Stack::addService(Service* svc) {
	_services.push_back(svc);
	return *this;
}

/**
 * The accepted values are -40, -30, -20, -16, -12, -8, -4, 0, and 4 dBm.
 * The -30 is not accepted on the nRF52.
 *
 * This function can be called at any moment (also when advertising).
 */
void Stack::setTxPowerLevel(int8_t powerLevel) {
#ifdef PRINT_STACK_VERBOSE
	LOGd(FMT_SET_INT_VAL, "TX power", powerLevel);
#endif

	switch (powerLevel) {
		case -40: case -20: case -16: case -12: case -8: case -4: case 0: case 4:
			// accepted values
			break;
		default:
			// other values are not accepted
			return;
	}
	if (_tx_power_level != powerLevel) {
		_tx_power_level = powerLevel;
		updateTxPowerLevel();
	}
}

/**
 * It seems that if we have a connectable advertisement and a subsequent connection, we cannot update the TX
 * power while already being connected. At least it returns a BLE_ERROR_INVALID_ADV_HANDLE.
 */
void Stack::updateTxPowerLevel() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	if (_adv_handle == BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		LOGw("Invalid handle");
		return;
	}
	uint32_t ret_code;
	LOGd("Update tx power level %i for handle %i", _tx_power_level, _adv_handle);

	ret_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, _adv_handle, _tx_power_level);
	APP_ERROR_CHECK(ret_code);
}

void Stack::setMinConnectionInterval(uint16_t connectionInterval_1_25_ms) {
	if (_gap_conn_params.min_conn_interval != connectionInterval_1_25_ms) {
		_gap_conn_params.min_conn_interval = connectionInterval_1_25_ms;
		updateConnParams();
	}
}

void Stack::setMaxConnectionInterval(uint16_t connectionInterval_1_25_ms) {
	if (_gap_conn_params.max_conn_interval != connectionInterval_1_25_ms) {
		_gap_conn_params.max_conn_interval = connectionInterval_1_25_ms;
		updateConnParams();
	}
}

void Stack::setSlaveLatency(uint16_t slaveLatency) {
	if ( _gap_conn_params.slave_latency != slaveLatency ) {
		_gap_conn_params.slave_latency = slaveLatency;
		updateConnParams();
	}
}

void Stack::setConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms) {
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

/** Configure advertisement data according to iBeacon specification.
 *
 * The iBeacon payload has 31 bytes:
 *   4 bytes to define lengths of major, minor, uuid and rssi
 *   3 bytes for major (1 byte for type,  2 bytes for data)
 *   3 bytes for minor (1 byte for type,  2 bytes for data)
 *  17 bytes for uuid  (1 byte for type, 16 bytes for data)
 *   2 bytes for rssi  (1 byte for type,  1 byte  for data)
 *   2 bytes undefined
 * The company identifier has to be set to 0x004C or it will not be recognized as an iBeacon by iPhones.
 */
void Stack::configureIBeaconAdvData(IBeacon* beacon) {
	LOGd("Configure iBeacon adv data");

	memset(&_manufac_apple, 0, sizeof(_manufac_apple));
	_manufac_apple.company_identifier = 0x004C;
	_manufac_apple.data.p_data = beacon->getArray();
	_manufac_apple.data.size = beacon->size();

	memset(&_config_advdata, 0, sizeof(_config_advdata));

	_config_advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	_config_advdata.p_manuf_specific_data = &_manufac_apple;
	_config_advdata.name_type = BLE_ADVDATA_NO_NAME;
	_config_advdata.include_appearance = false;

	ret_code_t ret_code;
	_adv_data.adv_data.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
	if (_adv_data.adv_data.p_data == NULL) {
		_adv_data.adv_data.p_data = (uint8_t*)calloc(sizeof(uint8_t), _adv_data.adv_data.len);
	}
	LOGd("Create advertisement of size %i", _adv_data.adv_data.len);
	ret_code = ble_advdata_encode(&_config_advdata, _adv_data.adv_data.p_data, &_adv_data.adv_data.len);
	LOGv("First bytes in encoded buffer: [%02x %02x %02x %02x]",
			_adv_data.adv_data.p_data[0], _adv_data.adv_data.p_data[1],
			_adv_data.adv_data.p_data[2], _adv_data.adv_data.p_data[3]
	    );
	LOGv("Got encoded buffer of size: %i", _adv_data.adv_data.len);
	APP_ERROR_CHECK(ret_code);
}

/** Configurate advertisement data according to Crowstone specification.
 *
 * The Crownstone advertisement payload has 31 bytes:
 *   1 byte  to define length of the data
 *   2 bytes for flags    (1 byte for type,  1 byte  for data)
 *   2 bytes for tx level (1 byte for type,  1 byte  for data)
 *  17 bytes for uuid     (1 byte for type, 16 bytes for data)
 *   7 bytes undefined
 * TODO: TX power is only set here. Why not when configuring as iBeacon?
 */
void Stack::configureBleDeviceAdvData() {

	// check if (and how many) services where enabled
	uint8_t uidCount = _services.size();
	if (uidCount == 0) {
		LOGw(MSG_BLE_NO_CUSTOM_SERVICES);
	}

	memset(&_config_advdata, 0, sizeof(_config_advdata));
	_config_advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	_config_advdata.p_tx_power_level = &_tx_power_level;
}

/** Configurate scan response according to Crowstone specification.
 *
 * The Crownstone scan response payload has 31 bytes:
 *   1 byte  to define length of the data
 *   X bytes for service data (1 byte for type, 29-N bytes for data)
 *   N bytes for name         (1 byte for type,  N-1 bytes for data)
 */
void Stack::configureScanResponse(uint8_t deviceType) {
	LOGd("Configure scan response and call ble_advdata_encode");

	uint8_t serviceDataLength = 0;

	memset(&_config_scanrsp, 0, sizeof(_config_scanrsp));
	_config_scanrsp.name_type = BLE_ADVDATA_SHORT_NAME;

	if (_serviceData && deviceType != DEVICE_UNDEF) {
		memset(&_service_data, 0, sizeof(_service_data));

		_service_data.service_uuid = CROWNSTONE_PLUG_SERVICE_DATA_UUID;

		_service_data.data.p_data = _serviceData->getArray();
		_service_data.data.size = _serviceData->getArraySize();

		_config_scanrsp.p_service_data_array = &_service_data;
		_config_scanrsp.service_data_count = 1;

		LOGd("Add UUID %X", _service_data.service_uuid);
		serviceDataLength += 2 + sizeof(_service_data.service_uuid) + _service_data.data.size;
	}

	const uint8_t maxDataLength = 29;
	uint8_t nameLength = maxDataLength - serviceDataLength;
	LOGd("Maximum data length minus service data length: %i - %i = %i", maxDataLength, serviceDataLength, nameLength);
	uint8_t deviceNameLength = getDeviceName().length();
	nameLength = std::min(nameLength, deviceNameLength);
	LOGd("Set BLE name to length %i", nameLength);
	_config_scanrsp.short_name_len = nameLength;

	if (nameLength == 0) {
		LOGe("Scan response payload too large or device name not set");
		return;
	}

	// we now have to encode the data by an explicit call
	ret_code_t ret_code;
	_adv_data.scan_rsp_data.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
	LOGd("Create scan response of size %i", _adv_data.scan_rsp_data.len);
	if (_adv_data.scan_rsp_data.p_data == NULL) {
		_adv_data.scan_rsp_data.p_data = (uint8_t*)calloc(sizeof(uint8_t),_adv_data.scan_rsp_data.len);
	}
	ret_code = ble_advdata_encode(&_config_scanrsp, _adv_data.scan_rsp_data.p_data, &_adv_data.scan_rsp_data.len);
	APP_ERROR_CHECK(ret_code);
}

/** Configure the Crownstone as iBeacon device
 *
 * This sets the advertisements data, the scan response data, and the advertisement parameters. Subseqeuently, these
 * values are written to the SoftDevice in setAdvertisementData. If the device is already advertising it will be
 * temporarily stopped in that function.
 */
void Stack::configureIBeacon(IBeacon* beacon, uint8_t deviceType) {
	LOGi(FMT_BLE_CONFIGURE_AS, "iBeacon");
	configureIBeaconAdvData(beacon);
	configureScanResponse(deviceType);
	configureAdvertisementParameters();
	setAdvertisementData();
}

void Stack::configureBleDevice(uint8_t deviceType) {
	LOGi(FMT_BLE_CONFIGURE_AS, "BleDevice");
	configureBleDeviceAdvData();
	configureScanResponse(deviceType);
	configureAdvertisementParameters();
	setAdvertisementData();
}

/**
 * It is only possible to include TX power if the advertisement is an "extended" type.
 */
void Stack::configureAdvertisementParameters() {
	LOGd("set _adv_params");
	_adv_params.primary_phy                 = BLE_GAP_PHY_1MBPS;
	_adv_params.properties.type             = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
	_adv_params.properties.anonymous        = 0;
	_adv_params.properties.include_tx_power = 0;
	_adv_params.p_peer_addr                 = NULL;
	_adv_params.filter_policy               = BLE_GAP_ADV_FP_ANY;
	_adv_params.interval                    = _interval;
	_adv_params.duration                    = _timeout;
}

void Stack::setConnectable() {
	// TODO: Have a function that sets address, which sends an event "set address" that makes the scanner pause (etc).
	LOGd("setConnectable");
	_adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
	// TODO: The identity address cannot be changed while advertising, scanning or creating a connection.
//	uint32_t ret_code;
//	do {
//		ret_code = sd_ble_gap_addr_set(&_connectable_address);
//	} while (ret_code == NRF_ERROR_INVALID_STATE);
//	NRF_LOG_FLUSH();
//	APP_ERROR_CHECK(ret_code);
}

void Stack::setNonConnectable() {
	LOGd("setNonConnectable");
	_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
//	_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED;
	// TODO: The identity address cannot be changed while advertising, scanning or creating a connection.
//	uint32_t ret_code;
//	do {
//		ret_code = sd_ble_gap_addr_set(&_nonconnectable_address);
//	} while (ret_code == NRF_ERROR_INVALID_STATE);
//	NRF_LOG_FLUSH();
//	APP_ERROR_CHECK(ret_code);
}

void Stack::restartAdvertising() {
	// TODO: do we still need to restart advertising to update advertisement?
	LOGd("Restart advertising");
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;

	if (_advertising) {
		stopAdvertising();
	}
	bool connectable = false;
	bool scannable = false;

	switch (_adv_params.properties.type) {
	case BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_CONNECTABLE_NONSCANNABLE_DIRECTED_HIGH_DUTY_CYCLE:
	case BLE_GAP_ADV_TYPE_CONNECTABLE_NONSCANNABLE_DIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_CONNECTABLE_NONSCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_CONNECTABLE_NONSCANNABLE_DIRECTED:
		connectable = true;
		break;
	case BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_SCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_SCANNABLE_DIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_DIRECTED:
	default:
		connectable = false;
		break;
	}

	switch (_adv_params.properties.type) {
	case BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_SCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_SCANNABLE_DIRECTED:
		scannable = true;
		break;
	case BLE_GAP_ADV_TYPE_CONNECTABLE_NONSCANNABLE_DIRECTED_HIGH_DUTY_CYCLE:
	case BLE_GAP_ADV_TYPE_CONNECTABLE_NONSCANNABLE_DIRECTED:
	case BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_CONNECTABLE_NONSCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_CONNECTABLE_NONSCANNABLE_DIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED:
	case BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_DIRECTED:
	default:
		scannable = false;
		break;
	}

	if (scannable) {
		ret_code_t ret_code;
		_adv_data.scan_rsp_data.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
		if (_adv_data.scan_rsp_data.p_data == NULL) {
			_adv_data.scan_rsp_data.p_data = (uint8_t*)calloc(sizeof(uint8_t),_adv_data.scan_rsp_data.len);
		}
		ret_code = ble_advdata_encode(&_config_scanrsp, _adv_data.scan_rsp_data.p_data, &_adv_data.scan_rsp_data.len);
		APP_ERROR_CHECK(ret_code);
	}
	else {
		if (_adv_data.scan_rsp_data.p_data != NULL) {
			free(_adv_data.scan_rsp_data.p_data);
		}
		_adv_data.scan_rsp_data.p_data = NULL;
		_adv_data.scan_rsp_data.len = 0;
	}

	if (connectable) {
		// TODO: The identity address cannot be changed while advertising, scanning or creating a connection.
//		uint32_t ret_code;
//		do {
//			ret_code = sd_ble_gap_addr_set(&_connectable_address);
//		} while (ret_code == NRF_ERROR_INVALID_STATE);
//		APP_ERROR_CHECK(ret_code);
	}
	else {
		// TODO: The identity address cannot be changed while advertising, scanning or creating a connection.
//		uint32_t ret_code;
//		do {
//			ret_code = sd_ble_gap_addr_set(&_nonconnectable_address);
//		} while (ret_code == NRF_ERROR_INVALID_STATE);
//		APP_ERROR_CHECK(ret_code);
	}

	uint32_t err;
	err = sd_ble_gap_adv_set_configure(&_adv_handle, &_adv_data, &_adv_params);
//	err = sd_ble_gap_adv_set_configure(&_adv_handle, &_adv_data, NULL);
	if (err != NRF_SUCCESS) {
		printAdvertisement();
	}
	APP_ERROR_CHECK(err);
	startAdvertising();
}

void Stack::startAdvertising() {
	if (_advertising) {
		LOGw("Should not already be advertising");
		return;
	}

	static int max_debug = 3;
	static int i = 0;
	if (i < max_debug) {
		LOGd("sd_ble_gap_adv_start(_adv_handle=%i,_conn_cfg_tag=%i)", _adv_handle, _conn_cfg_tag);
		i++;
		if (i == max_debug) {
			LOGd("Suppress further messages about adv start");
		}
	}
	uint32_t ret_code = sd_ble_gap_adv_start(_adv_handle, _conn_cfg_tag); // Only one advertiser may be active at any time.
	if (ret_code == NRF_SUCCESS) {
		_advertising = true;
	}
	APP_ERROR_CHECK(ret_code);
}

void Stack::stopAdvertising() {
	if (!_advertising) {
		LOGw("In stopAdvertising: already not advertising");
		return;
	}

	// This function call can take 31ms
	LOGd("sd_ble_gap_adv_stop(_adv_handle=%i", _adv_handle);
	uint32_t ret_code = sd_ble_gap_adv_stop(_adv_handle);
	// Ignore invalid state error,
	// see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
	APP_ERROR_CHECK_EXCEPT(ret_code, NRF_ERROR_INVALID_STATE);

	//LOGi(MSG_BLE_ADVERTISING_STOPPED);

	_advertising = false;
}

/** Update the advertisement package.
 *
 * This function toggles between connectable and non-connectable advertisements.
 * If we are connected we will not advertise connectable advertisements.
 *
 * @param toggle  When false, use connectable, when true toggle between connectable and non-connectable.
 */
// TODO: rename this function.
void Stack::updateAdvertisement(bool toggle) {
	if (!checkCondition(C_ADVERTISING, true)) return;

	if (isScanning()) {
		// Skip while scanning or we will get invalid state results when stopping/starting advertisement. TODO: is that so???
		return;
	}

	if (isConnected()) {
		// No need to restart, since it's already non-connectable.
		return;
	}

	bool connectable = true;
	if (toggle) {
		connectable = (++_advInterleaveCounter % 2);
	}
	LOGd("updateAdvertisement connectable %i", connectable);
	NRF_LOG_FLUSH();

	if (connectable) {
		setConnectable();
	} else {
		setNonConnectable();
	}

	restartAdvertising();
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
	case C_ADVERTISING:
		field = _advertising;
		if (expectation != field) {
			LOGw("Advertising init %s", field ? "true" : "false");
		}
		break;
	}
	return field;
}

/**
 * After this function _adv_handle should be valid. Note, that we are not allowed to:
 *
 *   call sd_ble_gap_adv_set_configure with adv_params != NULL while advertising
 *   call sd_ble_gap_adv_set_configure again with the same pointer to _adv_data
 *
 * If adv_params != NULL we are temporarily stopping advertising.
 * If our data is different we have to provide a different pointer to _adv_data.
 */
void Stack::setAdvertisementData() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;

	uint32_t err;

	bool first_time = false;
	if (_adv_handle == BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		first_time = true;
	}

	if (first_time) {
		printAdvertisement();
//		LOGv("sd_ble_gap_adv_set_configure(_adv_handle=%i,...,...)", _adv_handle);
//		LOGv("First bytes in encoded buffer: [%02x %02x %02x %02x]",
//				_adv_data.adv_data.p_data[0], _adv_data.adv_data.p_data[1],
//				_adv_data.adv_data.p_data[2], _adv_data.adv_data.p_data[3]
//		    );
//		LOGv("Length is: %i", _adv_data.adv_data.len);
		err = sd_ble_gap_adv_set_configure(&_adv_handle, &_adv_data, &_adv_params);
		APP_ERROR_CHECK(err);
	}
	else {
		// TODO: use 2 buffers and switch between them. Should be explained in the migration doc (from sdk 14 to 15).
		// TODO: for now, just do nothing
		return;
		// update now does not just allow adv_data to point to something else...
		//LOGw("Cannot just update adv_params on the fly");
		bool temporary_stop_advertising = _advertising;
		if (temporary_stop_advertising) {
			stopAdvertising();
		}

		// TODO: If we want to change the adv data without stopping, the adv_data should be a new buffer,
		// and adv_params should be NULL.
		err = sd_ble_gap_adv_set_configure(&_adv_handle, &_adv_data, &_adv_params);
		APP_ERROR_CHECK(err);
		if (temporary_stop_advertising) {
			startAdvertising();
		}
	}
}

void Stack::printAdvertisement() {
	LOGd("_adv_handle=%u", _adv_handle);

	LOGd("adv_data len=%u data:", _adv_data.adv_data.len);
	char str[4*8+1] = {0};
	int strIndex = 0;
	for (size_t i=0; i<_adv_data.adv_data.len; ++i) {
		strIndex += sprintf(&(str[strIndex]), "%3u ", _adv_data.adv_data.p_data[i]);
		if (i % 8 == 8-1) {
			LOGd("%s", str);
			memset(str, 0, sizeof(str));
			strIndex = 0;
		}
	}

	LOGd("scan_rsp_data len=%u data:", _adv_data.scan_rsp_data.len);
	memset(str, 0, sizeof(str));
	strIndex = 0;
	for (size_t i=0; i<_adv_data.scan_rsp_data.len; ++i) {
		strIndex += sprintf(&(str[strIndex]), "%3u ", _adv_data.scan_rsp_data.p_data[i]);
		if (i % 8 == 8-1) {
			LOGd("%s", str);
			memset(str, 0, sizeof(str));
			strIndex = 0;
		}
	}

	LOGd("type=%u", _adv_params.properties.type);
	LOGd("channel mask: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
			_adv_params.channel_mask[0],
			_adv_params.channel_mask[1],
			_adv_params.channel_mask[2],
			_adv_params.channel_mask[3],
			_adv_params.channel_mask[4]);
}

void Stack::startScanning() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	if (_scanning)
		return;

	LOGi(FMT_START, "scanning");
	ble_gap_scan_params_t p_scan_params;
	p_scan_params.active = 0;
	p_scan_params.timeout = 0x0000;

	State::getInstance().get(CS_TYPE::CONFIG_SCAN_INTERVAL, &p_scan_params.interval, sizeof(p_scan_params.interval));
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_WINDOW, &p_scan_params.window, sizeof(p_scan_params.window));

	// TODO: which fields to set here?
	// TODO: p_adv_report_buffer
	//BLE_CALL(sd_ble_gap_scan_start, (&p_scan_params));
	_scanning = true;

	event_t event(CS_TYPE::EVT_SCAN_STARTED, NULL, 0);
	EventDispatcher::getInstance().dispatch(event);
}


void Stack::stopScanning() {
	if (!checkCondition(C_RADIO_INITIALIZED, true)) return;
	if (!_scanning)
		return;

	LOGi(FMT_STOP, "scanning");
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

void Stack::lowPowerTimeout(void* p_context) {
	LOGw("bonding timeout!");
	((Stack*)p_context)->changeToNormalTxPowerMode();
}

void Stack::changeToLowTxPowerMode() {
	TYPIFY(CONFIG_LOW_TX_POWER) lowTxPower;
	State::getInstance().get(CS_TYPE::CONFIG_LOW_TX_POWER, &lowTxPower, sizeof(lowTxPower));
	setTxPowerLevel(lowTxPower);
}

void Stack::changeToNormalTxPowerMode() {
	TYPIFY(CONFIG_TX_POWER) txPower;
	State::getInstance().get(CS_TYPE::CONFIG_TX_POWER, &txPower, sizeof(txPower));
	setTxPowerLevel(txPower);
}

void Stack::on_ble_evt(const ble_evt_t * p_ble_evt) {

	if (p_ble_evt->header.evt_id !=  BLE_GAP_EVT_RSSI_CHANGED) {
		const char *evt_name __attribute__((unused)) = NordicEventTypeName(p_ble_evt->header.evt_id);
		LOGd("BLE event %i (0x%X) %s", p_ble_evt->header.evt_id, p_ble_evt->header.evt_id, evt_name);
	}

	if (_dm_initialized) {
		// Note: peer manager is removed
	}

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED: {
		on_connected(p_ble_evt);
		event_t event(CS_TYPE::EVT_BLE_CONNECT, NULL, 0);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case BLE_EVT_USER_MEM_REQUEST: {

		buffer_ptr_t buffer = NULL;
		uint16_t size = 0;
		MasterBuffer::getInstance().getBuffer(buffer, size);

		_user_mem_block.len = size+6;
		_user_mem_block.p_mem = buffer-6;
		BLE_CALL(sd_ble_user_mem_reply, (getConnectionHandle(), &_user_mem_block));

		break;
	}
	case BLE_EVT_USER_MEM_RELEASE:
		// nothing to do
		break;
	case BLE_GAP_EVT_DISCONNECTED: {
		on_disconnected(p_ble_evt);
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

			buffer_ptr_t buffer = NULL;
			uint16_t size = 0;

			// we want to have the actual payload word aligned, but header only needs 6 bytes,
			// so we need to skip the first 2 bytes
			MasterBuffer::getInstance().getBuffer(buffer, size, 2);

			uint16_t* header = (uint16_t*)buffer;

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

	case BLE_GAP_EVT_ADV_REPORT: {
		const ble_gap_evt_adv_report_t* advReport = &(p_ble_evt->evt.gap_evt.params.adv_report);
		scanned_device_t scan;
		memcpy(scan.address, advReport->peer_addr.addr, sizeof(scan.address));
		scan.rssi = advReport->rssi;
		scan.channel = advReport->ch_index;
		scan.dataSize = advReport->data.len;
		scan.data = advReport->data.p_data;

		event_t event(CS_TYPE::EVT_DEVICE_SCANNED, (void*)&scan, sizeof(scan));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
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

void Stack::on_connected(const ble_evt_t * p_ble_evt) {
	LOGd("Connection event");
	//ble_gap_evt_connected_t connected_evt = p_ble_evt->evt.gap_evt.params.connected;
	_advertising = false; // Advertising stops on connect, see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
	_disconnectingInProgress = false;
	BLE_CALL(sd_ble_gap_conn_param_update, (p_ble_evt->evt.gap_evt.conn_handle, &_gap_conn_params));

//	ble_gap_addr_t* peerAddr = &p_ble_evt->evt.gap_evt.params.connected.peer_addr;
//	LOGi("Connection from: %02X:%02X:%02X:%02X:%02X:%02X", peerAddr->addr[5],
//	                       peerAddr->addr[4], peerAddr->addr[3], peerAddr->addr[2], peerAddr->addr[1],
//	                       peerAddr->addr[0]);

	if (_callback_connected) {
		_callback_connected(p_ble_evt->evt.gap_evt.conn_handle);
	}
	_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
	for (Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}

	startConnectionAliveTimer();
}

void Stack::on_disconnected(const ble_evt_t * p_ble_evt) {
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
		uint32_t errorCode = sd_ble_gap_disconnect(_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		if (errorCode != NRF_ERROR_INVALID_STATE) {
			APP_ERROR_CHECK(errorCode);
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

void Stack::onConnect(const callback_connected_t& callback) {

	_callback_connected = callback;

	// TODO: why update connection parameters here?
	updateConnParams();
}

void Stack::onDisconnect(const callback_disconnected_t& callback) {
	_callback_disconnected = callback;
}
