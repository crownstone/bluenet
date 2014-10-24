/**
 * @author Christopher Mason
 * @author Anne van Rossum
 */
#include "BluetoothLE.h"
#include "nRF51822.h"

#if(NORDIC_SDK_VERSION >= 6)
#include "nrf_soc.h"
#endif

#ifndef SOFTDEVICE_SERIES
#error "The SOFTDEVICE_SERIES macro is required for compilation. Set it to 110 for example"
#endif

#include "ble_error.h"

#include "log.h"

using namespace BLEpp;

/**@brief Variable length data encapsulation in terms of length and pointer to data */

typedef struct
{
    uint8_t               * p_data;          /**< Pointer to data. */
    uint16_t                data_len;        /**< Length of data. */
} data_t;


/// UUID //////////////////////////////////////////////////////////////////////////////////////////

UUID::UUID(const char* fullUid) : _full(fullUid), _type(BLE_UUID_TYPE_UNKNOWN) {}

uint16_t UUID::init() {

	if (_full && _type == BLE_UUID_TYPE_UNKNOWN) {
		ble_uuid128_t u;

		int i = 0;
		int j = 0;
		int k = 0;
		uint8_t c;
		uint8_t v = 0;
		for (; ((c = _full[i]) != 0) && (j < 16); i++) {
			uint8_t vv = 0;

			if (c == '-' || c == ' ') {
				continue;
			} else if (c >= '0' && c <= '9') {
				vv = c - '0';
			} else if (c >= 'A' && c <= 'F') {
				vv = c - 'A' + 10;
			} else if (c >= 'a' && c <= 'f') {
				vv = c - 'a' + 10;
			} else {
				char cc[] = {c};// can't just call std::string(c) apparently.
				BLE_THROW(std::string("Invalid character ") + std::string(1,cc[0]) + " in UUID.");
			}

			v = v * 16 + vv;

			if (k++ % 2 == 1) {
				u.uuid128[15 - (j++)] = v;
				v = 0;
			}

		}
		if (j < 16) {
			BLE_THROW("UUID is too short,");
		} else if (_full[i] != 0) {
			BLE_THROW("UUID is too long.");
		}

		ble_uuid_t uu;

		uint32_t error_code = sd_ble_uuid_decode (16, u.uuid128, &uu);

		if (error_code == NRF_ERROR_NOT_FOUND) {
			BLE_CALL( sd_ble_uuid_vs_add, (&u, &_type) );

			_uuid = (uint16_t)(u.uuid128[13] << 8 | u.uuid128[12]);
		} else if (error_code == NRF_SUCCESS) {
			_type = uu.type;
			_uuid = uu.uuid;
		} else {
			BLE_THROW("Failed to add uuid.");
		}
	} else if (_type == BLE_UUID_TYPE_UNKNOWN) {
		BLE_THROW("TODO generate random UUID");
	} else {
		// nothing to do.
	}
	return _type;
}

/// Characteristic /////////////////////////////////////////////////////////////////////////////////////////////////////


void set_attr_md_read_only(ble_gatts_attr_md_t& md) {
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&md.write_perm);
	md.vloc = BLE_GATTS_VLOC_STACK;
	md.rd_auth    = 0;  // don't request read access every time a read is attempted.
	md.wr_auth    = 0;  // ditto for write.
	md.vlen       = 1;  // variable length.
}

void set_attr_md_read_write(ble_gatts_attr_md_t& md) {
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.write_perm);
	md.vloc = BLE_GATTS_VLOC_STACK;
	md.rd_auth    = 0;  // don't request read access every time a read is attempted.
	md.wr_auth    = 0;  // ditto for write.
	md.vlen       = 1;  // variable length.
}

CharacteristicBase::CharacteristicBase()
	:
		_handles({}), _service(0), _inited(false), _notifies(false), _writable(false), _unit(0), _updateIntervalMsecs(0) {


		}


CharacteristicBase& CharacteristicBase::setName(const std::string& name) {
	if (_inited) BLE_THROW("Already inited.");
	_name = name;


	return *this;
}

CharacteristicBase& CharacteristicBase::setUnit(uint16_t unit) {
	_unit = unit;
	return *this;
}

