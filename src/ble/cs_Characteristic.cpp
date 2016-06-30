/**
 * Bluetooth Low Energy Characteristic
 *
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 23, 2015
 * License: LGPLv3+
 */

#include "ble/cs_Characteristic.h"

#include <ble/cs_Softdevice.h>

#include <storage/cs_Settings.h>

using namespace BLEpp;

#define PRINT_VERBOSE

CharacteristicBase::CharacteristicBase() :
				_name(NULL),
				_handles( { }), _service(0), _encrypted(false)
				/*nited(false), _notifies(false), _writable(false), */
				/*_unit(0), */ /*_updateIntervalMsecs(0),*/ /* _notifyingEnabled(false), _indicates(false) */
{
}

CharacteristicBase& CharacteristicBase::setName(const char * const name) {
	if (_status.inited) BLE_THROW(MSG_BLE_CHAR_INITIALIZED);
	_name = name;

	return *this;
}

/** Set the default attributes of every characteristic
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
//void CharacteristicBase::setAttrMdReadOnly(ble_gatts_attr_md_t& md, char vloc) {
////	BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&md.read_perm);
////	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&md.write_perm);
//	md.vloc = vloc;
//	md.rd_auth = 0; //! don't request read access every time a read is attempted.
//	md.wr_auth = 0;  //! ditto for write.
//	md.vlen = 1;  //! variable length.
//}

//void CharacteristicBase::setAttrMdReadWrite(ble_gatts_attr_md_t& md, char vloc) {
//	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.read_perm);
//	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&md.write_perm);
//	md.vloc = vloc;
//	md.rd_auth = 0; //! don't request read access every time a read is attempted.
//	md.wr_auth = 0;  //! ditto for write.
//	md.vlen = 1;  //! variable length.
//}

/*
CharacteristicBase& CharacteristicBase::setUnit(uint16_t unit) {
	_unit = unit;
	return *this;
} */

//CharacteristicBase& CharacteristicBase::setUpdateIntervalMSecs(uint32_t msecs) {
//!    if (_updateIntervalMsecs != msecs) {
//!        _updateIntervalMsecs = msecs;
//!        //! TODO schedule task.
//!        app_timer_create(&_timer, APP_TIMER_MODE_REPEATED, [](void* context){
//!            CharacteristicBase* characteristic = (CharacteristicBase*)context;
//!            characteristic->read();
//!        });
//!        app_timer_start(_timer, (uint32_t)(_updateIntervalMsecs / 1000. * (32768 / NRF51_RTC1_PRESCALER +1)), this);
//!    }
//!    return *this;
//}

void CharacteristicBase::init(Service* svc) {
	_service = svc;

	CharacteristicInit ci;

    memset(&ci.char_md, 0, sizeof(ci.char_md));
	setupWritePermissions(ci);

	////////////////////////////////////////////////////////////////
	//! attribute metadata for client characteristic configuration //
	////////////////////////////////////////////////////////////////

    memset(&ci.cccd_md, 0, sizeof(ci.cccd_md));
	ci.cccd_md.vloc = BLE_GATTS_VLOC_STACK;
	ci.cccd_md.vlen = 1;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&ci.cccd_md.read_perm); //! required

    if (_encrypted) {
    	BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&ci.cccd_md.write_perm);
    } else {
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&ci.cccd_md.write_perm);
    }

	ci.char_md.p_cccd_md = &ci.cccd_md; //! the client characteristic metadata.

	/////////////////////
	//! attribute value //
	/////////////////////

	_uuid.init();
	ble_uuid_t uuid = _uuid;

    memset(&ci.attr_char_value, 0, sizeof(ci.attr_char_value));

	ci.attr_char_value.p_uuid = &uuid;

	ci.attr_char_value.init_offs = 0;
	ci.attr_char_value.init_len = getValueLength();
	ci.attr_char_value.max_len = getValueMaxLength();
	ci.attr_char_value.p_value = getValuePtr();

#ifdef PRINT_VERBOSE
	LOGd("%s init with buffer[%i] with %p", _name, getValueLength(), getValuePtr());
