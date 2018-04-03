/*
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 23, 2015
 * License: LGPLv3+
 */

#include "ble/cs_Stack.h"

#include "cfg/cs_Config.h"
#include "ble/cs_Handlers.h"

#include "structs/buffer/cs_MasterBuffer.h"

#if BUILD_MESHING == 1
extern "C" {
	#include <rbc_mesh.h>
}
#endif

#include "ble_stack_handler_types.h"

extern "C" {
#include "pstorage.h"
#include "app_util.h"
}

#include <drivers/cs_Storage.h>
#include <storage/cs_Settings.h>
#include "util/cs_Utils.h"
#include <cfg/cs_UuidConfig.h>

#include <events/cs_EventDispatcher.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Tracker.h>

//#define PRINT_STACK_VERBOSE

// Define test pin to enable gpio debug.
//#define TEST_PIN 19

Nrf51822BluetoothStack::Nrf51822BluetoothStack() :
				_appearance(BLE_APPEARANCE_GENERIC_TAG), _clock_source(defaultClockSource),
//				_mtu_size(defaultMtu),
				_tx_power_level(TX_POWER), _sec_mode({ }),
				_interval(defaultAdvertisingInterval_0_625_ms), _timeout(defaultAdvertisingTimeout_seconds),
			  	_gap_conn_params( { }),
				_inited(false), _initializedServices(false), _initializedRadio(false), _advertising(false), _scanning(false),
				_conn_handle(BLE_CONN_HANDLE_INVALID),
				_radio_notify(0),
				_dm_app_handle(0), _dm_initialized(false),

				_lowPowerTimeoutId(NULL),
                _secReqTimerId(NULL),
                _connectionKeepAliveTimerId(NULL),

                _advParamsCounter(0),
				_adv_manuf_data(NULL),
                _serviceData(NULL)
{

	_lowPowerTimeoutData = { {0} };
	_lowPowerTimeoutId = &_lowPowerTimeoutData;
	_secReqTimerData = { {0} };
	_secReqTimerId = &_secReqTimerData;
	_connectionKeepAliveTimerData = { {0} };
	_connectionKeepAliveTimerId = &_connectionKeepAliveTimerData;

	//! setup default values.
	memcpy(_passkey, STATIC_PASSKEY, BLE_GAP_PASSKEY_LEN);

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&_sec_mode);

	_gap_conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
	_gap_conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
	_gap_conn_params.slave_latency = SLAVE_LATENCY;
	_gap_conn_params.conn_sup_timeout = CONNECTION_SUPERVISION_TIMEOUT;
}

Nrf51822BluetoothStack::~Nrf51822BluetoothStack() {
	shutdown();
}

/*
const nrf_clock_lf_cfg_t Nrf51822BluetoothStack::defaultClockSource = {.source        = NRF_CLOCK_LF_SRC_RC,   \
                                                                       .rc_ctiv       = 16,                    \
                                                                       .rc_temp_ctiv  = 2,                     \
                                                                       .xtal_accuracy = 0};
*/
/*
const nrf_clock_lf_cfg_t Nrf51822BluetoothStack::defaultClockSource = {.source        = NRF_CLOCK_LF_SRC_SYNTH,\
                                                                       .rc_ctiv       = 0,                     \
                                                                       .rc_temp_ctiv  = 0,                     \
                                                                       .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_50_PPM};
*/
///*
const nrf_clock_lf_cfg_t Nrf51822BluetoothStack::defaultClockSource = { .source        = NRF_CLOCK_LF_SRC_XTAL,        \
                                                                        .rc_ctiv       = 0,                     \
                                                                        .rc_temp_ctiv  = 0,                     \
                                                                        .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
//*/

//! called by softdevice handler through ble_evt_dispatch on any event that passes through mesh and is not write
//extern "C" void ble_evt_handler(void* p_event_data, uint16_t event_size) {
//	Nrf51822BluetoothStack::getInstance().on_ble_evt((ble_evt_t *)p_event_data);
//}

//! called by softdevice handler on a ble event
extern "C" void ble_evt_dispatch(ble_evt_t* p_ble_evt) {

//	LOGi("Dispatch event %i", p_ble_evt->header.evt_id);

//! Example of how you can get the connection handle and role
// conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
// role        = ble_conn_state_role(conn_handle);

#if BUILD_MESHING == 1
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		//!  pass the incoming BLE event to the mesh framework
		rbc_mesh_ble_evt_handler(p_ble_evt);
	}
#endif

	//! Only dispatch functions to the scheduler which might take long to execute, such as ble write functions
	//! and handle other ble events directly in the interrupt. Otherwise app scheduler buffer might overflow fast
//	switch (p_ble_evt->header.evt_id) {
//	case BLE_GATTS_EVT_WRITE:
		//! let the scheduler execute the event handle function
//		LOGi("BLE_WRITE");
//		BLE_CALL(app_sched_event_put, (p_ble_evt, sizeof (ble_evt_hdr_t) + p_ble_evt->header.evt_len, ble_evt_handler));
//		break;
//	default:
//		ble_evt_handler(p_ble_evt, 0);
		Nrf51822BluetoothStack::getInstance().on_ble_evt(p_ble_evt);
//		break;
//	}
}