//CharacteristicBase& CharacteristicBase::setUpdateIntervalMSecs(uint32_t msecs) {
//    if (_updateIntervalMsecs != msecs) {
//        _updateIntervalMsecs = msecs;
//        // TODO schedule task.
//        app_timer_create(&_timer, APP_TIMER_MODE_REPEATED, [](void* context){
//            CharacteristicBase* characteristic = (CharacteristicBase*)context;
//            characteristic->read();
//        });
//        app_timer_start(_timer, (uint32_t)(_updateIntervalMsecs / 1000. * (32768 / NRF51_RTC1_PRESCALER +1)), this);
//    }
//    return *this;
//}


void CharacteristicBase::init(Service* svc) {
	_service = svc;

	CharacteristicInit ci;

	// by default:

	ci.char_md.char_props.read = 1; // allow read
	ci.char_md.char_props.notify = 1; // allow notification
	ci.char_md.char_props.broadcast = 0; // allow broadcast
	ci.char_md.char_props.indicate = 0; // allow indication


	// initially readable but not writeable
	set_attr_md_read_write(ci.cccd_md);
	set_attr_md_read_only(ci.sccd_md);
	set_attr_md_read_only(ci.attr_md);


	// these characteristic descriptors are optional, and I gather, not really used by anything.
	// we fill them in if the user specifies any of the data (eg name).
	ci.char_md.p_char_user_desc  = NULL;
	ci.char_md.p_char_pf         = NULL;
	ci.char_md.p_user_desc_md    = NULL;
	ci.char_md.p_cccd_md         = &ci.cccd_md; // the client characteristic metadata.
	//  _char_md.p_sccd_md         = &_sccd_md;

	_uuid.init();
	ble_uuid_t uid = _uuid;

	CharacteristicValue initialValue = getCharacteristicValue();

	ci.attr_char_value.p_attr_md    = &ci.attr_md;

	ci.attr_char_value.init_offs    = 0;
	ci.attr_char_value.init_len     = initialValue.length;
	ci.attr_char_value.max_len      = getValueMaxLength();
	ci.attr_char_value.p_value      = initialValue.length > 0 ? (uint8_t*) initialValue.data : 0;

	ci.attr_char_value.p_uuid       = &uid;

	setupWritePermissions(ci);

	if (!_name.empty()) {
		ci.char_md.p_char_user_desc = (uint8_t*)_name.c_str(); // todo utf8 conversion?
		ci.char_md.char_user_desc_size = _name.length();
		ci.char_md.char_user_desc_max_size = _name.length();
	}


	// This is the metadata (eg security settings) for the description of this characteristic.
	ci.char_md.p_user_desc_md = &ci.user_desc_metadata_md;

	set_attr_md_read_only(ci.user_desc_metadata_md);

	this->configurePresentationFormat(ci.presentation_format);
	ci.presentation_format.unit = _unit;
	ci.char_md.p_char_pf = &ci.presentation_format;

	volatile uint16_t svc_handle = svc->getHandle();

	volatile uint32_t err_code = sd_ble_gatts_characteristic_add(svc_handle,&ci.char_md, &ci.attr_char_value, &_handles);
	APP_ERROR_CHECK(err_code);

	_inited = true;
}

void CharacteristicBase::setupWritePermissions(CharacteristicInit& ci) {
	ci.attr_md.write_perm.sm = _writable ? 1 : 0;
	ci.attr_md.write_perm.lv = _writable ? 1 : 0;
	ci.char_md.char_props.write = _writable ? 1 : 0;
	ci.char_md.char_props.write_wo_resp = _writable ? 1 : 0;
	ci.char_md.char_props.notify = _notifies ? 1 : 0;
	ci.char_md.char_props.indicate = _notifies ? 1 : 0;
	ci.attr_md.write_perm = _writeperm;
	ci.char_md.char_props.write = ci.cccd_md.write_perm.lv > 0 ? 1 :0;
	ci.char_md.char_props.write_wo_resp = ci.cccd_md.write_perm.lv > 0 ? 1 :0;
}

