/**
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 23, 2015
 * License: LGPLv3+
 */

#include "ble/cs_Characteristic.h"

#include <ble/cs_Softdevice.h>

using namespace BLEpp;

CharacteristicBase::CharacteristicBase() :
				_name(NULL),
				_handles( { }), _service(0)
				/*nited(false), _notifies(false), _writable(false), */
				/*_unit(0), */ /*_updateIntervalMsecs(0),*/ /* _notifyingEnabled(false), _indicates(false) */
{
}

CharacteristicBase& CharacteristicBase::setName(const char * const name) {
	if (_status.inited) BLE_THROW(MSG_BLE_CHAR_INITIALIZED);
	_name = name;

	return *this;
}

/* Set the default attributes of every characteristic
 *
 * There are two settings for the location of the memory of the buffer that is used to communicate with the SoftDevice.
 *
 * + BLE_GATTS_VLOC_STACK
 * + BLE_GATTS_VLOC_USER
 *
 * The former makes the SoftDevice allocate the memory (the quantity of which is defined by the user). The latter
 * leaves it to the user. It is essential that with BLE_GATTS_VLOC_USER the calls to sd_ble_gatts_value_set() are
 * handed persistent pointers to a buffer. If a temporary value is used, this will screw up the data read by the user.
 * The same is true if this buffer is reused across characteristics. If it is meant as data for one characteristic
 * and is read through another, this will resolve to nonsense to the user.
 */
void CharacteristicBase::setAttrMdReadOnly(ble_gatts_attr_md_t& md, char vloc) {
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&md.write_perm);
	md.vloc = vloc;
	md.rd_auth = 0; // don't request read access every time a read is attempted.
	md.wr_auth = 0;  // ditto for write.
	md.vlen = 1;  // variable length.
}

void CharacteristicBase::setAttrMdReadWrite(ble_gatts_attr_md_t& md, char vloc) {
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.write_perm);
	md.vloc = vloc;
	md.rd_auth = 0; // don't request read access every time a read is attempted.
	md.wr_auth = 0;  // ditto for write.
	md.vlen = 1;  // variable length.
}

/*
CharacteristicBase& CharacteristicBase::setUnit(uint16_t unit) {
	_unit = unit;
	return *this;
} */

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
	setAttrMdReadWrite(ci.cccd_md, BLE_GATTS_VLOC_STACK);
	setAttrMdReadOnly(ci.sccd_md, BLE_GATTS_VLOC_STACK);
	setAttrMdReadOnly(ci.attr_md, BLE_GATTS_VLOC_USER);

	// these characteristic descriptors are optional, and I gather, not really used by anything.
	// we fill them in if the user specifies any of the data (eg name).
	ci.char_md.p_char_user_desc = NULL;
	ci.char_md.p_char_pf = NULL;
	ci.char_md.p_user_desc_md = NULL;
	ci.char_md.p_cccd_md = &ci.cccd_md; // the client characteristic metadata.
	//  _char_md.p_sccd_md         = &_sccd_md;

	_uuid.init();
	ble_uuid_t uid = _uuid;

	ci.attr_char_value.p_attr_md = &ci.attr_md;

	ci.attr_char_value.init_offs = 0;
	ci.attr_char_value.init_len = getValueLength();
	ci.attr_char_value.max_len = getValueMaxLength();
	ci.attr_char_value.p_value = getValuePtr();

	std::string name = std::string(_name);

	LOGd("%s init with buffer[%i] with %p", name.c_str(), getValueLength(), getValuePtr());

	ci.attr_char_value.p_uuid = &uid;

	setupWritePermissions(ci);

	if (!name.empty()) {
		ci.char_md.p_char_user_desc = (uint8_t*) name.c_str(); // todo utf8 conversion?
		ci.char_md.char_user_desc_size = name.length();
		ci.char_md.char_user_desc_max_size = name.length();
	}

	// This is the metadata (eg security settings) for the description of this characteristic.
	ci.char_md.p_user_desc_md = &ci.user_desc_metadata_md;

	setAttrMdReadOnly(ci.user_desc_metadata_md, BLE_GATTS_VLOC_STACK);

	this->configurePresentationFormat(ci.presentation_format);
	ci.presentation_format.unit = 0; //_unit;
	ci.char_md.p_char_pf = &ci.presentation_format;

	volatile uint16_t svc_handle = svc->getHandle();

	uint32_t err_code = sd_ble_gatts_characteristic_add(svc_handle, &ci.char_md, &ci.attr_char_value, &_handles);
	if (err_code != NRF_SUCCESS) {
		LOGe(MSG_BLE_CHAR_CREATION_ERROR);
		APP_ERROR_CHECK(err_code);
	}

	//BLE_CALL(sd_ble_gatts_characteristic_add, (svc_handle, &ci.char_md, &ci.attr_char_value, &_handles));

	_status.inited = true;
}