void Nrf51822BluetoothStack::init() {

	if (_inited) {
		LOGw("Already initialized");
		return;
	}

#ifdef TEST_PIN
	nrf_gpio_cfg_output(TEST_PIN);
#endif

	// Initialize SoftDevice
	uint8_t enabled;
	BLE_CALL(sd_softdevice_is_enabled, (&enabled));
	if (enabled) {
		LOGw(MSG_BLE_SOFTDEVICE_RUNNING);
		BLE_CALL(sd_softdevice_disable, ());
	}

	LOGi(MSG_BLE_SOFTDEVICE_INIT);
	// Initialize the SoftDevice handler module.
	// This would call with different clock!
	SOFTDEVICE_HANDLER_APPSH_INIT(&_clock_source, true);

	// Enable the softdevice
	LOGi(MSG_BLE_SOFTDEVICE_ENABLE);

	// Assign ble event handler, forward ble_evt to stack
	BLE_CALL(softdevice_ble_evt_handler_set, (ble_evt_dispatch));


	// Set system event handler
	BLE_CALL(softdevice_sys_evt_handler_set, (sys_evt_dispatch));

	// Enable power-fail comparator
	sd_power_pof_enable(true);
	// set threshold value, if power falls below threshold,
	// an NRF_EVT_POWER_FAILURE_WARNING will be triggered.
	sd_power_pof_threshold_set(BROWNOUT_TRIGGER_THRESHOLD);

	_inited = true;
}

void Nrf51822BluetoothStack::initRadio() {
	if (!_inited) {
		LOGw(FMT_NOT_INITIALIZED, "stack");
		return;
	}
	if (_initializedRadio) {
		LOGw(FMT_ALREADY_INITIALIZED, "radio");
		return;
	}

	// Do not define the service_changed characteristic, of course allow future changes
#define IS_SRVC_CHANGED_CHARACT_PRESENT  1

#define CENTRAL_LINK_COUNT 0
#define PERIPHERAL_LINK_COUNT 1

	// Set BLE stack parameters
	ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));
	BLE_CALL(softdevice_enable_get_default_config, (CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT, &ble_enable_params) );
	ble_enable_params.gatts_enable_params.attr_tab_size = ATTR_TABLE_SIZE;
	ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
	ble_enable_params.common_enable_params.vs_uuid_count = MAX_NUM_VS_SERVICES;

	// Enable BLE stack.
	ret_code_t err_code;
	uint32_t ramBase = RAM_R1_BASE;
	err_code = sd_ble_enable(&ble_enable_params, &ramBase);
	if (err_code == NRF_ERROR_NO_MEM) {
		ramBase = 0;
		// This sets ramBase to the minimal value.
		sd_ble_enable(&ble_enable_params, &ramBase);
		LOGe("RAM_R1_BASE should be: %p", ramBase);
		APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
	}
	APP_ERROR_CHECK(err_code);

	// Version is not saved or shown yet
	ble_version_t version( { });
	version.company_id = 12;
	BLE_CALL(sd_ble_version_get, (&version));

	if (!_device_name.empty()) {
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) _device_name.c_str(), _device_name.length()));
	}
	else {
		std::string name = "noname";
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) name.c_str(), name.length()));
	}
	BLE_CALL(sd_ble_gap_appearance_set, (_appearance));

	_initializedRadio = true;

	updateConnParams();

	updatePasskey();

	updateTxPowerLevel();

	// TODO: if initRadio is called later, the advertisement data should be set.
//	if (_advdata ... && _scan_resp ... ?
//	setAdvertisementData();
}


void Nrf51822BluetoothStack::setAppearance(uint16_t appearance) {
	_appearance = appearance;
}

void Nrf51822BluetoothStack::setDeviceName(const std::string& deviceName) {
	_device_name = deviceName;
}

void Nrf51822BluetoothStack::setClockSource(nrf_clock_lf_cfg_t clockSource) {
	_clock_source = clockSource;
}

void Nrf51822BluetoothStack::setAdvertisingInterval(uint16_t advertisingInterval) {
	if (advertisingInterval < 0x0020 || advertisingInterval > 0x4000) {
		LOGw("Invalid advertising interval");
		return;
	}
	_interval = advertisingInterval;
}

void Nrf51822BluetoothStack::updateAdvertisingInterval(uint16_t advertisingInterval, bool apply) {
	if (!_initializedRadio) {
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}
	setAdvertisingInterval(advertisingInterval);
	configureAdvertisementParameters();

	// Apply new interval.
	if (apply && _advertising) {
		stopAdvertising();
		startAdvertising();
	}
}

void Nrf51822BluetoothStack::updateDeviceName(const std::string& deviceName) {
	_device_name = deviceName;

	if (!_initializedRadio) {
		return;
	}
	if (!_device_name.empty()) {
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) _device_name.c_str(), _device_name.length()));
	} else {
		std::string name = "not set...";
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) name.c_str(), name.length()));
	}
}

void Nrf51822BluetoothStack::updateAppearance(uint16_t appearance) {
	_appearance = appearance;
	BLE_CALL(sd_ble_gap_appearance_set, (_appearance));
}

void Nrf51822BluetoothStack::createCharacteristics() {
	if (!_initializedRadio) {
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}
	for (Service* svc: _services) {
		svc->createCharacteristics();
	}
}

void Nrf51822BluetoothStack::initServices() {
	if (!_initializedRadio) {
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}
	if (_initializedServices) {
		LOGw(FMT_ALREADY_INITIALIZED, "services");
		return;
	}

	for (Service* svc : _services) {
		svc->init(this);
	}

	_initializedServices = true;
}

// TODO: remove this function
void Nrf51822BluetoothStack::startTicking() {
//	LOGi(FMT_START, "ticking");
//	for (Service* svc : _services) {
//		svc->startTicking();
//	}
}

void Nrf51822BluetoothStack::stopTicking() {
//	LOGi(FMT_STOP, "ticking");
//	for (Service* svc : _services) {
//		svc->stopTicking();
//	}
}