void CharacteristicBase::notify() {

	CharacteristicValue value = getCharacteristicValue();

	BLE_CALL(sd_ble_gatts_value_set, (_handles.value_handle, 0, &value.length, value.data) );

	if ((!_notifies) || (!_service->getStack()->connected()) ) return;

	ble_gatts_hvx_params_t hvx_params;
	uint16_t len = value.length;

	hvx_params.handle   = _handles.value_handle;
	hvx_params.type     = BLE_GATT_HVX_NOTIFICATION;
	hvx_params.offset   = 0;
	hvx_params.p_len    = &len;
	hvx_params.p_data   = (uint8_t*)value.data;

	BLE_CALL(sd_ble_gatts_hvx, (_service->getStack()->getConnectionHandle(), &hvx_params) );

}

/// Service ////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* Service::defaultServiceName = "unnamed";

void Service::start(BLEStack* stack) {

	_stack = stack;

	uint32_t err_code;

	_uuid.init();
	const ble_uuid_t uuid = _uuid;

	_service_handle = BLE_CONN_HANDLE_INVALID;
	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, (uint16_t*)&_service_handle);
	APP_ERROR_CHECK(err_code);

	for(CharacteristicBase* characteristic : getCharacteristics()) {
		characteristic->init(this);
	}

	_started = true;

}

void Service::on_ble_event(ble_evt_t * p_ble_evt) {
	switch (p_ble_evt->header.evt_id)
	{
		case BLE_GAP_EVT_CONNECTED:
			on_connect(p_ble_evt->evt.gap_evt.conn_handle, p_ble_evt->evt.gap_evt.params.connected);
			break;

		case BLE_GAP_EVT_DISCONNECTED:
			on_disconnect(p_ble_evt->evt.gap_evt.conn_handle, p_ble_evt->evt.gap_evt.params.disconnected);
			break;

		case BLE_GATTS_EVT_WRITE:
			on_write(p_ble_evt->evt.gatts_evt.params.write);
			break;

		default:
			break;
	}
}


void Service::on_connect(uint16_t conn_handle, ble_gap_evt_connected_t& gap_evt) {
	// nothing here yet.
}

void Service::on_disconnect(uint16_t conn_handle, ble_gap_evt_disconnected_t& gap_evt) {
	// nothing here yet.
}

void Service::on_write(ble_gatts_evt_write_t& write_evt) {
	bool found = false;



	for(CharacteristicBase* characteristic : getCharacteristics()) {

		// TODO: make a map.
		if (characteristic->getValueHandle() == write_evt.context.value_handle) {
			found = true;

			if (write_evt.op == BLE_GATTS_OP_WRITE_REQ || write_evt.op == BLE_GATTS_OP_WRITE_CMD || write_evt.op == BLE_GATTS_OP_SIGN_WRITE_CMD) {
				characteristic->written(write_evt.len, write_evt.offset, write_evt.data);
			} else {
				found = false;
			}

		}

	}

	if (!found) {
		// tell someone?
	}
}


/// Nrf51822BluetoothStack /////////////////////////////////////////////////////////////////////////////////////////////

Nrf51822BluetoothStack::Nrf51822BluetoothStack(Pool& pool) :
	_pool(pool),
	_appearance(defaultAppearance),
	_clock_source(defaultClockSource),
	_mtu_size(defaultMtu),
	_tx_power_level(defaultTxPowerLevel),
	_sec_mode({}),
	//_adv_params({}),
	_interval(defaultAdvertisingInterval_0_625_ms),
	_timeout(defaultAdvertisingTimeout_seconds),
	_gap_conn_params({}),

	_inited(false),
	_started(false),
	_advertising(false),
	_scanning(false),

	_conn_handle(BLE_CONN_HANDLE_INVALID),

	_radio_notify(0)