#endif

	////////////////////////
	//! attribute metadata //
	////////////////////////

    memset(&ci.attr_md, 0, sizeof(ci.attr_md));
	ci.attr_md.vloc = BLE_GATTS_VLOC_USER;
	ci.attr_md.vlen = 1;

    if (_encrypted) {
		BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&ci.attr_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&ci.attr_md.write_perm);
    } else {
    	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&ci.attr_md.read_perm);
    	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&ci.attr_md.write_perm);
    }
	ci.attr_char_value.p_attr_md = &ci.attr_md;

	/////////////////////////////////////
	//! Characteristic User Description //
	/////////////////////////////////////

	//! these characteristic descriptors are optional, and I gather, not really used by anything.
	//! we fill them in if the user specifies any of the data (eg name).
	ci.char_md.p_char_user_desc = NULL;
	ci.char_md.p_user_desc_md = NULL;

	std::string name = std::string(_name);
	if (!name.empty()) {
		ci.char_md.p_char_user_desc = (uint8_t*) name.c_str(); //! todo utf8 conversion?
		ci.char_md.char_user_desc_size = name.length();
		ci.char_md.char_user_desc_max_size = name.length();

		//! This is the metadata (eg security settings) for the description of this characteristic.
		memset(&ci.user_desc_metadata_md, 0, sizeof(ci.user_desc_metadata_md));

		ci.user_desc_metadata_md.vloc = BLE_GATTS_VLOC_STACK;
		ci.user_desc_metadata_md.vlen = 1;

	    if (_encrypted) {
	    	BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&ci.user_desc_metadata_md.read_perm);
	    } else {
			BLE_GAP_CONN_SEC_MODE_SET_OPEN(&ci.user_desc_metadata_md.read_perm);
	    }

		BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&ci.user_desc_metadata_md.write_perm); //! required

		ci.char_md.p_user_desc_md = &ci.user_desc_metadata_md;
	}

	/////////////////////////
	//! Presentation Format //
	/////////////////////////

	//! presentation format is optional, only fill out if characteristic supports

	ci.char_md.p_char_pf = NULL;
	if (configurePresentationFormat(ci.presentation_format)) {
		ci.presentation_format.unit = 0; //_unit;
		ci.char_md.p_char_pf = &ci.presentation_format;
	}

	//! add all
	uint32_t err_code = sd_ble_gatts_characteristic_add(svc->getHandle(), &ci.char_md, &ci.attr_char_value, &_handles);
	if (err_code != NRF_SUCCESS) {
		LOGe(MSG_BLE_CHAR_CREATION_ERROR);
		APP_ERROR_CHECK(err_code);
	}

	//BLE_CALL(sd_ble_gatts_characteristic_add, (svc_handle, &ci.char_md, &ci.attr_char_value, &_handles));

	//! set initial value (default value)
	notify();
//	BLE_CALL(cs_sd_ble_gatts_value_set, (_service->getStack()->getConnectionHandle(),
//			_handles.value_handle, &ci.attr_char_value.init_len, ci.attr_char_value.p_value));

	_status.inited = true;
}

/** Setup default write permissions.
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
	ci.char_md.char_props.read = 1; //! allow read
	ci.char_md.char_props.broadcast = 0; //! don't allow broadcast
	ci.char_md.char_props.write = _status.writable ? 1 : 0;
	ci.char_md.char_props.notify = _status.notifies ? 1 : 0;
	ci.char_md.char_props.indicate = _status.indicates ? 1 : 0;

	ci.attr_md.write_perm = _writeperm;
}

uint32_t CharacteristicBase::notify() {

	uint16_t valueLength = getValueLength();
	uint8_t* valueAddress = getValuePtr();

	BLE_CALL(cs_sd_ble_gatts_value_set, (_service->getStack()->getConnectionHandle(),
			_handles.value_handle, &valueLength, valueAddress));

	//! stop here if we are not in notifying state
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
			//! Dominik: this happens if several characteristics want to send a notification,
			//!   but the system only has a limited number of tx buffers available. so queueing up
			//!   notifications faster than being able to send them out from the stack results
			//!   in this error.
			onNotifyTxError();
		} else if (err_code == NRF_ERROR_INVALID_STATE) {
			//! Dominik: if a characteristic is updating it's value "too fast" and notification is enabled
			//!   it can happen that it tries to update it's value although notification was disabled in
			//!   in the meantime, in which case an invalid state error is returned. but this case we can
			//!   ignore

			//! this is not a serious error, but better to at least write it to the log
			LOGe("ERR_CODE: %d (0x%X)", err_code, err_code);
		} else if (err_code == BLE_ERROR_GATTS_SYS_ATTR_MISSING) {
			//! Anne: do not complain for now... (meshing)
		} else {
			APP_ERROR_CHECK(err_code);
		}
	}

	return err_code;
}

void CharacteristicBase::onNotifyTxError() {
	//! in most cases we can just ignore the error, because the characteristic is updated frequently
	//! so the next update will probably be done as soon as the tx buffers are ready anyway, but in
	//! case there are characteristics that only get updated really infrequently, the notification
	//! should be buffered and sent again once the tx buffers are ready
	//	LOGd("[%s] no tx buffers, notification skipped!", _name.c_str());
}

void CharacteristicBase::onTxComplete(ble_common_evt_t * p_ble_evt) {
	//! if a characteristic buffers notification when it runs out of tx buffers it can use
	//! this callback to resend the buffered notification
}