void Nrf51822BluetoothStack::shutdown() {
	stopAdvertising();

	for (Service* svc : _services) {
		svc->stopAdvertising();
	}

	_inited = false;
}

void Nrf51822BluetoothStack::addService(Service* svc) {
	//APP_ERROR_CHECK_BOOL(_services.size() < MAX_SERVICE_COUNT);

	_services.push_back(svc);
}

//! accepted values are -40, -30, -20, -16, -12, -8, -4, 0, and 4 dBm
//! Can be done at any moment (also when advertising)
void Nrf51822BluetoothStack::setTxPowerLevel(int8_t powerLevel) {
#ifdef PRINT_STACK_VERBOSE
	LOGd(FMT_SET_INT_VAL, "tx power", powerLevel);
#endif

	switch (powerLevel) {
	case -40: break;
//	case -30: break; // -30 is only available on nrf51
	case -20: break;
	case -16: break;
	case -12: break;
	case -8: break;
	case -4: break;
	case 0: break;
	case 4: break;
	default: return;
	}
	if (_tx_power_level != powerLevel) {
		_tx_power_level = powerLevel;
		if (_initializedRadio) {
			updateTxPowerLevel();
		}
	}
}

void Nrf51822BluetoothStack::updateTxPowerLevel() {
	BLE_CALL(sd_ble_gap_tx_power_set, (_tx_power_level));
}

void Nrf51822BluetoothStack::setMinConnectionInterval(uint16_t connectionInterval_1_25_ms) {
	if (_gap_conn_params.min_conn_interval != connectionInterval_1_25_ms) {
		_gap_conn_params.min_conn_interval = connectionInterval_1_25_ms;
		if (_initializedRadio) {
			updateConnParams();
		}
	}
}

void Nrf51822BluetoothStack::setMaxConnectionInterval(uint16_t connectionInterval_1_25_ms) {
	if (_gap_conn_params.max_conn_interval != connectionInterval_1_25_ms) {
		_gap_conn_params.max_conn_interval = connectionInterval_1_25_ms;
		if (_initializedRadio) {
			updateConnParams();
		}
	}
}

void Nrf51822BluetoothStack::setSlaveLatency(uint16_t slaveLatency) {
	if ( _gap_conn_params.slave_latency != slaveLatency ) {
		_gap_conn_params.slave_latency = slaveLatency;
		if (_initializedRadio) {
			updateConnParams();
		}
	}
}

void Nrf51822BluetoothStack::setConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms) {
	if (_gap_conn_params.conn_sup_timeout != conSupTimeout_10_ms) {
		_gap_conn_params.conn_sup_timeout = conSupTimeout_10_ms;
		if (_initializedRadio) {
			updateConnParams();
		}
	}
}

void Nrf51822BluetoothStack::updateConnParams() {
	BLE_CALL(sd_ble_gap_ppcp_set, (&_gap_conn_params));
}

void Nrf51822BluetoothStack::configureIBeaconAdvData(IBeacon* beacon) {

	uint32_t err_code __attribute__((unused));
	uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	//	uint8_t flags = BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_LE_BR_EDR_CONTROLLER
	//			| BLE_GAP_ADV_FLAG_LE_BR_EDR_HOST;

	/*
	 * 31 bytes total payload
	 *
	 * 1 byte per element for the length of it's data.
	 * in this case, 4 elements -> 4 bytes
	 *
	 * => 27 bytes available payload
	 *
	 * 3 bytes for major (1 byte for type, 2 byte for data)
	 * 3 bytes for minor (1 byte for type, 2 byte for data)
	 * 17 bytes for UUID (1 byte for type, 16 byte for data)
	 * 2 bytes for rssi (1 byte for type, 1 byte for data)
	 *
	 * -> 2 bytes left => FULL
	 *
	 * Note: each element has an overhead of 2 bytes. Since there
	 * are only 2 bytes left, there is no more space left for additional
	 * data
	 */

//	ble_advdata_manuf_data_t manufac_apple;
	memset(&_manufac_apple, 0, sizeof(_manufac_apple));
	_manufac_apple.company_identifier = 0x004C; //! Apple Company ID, if it is not set to apple it's not recognized as an iBeacon

//	uint8_t adv_manuf_data_apple[beacon->size()];
//	if (adv_manuf_data_apple != NULL) {
//		free(adv_manuf_data_apple);
//	}

//	adv_manuf_data_apple = new uint8_t[beacon->size()];
//	memset(adv_manuf_data_apple, 0, sizeof(*adv_manuf_data_apple));
//	beacon->toArray(adv_manuf_data_apple);

//	manufac_apple.data.p_data = adv_manuf_data_apple;
	_manufac_apple.data.p_data = beacon->getArray();
	_manufac_apple.data.size = beacon->size();

	//! Build and set advertising data
//	ble_advdata_t advdata;
	memset(&_advdata, 0, sizeof(_advdata));

	_advdata.flags = flags;
	_advdata.p_manuf_specific_data = &_manufac_apple;

}