{
	if (_stack) BLE_THROW("Can't have more than one Nrf51822BluetoothStack");
	_stack = this;

	_evt_buffer_size = sizeof(ble_evt_t) + (_mtu_size) * sizeof(uint32_t);
	_evt_buffer = (uint8_t*)malloc(_evt_buffer_size);

	//if (!is_word_aligned(_evt_buffer)) throw ble_exception("Event buffer not aligned.");

	// setup default values.

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&_sec_mode);

	_gap_conn_params.min_conn_interval = defaultMinConnectionInterval_1_25_ms;
	_gap_conn_params.max_conn_interval = defaultMaxConnectionInterval_1_25_ms;
	_gap_conn_params.slave_latency     = defaultSlaveLatencyCount;
	_gap_conn_params.conn_sup_timeout  = defaultConnectionSupervisionTimeout_10_ms;

}


Nrf51822BluetoothStack::~Nrf51822BluetoothStack() {
	shutdown();

	if (_evt_buffer) free(_evt_buffer);
}

Nrf51822BluetoothStack& Nrf51822BluetoothStack::init() {

	if (_inited) return *this;

	// Initialise SoftDevice
	BLE_CALL(sd_softdevice_enable, (_clock_source, softdevice_assertion_handler) );

	// enable the BLE stack
#if(NORDIC_SDK_VERSION >= 6)

#if(SOFTDEVICE_SERIES == 110) 
	// do not define the service_changed characteristic, of course allow future changes
	#define IS_SRVC_CHANGED_CHARACT_PRESENT  1
	ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));
	ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
	BLE_CALL(sd_ble_enable, (&ble_enable_params) );
#endif
#endif

	// according to the migration guide the address needs to be set directly after the sd_ble_enable call
	// due to "an issue present in the s110_nrf51822_7.0.0 release
#if(NORDIC_SDK_VERSION >= 7)
	BLE_CALL(sd_ble_gap_enable, () );
	ble_gap_addr_t addr;
	BLE_CALL(sd_ble_gap_address_get, (&addr) );
	BLE_CALL(sd_ble_gap_address_set, (BLE_GAP_ADDR_CYCLE_MODE_NONE, &addr) );
#endif

	ble_version_t version({});
	version.company_id = 12;
	BLE_CALL(sd_ble_version_get, (&version) );

	sd_nvic_EnableIRQ(SWI2_IRQn);

	// set device name
	if (!_device_name.empty()) {
		BLE_CALL(sd_ble_gap_device_name_set, (&_sec_mode, (uint8_t*) _device_name.c_str(), _device_name.length()) );
		//BLE_CALL(sd_ble_gap_device_name_set, (&_sec_mode, DEVICE_NAME, strlen(DEVICE_NAME)));
	} else {
		std::string name = "not set...";
		BLE_CALL(sd_ble_gap_device_name_set, (&_sec_mode, (uint8_t*) name.c_str(), name.length()) );
	}
	BLE_CALL(sd_ble_gap_appearance_set, (_appearance) );

	setConnParams();

	setTxPowerLevel();

	_inited = true;

	return *this;
}

Nrf51822BluetoothStack& Nrf51822BluetoothStack::start() {
	if (_started) return *this;

	for(Service* svc : _services) {
		svc->start(this);
	}

	_started = true;

	return *this;
}


Nrf51822BluetoothStack& Nrf51822BluetoothStack::shutdown() {
	stopAdvertising();

	for(Service* svc : _services) {
		svc->stop();
	}

	// TODO: shutdown soft device.

	_inited = false;

	return *this;
}

Nrf51822BluetoothStack& Nrf51822BluetoothStack::addService(Service* svc) {
	//APP_ERROR_CHECK_BOOL(_services.size() < MAX_SERVICE_COUNT);

	_services.push_back(svc);

	return *this;
}

void Nrf51822BluetoothStack::setTxPowerLevel() {
	BLE_CALL(sd_ble_gap_tx_power_set, (_tx_power_level) );
}


void Nrf51822BluetoothStack::setConnParams() {
	BLE_CALL(sd_ble_gap_ppcp_set, (&_gap_conn_params) );
}

Nrf51822BluetoothStack& Nrf51822BluetoothStack::setTxPowerLevel(int8_t powerLevel) {
	if (_tx_power_level != powerLevel) {
		_tx_power_level = powerLevel;
		if (_inited) setTxPowerLevel();

		//        if (_advertising) {
		//            stopAdvertising();
		//            startAdvertising();
		//        }
	}
}