/* Setup default write permissions.
 *
 * Structure has the following layout:
 *   ble_gatt_char_props_t       char_props;               Characteristic Properties.
 *   ble_gatt_char_ext_props_t   char_ext_props;           Characteristic Extended Properties.
 *   uint8_t                    *p_char_user_desc;         Pointer to a UTF-8, NULL if the descriptor is not required.
 *   uint16_t                    char_user_desc_max_size;  The maximum size in bytes of the user description descriptor.
 *   uint16_t                    char_user_desc_size;      The size of the user description, must be smaller or equal to char_user_desc_max_size.
 *   ble_gatts_char_pf_t*        p_char_pf;                Pointer to a presentation format structure or NULL if the descriptor is not required.
 *   ble_gatts_attr_md_t*        p_user_desc_md;           Attribute metadata for the User Description descriptor, or NULL for default values.
 *   ble_gatts_attr_md_t*        p_cccd_md;                Attribute metadata for the Client Characteristic Configuration Descriptor, or NULL for default values.
 *   ble_gatts_attr_md_t*        p_sccd_md;                Attribute metadata for the Server Characteristic Configuration Descriptor, or NULL for default values.
 *
 */
void CharacteristicBase::setupWritePermissions(CharacteristicInit& ci) {
	// Dominik: why set it if the whole struct is being overwritten anyway futher down??!!
	//	ci.attr_md.write_perm.sm = _writable ? 1 : 0;
	//	ci.attr_md.write_perm.lv = _writable ? 1 : 0;
	ci.char_md.char_props.write = _status.writable ? 1 : 0;
	// Dominik: why is write wo response automatically enabled when writable? shouldn't it
	//  be handled independently?!
//	ci.char_md.char_props.write_wo_resp = _status.writable ? 1 : 0;
	ci.char_md.char_props.notify = _status.notifies ? 1 : 0;
	// Dominik: agreed, indications seem to be almost the same as notifications, but they
	//  are not totally the same, so why set them together? they should be handled
	//  independently!!
	// ci.char_md.char_props.indicate = _notifies ? 1 : 0;
	ci.char_md.char_props.indicate = _status.indicates ? 1 : 0;
	ci.attr_md.write_perm = _writeperm;
	// Dominik: this overwrites the write permissions based on the security mode,
	// so even setting writable to false will always have the write permissions set
	// to true because security mode of 0 / 0 is not defined by the Bluetooth Core
	// specification anyway. security mode is always at least 1 / 1. Doesnt make sense !!!
	//	ci.char_md.char_props.write = ci.cccd_md.write_perm.lv > 0 ? 1 :0;
	//	ci.char_md.char_props.write_wo_resp = ci.cccd_md.write_perm.lv > 0 ? 1 :0;

   // BART
	//ci.char_md.char_ext_props.reliable_wr = _status.writable ? 1 : 0;
}

uint32_t CharacteristicBase::notify() {

	uint16_t valueLength = getValueLength();
	uint8_t* valueAddress = getValuePtr();

	BLE_CALL(cs_sd_ble_gatts_value_set, (_service->getStack()->getConnectionHandle(),
			_handles.value_handle, &valueLength, valueAddress));

	// stop here if we are not in notifying state
	if ((!_status.notifies) || (!_service->getStack()->connected()) || !_status.notifyingEnabled) {
		return NRF_SUCCESS;
	}

	ble_gatts_hvx_params_t hvx_params;
	uint16_t len = valueLength;

	hvx_params.handle = _handles.value_handle;
	hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
	hvx_params.offset = 0;
	hvx_params.p_len = &len;
	hvx_params.p_data = valueAddress;

	//	BLE_CALL(sd_ble_gatts_hvx, (_service->getStack()->getConnectionHandle(), &hvx_params));
	uint32_t err_code = sd_ble_gatts_hvx(_service->getStack()->getConnectionHandle(), &hvx_params);
	if (err_code != NRF_SUCCESS) {

		if (err_code == BLE_ERROR_NO_TX_BUFFERS) {
			// Dominik: this happens if several characteristics want to send a notification,
			//   but the system only has a limited number of tx buffers available. so queueing up
			//   notifications faster than being able to send them out from the stack results
			//   in this error.
			onNotifyTxError();
		} else if (err_code == NRF_ERROR_INVALID_STATE) {
			// Dominik: if a characteristic is updating it's value "too fast" and notification is enabled
			//   it can happen that it tries to update it's value although notification was disabled in
			//   in the meantime, in which case an invalid state error is returned. but this case we can
			//   ignore

			// this is not a serious error, but better to at least write it to the log
			LOGd("ERR_CODE: %d (0x%X)", err_code, err_code);
		} else if (err_code == BLE_ERROR_GATTS_SYS_ATTR_MISSING) {
			// Anne: do not complain for now... (meshing)
		} else {
			APP_ERROR_CHECK(err_code);
		}
	}

	return err_code;
}

void CharacteristicBase::onNotifyTxError() {
	// in most cases we can just ignore the error, because the characteristic is updated frequently
	// so the next update will probably be done as soon as the tx buffers are ready anyway, but in
	// case there are characteristics that only get updated really infrequently, the notification
	// should be buffered and sent again once the tx buffers are ready
	//	LOGd("[%s] no tx buffers, notification skipped!", _name.c_str());
}

void CharacteristicBase::onTxComplete(ble_common_evt_t * p_ble_evt) {
	// if a characteristic buffers notification when it runs out of tx buffers it can use
	// this callback to resend the buffered notification
}