void Nrf51822BluetoothStack::configureBleDeviceAdvData() {

	//! check if (and how many) services where enabled
	uint8_t uidCount = _services.size();

#ifdef PRINT_STACK_VERBOSE
	LOGd("Number of services: %u", uidCount);
#endif


//	ble_uuid_t adv_uuids[uidCount];
//
//	uint8_t cnt = 0;
//	for (Service* svc : _services) {
//		adv_uuids[cnt++] = svc->getUUID();
//	}

	if (uidCount == 0) {
		LOGw(MSG_BLE_NO_CUSTOM_SERVICES);
	}

	//! Build and set advertising data

	/*
	 * 31 bytes total payload
	 *
	 * 1 byte per element for the length of it's data.
	 * in this case, 3 elements -> 3 bytes
	 *
	 * => 28 bytes available payload
	 *
	 * 2 bytes for flags (1 byte for type, 1 byte for data)
	 * 2 bytes for tx power level (1 byte for type, 1 byte for data)
	 * 17 bytes for UUID (1 byte for type, 16 byte for data)
	 *
	 * -> 7 bytes left
	 *
	 * Note: by adding an element, one byte will be lost because of the
	 *   addition, so there are actually only 6 bytes left after adding
	 *   an element, and 1 byte of that is used for the type, so 5 bytes
	 *   are left for the data
	 */
//	ble_advdata_t advdata;
	memset(&_advdata, 0, sizeof(_advdata));

	//! Anne: setting NO_NAME breaks the Android Nordic nRF Master Console app.
	//! assign tx power level to advertisement data
	_advdata.p_tx_power_level = &_tx_power_level;

	//! set flags of advertisement data
	uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	_advdata.flags = flags;

	//! add (first) service uuid. there is only space for 1 uuid, since we use
	//! 128-bit UUDs
	//! TODO -oDE: It doesn't really make sense to have this function in the library
	//!  because it really depends on the application on how many and what kind
	//!  of services are available and what should be advertised
	//#ifdef YOU_WANT_TO_USE_SPACE
//	if (uidCount > 1) {
//		advdata.uuids_more_available.uuid_cnt = 1;
//		advdata.uuids_more_available.p_uuids = adv_uuids;
//	} else if (uidCount == 1) {
//		advdata.uuids_complete.uuid_cnt = 1;
//		advdata.uuids_complete.p_uuids = adv_uuids;
//	}
	//#endif

}

void Nrf51822BluetoothStack::configureScanResponse(uint8_t deviceType) {

	//! Because of the limited amount of space in the advertisement data, additional
	//! data can be supplied in the scan response package. Same space restrictions apply
	//! here:
	/*
	 * 31 bytes total payload
	 *
	 * 1 byte per element for the length of it's data.
	 * in this case, 1 element -> 1 byte
	 *
	 * 30 bytes of available payload
	 *
	 * 1 byte for name type
	 * -> 29 bytes left for name
	 */
#define MAXNAMELENGTH 29
	uint8_t nameLength = MAXNAMELENGTH;

//	ble_advdata_t scan_resp;
	memset(&_scan_resp, 0, sizeof(_scan_resp));
	_scan_resp.name_type = BLE_ADVDATA_SHORT_NAME;

	if (_serviceData && deviceType != DEVICE_UNDEF) {
	//	ble_advdata_service_data_t service_data;
		memset(&_service_data, 0, sizeof(_service_data));

//		switch(deviceType) {
//			case DEVICE_CROWNSTONE_PLUG: {
//				_service_data.service_uuid = CROWNSTONE_PLUG_SERVICE_DATA_UUID;
//				break;
//			}
//			case DEVICE_CROWNSTONE_BUILTIN: {
//				_service_data.service_uuid = CROWNSTONE_BUILT_SERVICE_DATA_UUID;
//				break;
//			}
//			case DEVICE_GUIDESTONE: {
//				_service_data.service_uuid = GUIDESTONE_SERVICE_DATA_UUID;
//				break;
//			}
//		}
		_service_data.service_uuid = CROWNSTONE_PLUG_SERVICE_DATA_UUID;

		_service_data.data.p_data = _serviceData->getArray();
		_service_data.data.size = _serviceData->getArraySize();

//		LOGd("service data size: %d", _service_data.data.size);

		_scan_resp.p_service_data_array = &_service_data;
		_scan_resp.service_data_count = 1;

		nameLength = nameLength - 2 - sizeof(_service_data.service_uuid) - _service_data.data.size;
	}

	//! since advertisement data already has the manufacturing data
	//! of Apple for the iBeacon, we set our own manufacturing data
	//! in the scan response

	//! only add manufacturing data if device type is set
//	if (deviceType != DEVICE_UNDEF) {
//
////		ble_advdata_manuf_data_t manufac;
//		memset(&_manufac, 0, sizeof(_manufac));
//		//! TODO: made up ID, has to be replaced by official ID
//		_manufac.company_identifier = CROWNSTONE_COMPANY_ID; //! DoBots Company ID
//		_manufac.data.size = 0;
//
//		CrownstoneManufacturer crownstoneManufacturer(deviceType);
//
////		uint8_t adv_manuf_data[crownstoneManufacturer.size()];
//
//		if (_adv_manuf_data != NULL) {
//			free(_adv_manuf_data);
//		}
//
//		_adv_manuf_data = new uint8_t[crownstoneManufacturer.size()];
//		memset(_adv_manuf_data, 0, sizeof(*_adv_manuf_data));
//		crownstoneManufacturer.toArray(_adv_manuf_data);
//
//		_manufac.data.p_data = _adv_manuf_data;
//		_manufac.data.size = crownstoneManufacturer.size();
//
//		_scan_resp.p_manuf_specific_data = &_manufac;
//
//		//! we only have limited space available in the scan response data. so we have to adjust the maximum
//		//! length available for the name, based on the size of the manufacturing data
//		nameLength = nameLength - sizeof(_adv_manuf_data) - sizeof(_manufac.company_identifier) - 2 ; //! last 2 comes from one byte for length + 1 byte for type
//	} else {
//		//! if no manufacturing data is set, we can use the maximum available space for the name
////		nameLength = nameLength;
//	}

//	LOGd("maxNameLength: %d", nameLength);

	//! NOTE: if anything else is added to the scan response data, the nameLength has to be adjusted
	//!   similar to the case of manufacturing data
	//! last, make sure we don't try to use more letters than the name is long or it will attach
	//! garbage to the name
	nameLength = std::min(nameLength, (uint8_t)(getDeviceName().length()));

	//! and assign the value to the advertisement package
	_scan_resp.short_name_len = nameLength;

}