Nrf51822BluetoothStack& Nrf51822BluetoothStack::startAdvertising() {
	if (_advertising) return *this;

	init(); // we should already be.

	start();

	uint32_t      err_code __attribute__((unused));
	ble_advdata_t advdata;
	uint8_t       flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	uint8_t uidCount = _services.size();
	ble_uuid_t adv_uuids[uidCount];

	uint8_t cnt = 0;
	for(Service* svc : _services ) {
		adv_uuids[cnt++] = svc->getUUID();
	}

	if (cnt == 0) {
		BLE_THROW("No services.");
	}

	ble_gap_adv_params_t adv_params;

	adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
	adv_params.p_peer_addr = NULL;                           // Undirected advertisement
	adv_params.fp          = BLE_GAP_ADV_FP_ANY;
	adv_params.interval    = _interval;
	adv_params.timeout     = _timeout;

	// Build and set advertising data
	memset(&advdata, 0, sizeof(advdata));

	//advdata.name_type               = BLE_ADVDATA_NO_NAME;

	// Anne: setting NO_NAME breaks the Android Nordic nRF Master Console app.
	advdata.name_type               = BLE_ADVDATA_FULL_NAME;
	advdata.short_name_len          = 5;
	advdata.include_appearance      = _appearance != BLE_APPEARANCE_UNKNOWN;
	advdata.p_tx_power_level        = &_tx_power_level;
	advdata.flags.size              = sizeof(flags);
	advdata.flags.p_data            = &flags;
	advdata.uuids_complete.uuid_cnt = 1;
	advdata.uuids_complete.p_uuids  = adv_uuids;

	BLE_CALL(ble_advdata_set, (&advdata, NULL) );



	BLE_CALL(sd_ble_gap_adv_start, (&adv_params) );

	_advertising = true;

	return *this;
}

Nrf51822BluetoothStack& Nrf51822BluetoothStack::stopAdvertising() {
	if (!_advertising) return *this;

	BLE_CALL(sd_ble_gap_adv_stop, () );

	_advertising = false;

	return *this;
}


#if(SOFTDEVICE_SERIES != 110)

#define SCAN_INTERVAL                    0x00A0         /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                      0x0050         /**< Determines scan window in units of 0.625 millisecond. */


/**
 * Only call the following functions with a S120 or S130 device that can play a central role. The following functions
 * are probably the ones your recognize from implementing BLE functionality on Android or iOS if you are a smartphone
 * developer.
 */
Nrf51822BluetoothStack& Nrf51822BluetoothStack::startScanning() {
	if (_scanning) return *this;
	ble_gap_scan_params_t p_scan_params;
	// No devices in whitelist, hence non selective performed.
	p_scan_params.active       = 0;            // Active scanning set.
	p_scan_params.selective    = 0;            // Selective scanning not set.
	p_scan_params.interval     = SCAN_INTERVAL;// Scan interval.
	p_scan_params.window       = SCAN_WINDOW;  // Scan window.
	p_scan_params.p_whitelist  = NULL;         // No whitelist provided.
	p_scan_params.timeout      = 0x0000;       // No timeout.

	// todo: which fields to set here?
	BLE_CALL(sd_ble_gap_scan_start, (&p_scan_params) );
	_scanning = true;
	return *this;
}

Nrf51822BluetoothStack& Nrf51822BluetoothStack::stopScanning() {
	if (!_scanning) return *this;
	BLE_CALL(sd_ble_gap_scan_stop, () );
	_scanning = false;
	return *this;
}
#endif