void Nrf51822BluetoothStack::configureIBeacon(IBeacon* beacon, uint8_t deviceType) {
	LOGi(FMT_BLE_CONFIGURE_AS, "iBeacon");

	configureIBeaconAdvData(beacon);

	configureScanResponse(deviceType);

	setAdvertisementData();

	// set advertisement parameters
	configureAdvertisementParameters();
}

void Nrf51822BluetoothStack::configureBleDevice(uint8_t deviceType) {
	LOGi(FMT_BLE_CONFIGURE_AS, "BleDevice");

	configureBleDeviceAdvData();

	configureScanResponse(deviceType);

	setAdvertisementData();

	// set advertisement parameters
	configureAdvertisementParameters();
}

void Nrf51822BluetoothStack::configureAdvertisementParameters() {
	_adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	_adv_params.p_peer_addr = NULL;                   // Undirected advertisement
	_adv_params.fp = BLE_GAP_ADV_FP_ANY;
	_adv_params.interval = _interval;
	_adv_params.timeout = _timeout;
}

void Nrf51822BluetoothStack::setConnectable() {
	_adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
}

void Nrf51822BluetoothStack::setNonConnectable() {
	_adv_params.type = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
}

void Nrf51822BluetoothStack::restartAdvertising() {
	if (!_initializedRadio) {
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}

	uint32_t err_code;
	if (_advertising) {
#ifdef TEST_PIN
	nrf_gpio_pin_set(TEST_PIN);
#endif
		err_code = sd_ble_gap_adv_stop(); // This function call can take 30ms!
#ifdef TEST_PIN
	nrf_gpio_pin_clear(TEST_PIN);
#endif
		// Ignore invalid state error, see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
		if (err_code != NRF_ERROR_INVALID_STATE) {
			APP_ERROR_CHECK(err_code);
		}
		else {
			LOGw("adv_stop invalid state");
		}
	}

	err_code = sd_ble_gap_adv_start(&_adv_params);
	switch (err_code) {
	case NRF_ERROR_BUSY:
		// Docs say: retry again later.
		// Since restartAdvertising() gets called all the time anyway, I guess we don't have to schedule a retry.
		LOGw("adv_start busy");
		break;
	case NRF_ERROR_INVALID_PARAM:
		LOGf(MSG_BLE_ADVERTISEMENT_CONFIG_INVALID);
		APP_ERROR_CHECK(err_code);
		_advertising = true;
		break;
	default:
		APP_ERROR_CHECK(err_code);
		_advertising = true;
	}
}

void Nrf51822BluetoothStack::startAdvertising() {
	if (_advertising) {
		LOGw("invalid state");
		return;
	}

	LOGi(MSG_BLE_ADVERTISING_STARTING);

	uint32_t err_code;
	err_code = sd_ble_gap_adv_start(&_adv_params);
	if (err_code == NRF_ERROR_INVALID_PARAM) {
		LOGf(MSG_BLE_ADVERTISEMENT_CONFIG_INVALID);
	}
	APP_ERROR_CHECK(err_code);

	_advertising = true;

//	LOGi(MSG_BLE_ADVERTISING_STARTED);
}

void Nrf51822BluetoothStack::stopAdvertising() {
	if (!_advertising) {
		LOGw("invalid state");
		return;
	}

	uint32_t err_code = sd_ble_gap_adv_stop();
	// Ignore invalid state error, see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
	if (err_code != NRF_ERROR_INVALID_STATE) {
		APP_ERROR_CHECK(err_code);
	}

	LOGi(MSG_BLE_ADVERTISING_STOPPED);

	_advertising = false;
}

void Nrf51822BluetoothStack::updateAdvertisement(bool toggle) {
	if (!_advertising)
		// only update if actually advertising
		return;

	if (isConnected() || isScanning()) {
		// skip as long as we are connected or it will interfere with connection
		// also skip while scanning or we will get invalid state results when stopping/starting advertisement
		return;
	} else {
//		LOGi("UPDATE advertisement");

//		uint32_t err_code;
//		err_code = sd_ble_gap_adv_stop();
//		APP_ERROR_CHECK(err_code);

		// if toggle is false, advertisement should be always connectable, otherwise, toggle between
		// connectable and non connectable
		if (toggle) {
			if (++_advParamsCounter %2) {
				setConnectable();
			} else {
				setNonConnectable();
			}
		} else {
			setConnectable();
		}

		restartAdvertising();

//		err_code = sd_ble_gap_adv_start(&_adv_params);
//		if (err_code == NRF_ERROR_INVALID_PARAM) {
//			LOGf(MSG_BLE_ADVERTISEMENT_CONFIG_INVALID);
//		}
//		APP_ERROR_CHECK(err_code);
	}
}

bool Nrf51822BluetoothStack::isAdvertising() {
	return _advertising;
}

void Nrf51822BluetoothStack::setAdvertisementData() {
//      Why disabled?
//	if (!_advertising)
//		return;
	if (!_initializedRadio) {
		APP_ERROR_CHECK(1);
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}

	uint32_t err;
	err = ble_advdata_set(&_advdata, &_scan_resp);

	//! TODO: why do we allow (and get) invalid configuration?
	//! It seems like we setAdvertisementData before we configure as iBeacon (probably from the service data?)
	switch(err) {
	//case NRF_ERROR_INVALID_PARAMETER:
	case NRF_SUCCESS:
		break;
	case NRF_ERROR_INVALID_PARAM:
		LOGw("Invalid advertisement configuration");
		// TODO: why not APP_ERROR_CHECK?
		break;
	default:
		LOGd("Retry setAdvData (err %d)", err);
	//	BLE_CALL(sd_ble_gap_adv_stop, ());
		BLE_CALL(ble_advdata_set, (&_advdata, &_scan_resp));
	//	BLE_CALL(sd_ble_gap_adv_start, (&adv_params));
	}
}