Nrf51822BluetoothStack& Nrf51822BluetoothStack::onRadioNotificationInterrupt(uint32_t distanceUs, callback_radio_t callback) {
	_callback_radio = callback;

	nrf_radio_notification_distance_t distance = NRF_RADIO_NOTIFICATION_DISTANCE_NONE;

	if (distanceUs >= 5500) {
		distance = NRF_RADIO_NOTIFICATION_DISTANCE_5500US;
	} else if (distanceUs >= 4560) {
		distance = NRF_RADIO_NOTIFICATION_DISTANCE_4560US;
	} else if (distanceUs >= 3620) {
		distance = NRF_RADIO_NOTIFICATION_DISTANCE_3620US;
	} else if (distanceUs >= 2680) {
		distance = NRF_RADIO_NOTIFICATION_DISTANCE_2680US;
	} else if (distanceUs >= 1740) {
		distance = NRF_RADIO_NOTIFICATION_DISTANCE_1740US;
	} else if (distanceUs >= 800) {
		distance = NRF_RADIO_NOTIFICATION_DISTANCE_800US;
	}

	uint32_t result = sd_nvic_SetPriority(SWI1_IRQn, NRF_APP_PRIORITY_LOW);

	BLE_THROW_IF(result, "Could not set radio notification IRQ priority.");

	result = sd_nvic_EnableIRQ(SWI1_IRQn);

	BLE_THROW_IF(result, "Could not enable radio notification IRQ.");

	result = sd_radio_notification_cfg_set(
			distance == NRF_RADIO_NOTIFICATION_DISTANCE_NONE
			? NRF_RADIO_NOTIFICATION_TYPE_NONE : NRF_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH,
			distance);

	BLE_THROW_IF(result, "Could not configure radio notification.");

	return *this;
}

extern "C" {
	void SWI1_IRQHandler() { // radio notification IRQ handler
		uint8_t radio_notify;
		Nrf51822BluetoothStack::_stack->_radio_notify = radio_notify = ( Nrf51822BluetoothStack::_stack->_radio_notify + 1) % 2;
		Nrf51822BluetoothStack::_stack->_callback_radio(radio_notify == 1);
	}

	void SWI2_IRQHandler(void) { // sd event IRQ handler
		// do nothing.
	}
}

void Nrf51822BluetoothStack::loop() {

#if(NORDIC_SDK_VERSION > 5)
	BLE_CALL(sd_app_evt_wait, () );
#else
	BLE_CALL(sd_app_event_wait, () );
#endif
	while(1) {

		//        uint8_t nested;
		//        sd_nvic_critical_region_enter(&nested);
		//        uint8_t radio_notify = _radio_notify = (radio_notify + 1) % 4;
		//        sd_nvic_critical_region_exit(nested);
		//        if ((radio_notify % 2 == 0) && _callback_radio) {
		//            _callback_radio(radio_notify == 0);
		//        }

		uint16_t evt_len = _evt_buffer_size;
		uint32_t err_code = sd_ble_evt_get(_evt_buffer, &evt_len);
		if (err_code == NRF_ERROR_NOT_FOUND)
		{
			break;
		}
		BLE_THROW_IF(err_code, "Error retrieving bluetooth event from stack.");

		on_ble_evt((ble_evt_t *)_evt_buffer);

	}
}