void Nrf51822BluetoothStack::startScanning() {
	if (!_initializedRadio) {
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}
	if (_scanning)
		return;

	LOGi(FMT_START, "scanning");
	ble_gap_scan_params_t p_scan_params;
	//! No devices in whitelist, hence non selective performed.
	p_scan_params.active = 1;            //! Active scanning set.
	p_scan_params.selective = 0;            //! Selective scanning not set.
	p_scan_params.p_whitelist = NULL;         //! No whitelist provided.
	p_scan_params.timeout = 0x0000;       //! No timeout.

	Settings::getInstance().get(CONFIG_SCAN_INTERVAL, &p_scan_params.interval);
	Settings::getInstance().get(CONFIG_SCAN_WINDOW, &p_scan_params.window);

	//! todo: which fields to set here?
	BLE_CALL(sd_ble_gap_scan_start, (&p_scan_params));
	_scanning = true;

	EventDispatcher::getInstance().dispatch(EVT_SCAN_STARTED);
}


void Nrf51822BluetoothStack::stopScanning() {
	if (!_initializedRadio) {
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}
	if (!_scanning)
		return;

	LOGi(FMT_STOP, "scanning");
	BLE_CALL(sd_ble_gap_scan_stop, ());
	_scanning = false;

	EventDispatcher::getInstance().dispatch(EVT_SCAN_STOPPED);
}

bool Nrf51822BluetoothStack::isScanning() {
	return _scanning;
}

//Nrf51822BluetoothStack& Nrf51822BluetoothStack::onRadioNotificationInterrupt(
//		uint32_t distanceUs, callback_radio_t callback) {
//	_callback_radio = callback;
//
//	nrf_radio_notification_distance_t distance = NRF_RADIO_NOTIFICATION_DISTANCE_NONE;
//
//	if (distanceUs >= 5500) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_5500US;
//	} else if (distanceUs >= 4560) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_4560US;
//	} else if (distanceUs >= 3620) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_3620US;
//	} else if (distanceUs >= 2680) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_2680US;
//	} else if (distanceUs >= 1740) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_1740US;
//	} else if (distanceUs >= 800) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_800US;
//	}
//
//	uint32_t result = sd_nvic_SetPriority(SWI1_IRQn, NRF_APP_PRIORITY_LOW);
//	BLE_THROW_IF(result, "Could not set radio notification IRQ priority.");
//
//	result = sd_nvic_EnableIRQ(SWI1_IRQn);
//	BLE_THROW_IF(result, "Could not enable radio notification IRQ.");
//
//	result = sd_radio_notification_cfg_set(
//			distance == NRF_RADIO_NOTIFICATION_DISTANCE_NONE ?
//					NRF_RADIO_NOTIFICATION_TYPE_NONE :
//					NRF_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH, distance);
//	BLE_THROW_IF(result, "Could not configure radio notification.");
//
//	return *this;
//}

//extern "C" {
//	void SWI1_IRQHandler() { //! radio notification IRQ handler
//		uint8_t radio_notify;
//		Nrf51822BluetoothStack::_stack->_radio_notify = radio_notify = ( Nrf51822BluetoothStack::_stack->_radio_notify + 1) % 2;
//		Nrf51822BluetoothStack::_stack->_callback_radio(radio_notify == 1);
//	}
//
//	void SWI2_IRQHandler(void) { //! sd event IRQ handler
//		//! do nothing.
//	}
//}

//void Nrf51822BluetoothStack::tick() {
//
//#if TICK_CONTINUOUSLY==0
//#if(NORDIC_SDK_VERSION > 5)
//	BLE_CALL(sd_app_evt_wait, ());
//#else
//	BLE_CALL(sd_app_event_wait, () );
//#endif
//#endif
//	while (1) {
//
//		//!        uint8_t nested;
//		//!        sd_nvic_critical_region_enter(&nested);
//		//!        uint8_t radio_notify = _radio_notify = (radio_notify + 1) % 4;
//		//!        sd_nvic_critical_region_exit(nested);
//		//!        if ((radio_notify % 2 == 0) && _callback_radio) {
//		//!            _callback_radio(radio_notify == 0);
//		//!        }
//
//		uint16_t evt_len = _evt_buffer_size;
//		uint32_t err_code = sd_ble_evt_get(_evt_buffer, &evt_len);
//		if (err_code == NRF_ERROR_NOT_FOUND) {
//			break;
//		}
//		BLE_THROW_IF(err_code, "Error retrieving bluetooth event from stack.");
//
//		on_ble_evt((ble_evt_t *) _evt_buffer);
//
//	}
//}

void Nrf51822BluetoothStack::setPinEncrypted(bool encrypted) {
	if (encrypted) {
		//! initialize device manager to handle PIN encryption keys
		device_manager_init(false);
	}

	//! set characteristics of all services to encrypted
	for (Service* svc : _services) {
		svc->setPinEncrypted(encrypted);
	}
}

void Nrf51822BluetoothStack::setAesEncrypted(bool encrypted) {

	//! set characteristics of all services to encrypted
	for (Service* svc : _services) {
		svc->setAesEncrypted(encrypted);
	}
}

void Nrf51822BluetoothStack::setPasskey(uint8_t* passkey) {
#ifdef PRINT_STACK_VERBOSE
	LOGd(FMT_SET_STR_VAL, "passkey", std::string((char*)passkey, BLE_GAP_PASSKEY_LEN).c_str());
#endif

	memcpy(_passkey, passkey, BLE_GAP_PASSKEY_LEN);

	if (_initializedRadio) {
		updatePasskey();
	}
}

void Nrf51822BluetoothStack::updatePasskey() {
	if (!_initializedRadio) {
		LOGw(FMT_NOT_INITIALIZED, "radio");
		return;
	}
	ble_opt_t static_pin_option;
	static_pin_option.gap_opt.passkey.p_passkey = _passkey;
	BLE_CALL(sd_ble_opt_set, (BLE_GAP_OPT_PASSKEY, &static_pin_option));
}

/** Function for handling the Device Manager events.
 *
 * @param[in]   p_evt   Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const    * p_handle,
                                           dm_event_t const     * p_event,
                                           ret_code_t           event_result)
{
	return Nrf51822BluetoothStack::getInstance().deviceManagerEvtHandler(p_handle, p_event, event_result);
}

//#define SECURITY_REQUEST_DELAY          APP_TIMER_TICKS(4000, APP_TIMER_PRESCALER)  /*< Delay after connection until Security Request is sent, if necessary (ticks). */

void Nrf51822BluetoothStack::lowPowerTimeout(void* p_context) {
	LOGw("bonding timeout!");
	((Nrf51822BluetoothStack*)p_context)->changeToNormalTxPowerMode();
}

void Nrf51822BluetoothStack::changeToLowTxPowerMode() {
	int8_t lowTxPower;
	Settings::getInstance().get(CONFIG_LOW_TX_POWER, &lowTxPower);
	setTxPowerLevel(lowTxPower);
}

void Nrf51822BluetoothStack::changeToNormalTxPowerMode() {
	int8_t txPower;
	Settings::getInstance().get(CONFIG_TX_POWER, &txPower);
	setTxPowerLevel(txPower);
}

uint32_t Nrf51822BluetoothStack::deviceManagerEvtHandler(dm_handle_t const    * p_handle,
                                           dm_event_t const     * p_event,
										   ret_code_t           event_result)
{
//	uint32_t err_code;
//	LOGd("deviceManagerEvtHandler: %p", p_event->event_id);

// todo: why was this here?
//    if (event_result != BLE_GAP_SEC_STATUS_SUCCESS) {
//    	LOGe("[SECURITY ERROR] bonding failed with code: %d", event_result);
//    	sd_ble_gap_disconnect(p_event->event_param.p_gap_param->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
////!    	return NRF_ERROR_INTERNAL;
//    }

//	LOGi("event_id: %d, event_result: %d", p_event->event_id, event_result);

    switch (p_event->event_id)
    {
        case DM_EVT_CONNECTION:
//        	LOGi("DM_EVT_CONNECTION");
            //! Start Security Request timer.
//            if (p_handle->device_id != DM_INVALID_ID)
//            {
//!            	  LOGi("start sec timer");
//!                err_code = app_timer_start(m_sec_req_timer_id, SECURITY_REQUEST_DELAY, NULL);
//!                APP_ERROR_CHECK(err_code);
//            }
        	_peerHandle = (*p_handle);
        	Timer::getInstance().start(_secReqTimerId, MS_TO_TICKS(SECURITY_REQUEST_DELAY), NULL);
            break;
        case DM_EVT_SECURITY_SETUP:
        case DM_EVT_SECURITY_SETUP_REFRESH: {
        	// [15.8.2016] ALEX: low tx will be set on boot not per default.

//        	LOGi("going into low power mode for bonding ...");

        	//! schedule timeout
//        	Timer::getInstance().createSingleShot(_lowPowerTimeoutId, lowPowerTimeout);
//        	Timer::getInstance().start(_lowPowerTimeoutId, MS_TO_TICKS(60000), this);


//        	changeToLowTxPowerMode();
        	break;
        }
        case DM_EVT_SECURITY_SETUP_COMPLETE: {
        	if (event_result == NRF_SUCCESS) {
        		LOGi("bonding completed");
        	} else {
        		LOGe("bonding failed with error: %d (%p)", event_result, event_result);
        		disconnect();
        	}

        	// [15.8.2016] ALEX: we are in low tx for a reason. A single shot security
        	// pass should not remove this. On disconnect it will remain high TX
//        	changeToNormalTxPowerMode();

        	//! clear timeout
 //       	Timer::getInstance().stop(_lowPowerTimeoutId);
        	break;
        }
        default:
            break;
    }
    return NRF_SUCCESS;
}

/**@brief Function for handling the security request timer time-out.
 *
 * @details This function is called each time the security request timer expires.
 *
 * @param[in] p_context  Pointer used for passing context information from the
 *                       app_start_timer() call to the time-out handler.
 */
void Nrf51822BluetoothStack::secReqTimeoutHandler(void * p_context)
{
    uint32_t             err_code;
    dm_security_status_t status;

    if (_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        err_code = dm_security_status_req(&_peerHandle, &status);
        APP_ERROR_CHECK(err_code);

        // If the link is still not secured by the peer, initiate security procedure.
        if (status == NOT_ENCRYPTED)
        {
            err_code = dm_security_setup_req(&_peerHandle);
            APP_ERROR_CHECK(err_code);
        }
    }
}

static void sec_req_timeout_handler(void * p_context) {
	Nrf51822BluetoothStack::getInstance().secReqTimeoutHandler(p_context);
}