void Nrf51822BluetoothStack::on_ble_evt(ble_evt_t * p_ble_evt) {
//	if (p_ble_evt->header.evt_id != BLE_GAP_EVT_RSSI_CHANGED) {
//		LOG_DEBUG("on_ble_event: %X", p_ble_evt->header.evt_id);
//	}
	switch (p_ble_evt->header.evt_id)
	{
		// TODO: how do we identify which service evt is for?

		case BLE_GAP_EVT_CONNECTED:
			on_connected(p_ble_evt);
			break;

		case BLE_GAP_EVT_DISCONNECTED:
			on_disconnected(p_ble_evt);
			break;

		case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
		case BLE_GATTS_EVT_TIMEOUT:
		case BLE_GAP_EVT_RSSI_CHANGED:
			for(Service* svc : _services) {
				svc->on_ble_event(p_ble_evt);
			}
			break;

		case BLE_GATTS_EVT_WRITE:
			for(Service* svc : _services) {
				// TODO use a map...
				if (svc->getHandle() == p_ble_evt->evt.gatts_evt.params.write.context.srvc_handle) {
					svc->on_ble_event(p_ble_evt);
				}
			}
			break;


		case BLE_GATTS_EVT_HVC:
			for(Service* svc : _services) {
				if (svc->getHandle() == p_ble_evt->evt.gatts_evt.params.write.context.srvc_handle) {
					svc->on_ble_event(p_ble_evt);
				}
			}
			break;

		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:

			ble_gap_sec_params_t sec_params;

#if(SOFTDEVICE_SERIES == 110) 
			sec_params.timeout      = 30; // seconds
#endif
			sec_params.bond         = 1;  // perform bonding.
			sec_params.mitm         = 0;  // man in the middle protection not required.
			sec_params.io_caps      = BLE_GAP_IO_CAPS_NONE;  // no display capabilities.
			sec_params.oob          = 0;  // out of band not available.
			sec_params.min_key_size = 7;  // min key size
			sec_params.max_key_size = 16; // max key size.


#if(SOFTDEVICE_SERIES != 110) 
// https://devzone.nordicsemi.com/documentation/nrf51/6.0.0/s120/html/a00527.html#ga7b23027c97b3df21f6cbc23170e55663
			
			// do not store the keys for now...
			//ble_gap_sec_keyset_t sec_keyset;
			//BLE_CALL(sd_ble_gap_sec_params_reply, (p_ble_evt->evt.gap_evt.conn_handle,
			//			BLE_GAP_SEC_STATUS_SUCCESS,
			//			&sec_params, &sec_keyset) );
			BLE_CALL(sd_ble_gap_sec_params_reply, (p_ble_evt->evt.gap_evt.conn_handle,
						BLE_GAP_SEC_STATUS_SUCCESS,
						&sec_params, NULL) );
#else 
			BLE_CALL(sd_ble_gap_sec_params_reply, (p_ble_evt->evt.gap_evt.conn_handle,
						BLE_GAP_SEC_STATUS_SUCCESS,
						&sec_params) );
#endif
			break;
#if(SOFTDEVICE_SERIES != 110) 
		case BLE_GAP_EVT_ADV_REPORT:
		{
			//data_t adv_data;
			//data_t type_data;
			// Initialize advertisement report for parsing.
			const ble_gap_evt_t   * p_gap_evt = &p_ble_evt->evt.gap_evt;
			//adv_data.p_data = (uint8_t *)p_gap_evt->params.adv_report.data;
			//adv_data.data_len = p_gap_evt->params.adv_report.dlen;

			uint8_t * adrs_ptr = (uint8_t*) &(p_gap_evt->params.adv_report.peer_addr.addr);
			char addrs[28];
			sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", 
                                adrs_ptr[5], adrs_ptr[4], adrs_ptr[3],
                                adrs_ptr[2], adrs_ptr[1], adrs_ptr[0]);
			LOG_INFO("Advertisement from: %s", addrs);
		}
			break;
#endif
		case BLE_GAP_EVT_TIMEOUT:
			break;

		default:
			break;

	}
}

void Nrf51822BluetoothStack::on_connected(ble_evt_t * p_ble_evt) {
	//ble_gap_evt_connected_t connected_evt = p_ble_evt->evt.gap_evt.params.connected;
	_advertising = false; // it seems like maybe we automatically stop advertising when we're connected.

	BLE_CALL(sd_ble_gap_conn_param_update, (p_ble_evt->evt.gap_evt.conn_handle, &_gap_conn_params) );

	if (_callback_connected) {
		_callback_connected( p_ble_evt->evt.gap_evt.conn_handle);
	}
	_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
	for(size_t i = 0; i < _services.size(); ++i) {
		Service* svc = _services[i];
		svc->on_ble_event(p_ble_evt);
	}
}

void Nrf51822BluetoothStack::on_disconnected(ble_evt_t * p_ble_evt) {
	//ble_gap_evt_disconnected_t disconnected_evt = p_ble_evt->evt.gap_evt.params.disconnected;
	if (_callback_connected) {
		_callback_disconnected(p_ble_evt->evt.gap_evt.conn_handle);
	}
	_conn_handle = BLE_CONN_HANDLE_INVALID;
	for(Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}

	// Of course this is not nice, but dirty! We immediately start advertising automatically after being 
	// disconnected. But for now this will be the default behaviour.
	startAdvertising();
}


Nrf51822BluetoothStack * Nrf51822BluetoothStack::_stack = 0;