void Nrf51822BluetoothStack::device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_data;
    dm_application_param_t register_param;

    //! Don't clear bonded centrals
    init_data.clear_persistent_data = erase_bonds;

    err_code = dm_init(&init_data);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    //! Using static pin:
    register_param.sec_param.mitm    = SEC_PARAM_MITM;
    register_param.sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;

    err_code = dm_register(&_dm_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);

    // create a timer to handle the security request timeout
    Timer::getInstance().createSingleShot(_secReqTimerId, sec_req_timeout_handler);

    _dm_initialized = true;

//    LOGi("device_manager_init");
}

void Nrf51822BluetoothStack::device_manager_reset() {
	uint32_t err_code;
	err_code = dm_device_delete_all(&_dm_app_handle);
	APP_ERROR_CHECK(err_code);
}

void Nrf51822BluetoothStack::on_ble_evt(ble_evt_t * p_ble_evt) {
//	if (p_ble_evt->header.evt_id != BLE_GAP_EVT_RSSI_CHANGED) {
//		LOGi("on_ble_event: 0x%X", p_ble_evt->header.evt_id);
//	}

//    if (_dm_initialized && Settings::getInstance().isEnabled(CONFIG_ENCRYPTION_ENABLED)) {
	if (_dm_initialized) {
    	dm_ble_evt_handler(p_ble_evt);
	}
//    }

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
//		_log(SERIAL_INFO, "address: ");
//		BLEutil::printArray(p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr, BLE_GAP_ADDR_LEN);
		on_connected(p_ble_evt);
		EventDispatcher::getInstance().dispatch(EVT_BLE_CONNECT);
		break;

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
		//! nothing to do
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		on_disconnected(p_ble_evt);
		EventDispatcher::getInstance().dispatch(EVT_BLE_DISCONNECT);
		break;

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

		} else {
			for (Service* svc : _services) {
				// TODO: this check is probably wrong
//				if (svc->getHandle() == p_ble_evt->evt.gatts_evt.params.write.handle) {
				//! TODO: better way of selection?
				if (true) { //! Just let every service check for now...
					svc->on_write(p_ble_evt->evt.gatts_evt.params.write, p_ble_evt->evt.gatts_evt.params.write.handle);
//					return;
				}
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

	case BLE_GAP_EVT_ADV_REPORT:
		EventDispatcher::getInstance().dispatch(EVT_BLE_EVENT, p_ble_evt, -1);
		break;
	case BLE_GAP_EVT_TIMEOUT:
		if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISING) {
			_advertising = false; //! Advertising stops, see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
		}
		break;

	case BLE_EVT_TX_COMPLETE:
//		LOGi("BLE_EVT_TX_COMPLETE");
		onTxComplete(p_ble_evt);
		break;

	default:
		break;

	}
}

#if CONNECTION_ALIVE_TIMEOUT>0
static void connection_keep_alive_timeout(void* p_context) {
	LOGw("connection keep alive timeout!");
	Nrf51822BluetoothStack::getInstance().disconnect();
}
#endif

void Nrf51822BluetoothStack::startConnectionAliveTimer() {
#if CONNECTION_ALIVE_TIMEOUT>0
	Timer::getInstance().createSingleShot(_connectionKeepAliveTimerId, connection_keep_alive_timeout);
	Timer::getInstance().start(_connectionKeepAliveTimerId, MS_TO_TICKS(CONNECTION_ALIVE_TIMEOUT), NULL);
#endif
}

void Nrf51822BluetoothStack::stopConnectionAliveTimer() {
#if CONNECTION_ALIVE_TIMEOUT>0
	Timer::getInstance().stop(_connectionKeepAliveTimerId);
#endif
}

void Nrf51822BluetoothStack::resetConnectionAliveTimer() {
#if CONNECTION_ALIVE_TIMEOUT>0
	Timer::getInstance().reset(_connectionKeepAliveTimerId, MS_TO_TICKS(CONNECTION_ALIVE_TIMEOUT), NULL);
#endif
}

void Nrf51822BluetoothStack::on_connected(ble_evt_t * p_ble_evt) {
	//ble_gap_evt_connected_t connected_evt = p_ble_evt->evt.gap_evt.params.connected;
	_advertising = false; //! Advertising stops on connect, see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
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

void Nrf51822BluetoothStack::on_disconnected(ble_evt_t * p_ble_evt) {
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

void Nrf51822BluetoothStack::disconnect() {
	//! Only disconnect when we are actually connected to something
	if (_conn_handle != BLE_CONN_HANDLE_INVALID && _disconnectingInProgress == false) {
		_disconnectingInProgress = true;
		LOGi("Forcibly disconnecting from device");
		//! It seems like we're only allowed to use BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION.
		//! This sometimes gives us an NRF_ERROR_INVALID_STATE (disconnection is already in progress)
		//! NRF_ERROR_INVALID_STATE can safely be ignored, see: https://devzone.nordicsemi.com/question/81108/handling-nrf_error_invalid_state-error-code/
		uint32_t errorCode = sd_ble_gap_disconnect(_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		if (errorCode != NRF_ERROR_INVALID_STATE) {
			APP_ERROR_CHECK(errorCode);
		}
	}
}

bool Nrf51822BluetoothStack::isDisconnecting() {
		return _disconnectingInProgress;
};

bool Nrf51822BluetoothStack::isConnected() {
	return _conn_handle != BLE_CONN_HANDLE_INVALID;
}

void Nrf51822BluetoothStack::onTxComplete(ble_evt_t * p_ble_evt) {
	for (Service* svc: _services) {
		svc->onTxComplete(&p_ble_evt->evt.common_evt);
	}
}



