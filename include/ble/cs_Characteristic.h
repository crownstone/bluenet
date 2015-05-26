/**
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>

#include <ble/cs_Service.h>
#include <ble/cs_UUID.h>

#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <util/cs_BleError.h>
#include <third/std/function.h>

#include <common/cs_Types.h>

/* General BLE name service
 *
 * All functionality that is just general BLE functionality is encapsulated in the BLEpp namespace.
 */
namespace BLEpp {

class Service;
//class Nrf51822BluetoothStack;

/* CharacteristicInit collects fields required to define a BLE characteristic
 */
struct CharacteristicInit {
	ble_gatts_attr_t          attr_char_value;
	// pointer to a presentation format structure (p_char_pf)
	ble_gatts_char_pf_t       presentation_format;
	// characteristic metadata
	ble_gatts_char_md_t       char_md;
	// attribute metadata for client characteristic configuration  (p_cccd_md)
	ble_gatts_attr_md_t       cccd_md;
	// attributed metadata for server characteristic configuration (p_sccd_md)
	ble_gatts_attr_md_t       sccd_md;
	ble_gatts_attr_md_t       attr_md;
	// attribute metadata for user description (p_user_desc_md)
	ble_gatts_attr_md_t       user_desc_metadata_md;

	CharacteristicInit() : presentation_format({}), char_md({}), cccd_md({}), attr_md({}) {}
};


typedef uint8_t boolean_t;

#define STATUS_INITED

struct Status {
	boolean_t inited                                  : 1;
	boolean_t notifies                                : 1;
	boolean_t writable                                : 1;
	boolean_t notifyingEnabled                        : 1;
	boolean_t indicates                               : 1;
	boolean_t                                         : 3;
};

/* Non-template base class for Characteristics.
 *
 * A non-templated base class saves on code size. Note that every characteristic however does still
 * contribute to code size.
 */
class CharacteristicBase {

public:

protected:
	// Universally Unique Identifier (8 bytes)
	UUID                      _uuid;
	// Name (4 bytes)
	const char *              _name;
	// Read permission (1 byte)
	ble_gap_conn_sec_mode_t   _readperm;
	// Write permission (1 byte)
	ble_gap_conn_sec_mode_t   _writeperm;
	// Handles (8 bytes)
	ble_gatts_char_handles_t  _handles;
	// Reference to corresponding service (4 bytes)
	Service*                  _service;

	// Status of CharacteristicBase (basically a bunch of 1-bit flags)
	Status                    _status;

#ifdef BIG_SIZE_REQUIRED
	bool                      .inited;

	/* This characteristic can be set to notify at regular intervals.
	 *
	 * This interval cannot be set from the client side.
	 */
	bool                      .notifies;

	/* This characteristic can be written by another device.
	 */
	bool                      .writable;

	/* If this characteristic can notify a listener (<.notifies>), this field enables it.
	 */
	bool                      .notifyingEnabled;

	/* This characteristic can be set to indicate at regular intervals.
	 *
	 * Indication is different from notification, in the sense that it requires ACKs.
	 * https://devzone.nordicsemi.com/question/310/notificationindication-difference/
	 */
	bool                      .indicates;
#endif
	// Unit
//	uint16_t                  _unit;

	/* Interval for updates (4 bytes), 0 means don't update
	 *
	 * TODO: Currently, this is not in use.
	 */
//	uint32_t                  _updateIntervalMsecs;

public:
	/* Default constructor for CharacteristicBase
	 */
	CharacteristicBase();

	/* Empty destructor
	 */
	virtual ~CharacteristicBase() {}

	/* Initialize the characteristic.
	 * @param svc BLE service this characteristic will belong to.
	 *
	 * Defaults:
	 *
	 * + readable: true
	 * + notifies: true
	 * + broadcast: false
	 * + indicates: false
	 *
	 * Side effect: sets member field <_status.inited>.
	 */
	void init(Service* svc);

	/* Set this characteristic to be writable.
	 */
	CharacteristicBase& setWritable(bool writable) {
		_status.writable = writable;
		setWritePermission(1, 1);
		return *this;
	}

	void setupWritePermissions(CharacteristicInit& ci);

	/* Set this characteristic to be notifiable.
	 */
	CharacteristicBase& setNotifies(bool notifies) {
		_status.notifies = notifies;
		return *this;
	}

	bool isNotifyingEnabled() {
		return _status.notifyingEnabled;
	}

	void setNotifyingEnabled(bool enabled) {
		//        	LOGd("[%s] notfying enabled: %s", _name.c_str(), enabled ? "true" : "false");
		_status.notifyingEnabled = enabled;
	}

	CharacteristicBase& setIndicates(bool indicates) {
		_status.indicates = indicates;
		return *this;
	}

	/* Security Mode 0 Level 0: No access permissions at all (this level is not defined by the Bluetooth Core specification).\n
	 * Security Mode 1 Level 1: No security is needed (aka open link).\n
	 * Security Mode 1 Level 2: Encrypted link required, MITM protection not necessary.\n
	 * Security Mode 1 Level 3: MITM protected encrypted link required.\n
	 * Security Mode 2 Level 1: Signing or encryption required, MITM protection not necessary.\n
	 * Security Mode 2 Level 2: MITM protected signing required, unless link is MITM protected encrypted.\n
	 */
	CharacteristicBase& setWritePermission(uint8_t securityMode, uint8_t securityLevel) {
		_writeperm.sm = securityMode;
		_writeperm.lv = securityLevel;
		return *this;
	}

	CharacteristicBase& setUUID(const UUID& uuid) {
		if (_status.inited) BLE_THROW("Already inited.");
		_uuid = uuid;
		return *this;
	}

	CharacteristicBase& setName(const char * const name);

	CharacteristicBase& setUnit(uint16_t unit);

	const UUID & getUUID() {
		return _uuid;
	}

	uint16_t getValueHandle() {
		return _handles.value_handle;
	}

	uint16_t getCccdHandle() {
		return _handles.cccd_handle;
	}

	CharacteristicBase& setUpdateIntervalMSecs(uint32_t msecs);

	/* Return the maximum length of the value */
	virtual uint16_t getValueMaxLength() = 0;
	/* Return the actual length of the value */
	virtual uint16_t getValueLength() = 0;
	/* Return the pointer to the memory where the value is stored */
	virtual uint8_t* getValuePtr() = 0;

	/* Set the actual length of the data
	 * @length the length of the data to which the value points
	 *
	 * This is only necessary for buffer values. When a value is received
	 * over BT we need to update the length of the data
	 */
	virtual void setDataLength(uint16_t length) {};

	virtual void read() = 0;
	virtual void written(uint16_t len) = 0;

	virtual void onTxComplete(ble_common_evt_t * p_ble_evt);

	/* Notify any listening party.
	 *
	 * If this characteristic can notify, and if notification is enabled, and if there is a connection calling
	 * this function will notify the listening party.
	 *
	 * Notification quickly fills an outgoing buffer in BLE. When this buffer gets full, an error code
	 * <BLE_ERROR_NO_TX_BUFFERS> is generated by the SoftDevice. In that case <onNotifyTxError> is called.
	 *
	 * @return err_code (which should be NRF_SUCCESS if no error occurred)
	 */
	uint32_t notify();

protected:

	virtual void configurePresentationFormat(ble_gatts_char_pf_t &) {}

	/* Any error in <notify> evokes onNotifyTxError.
	 */
	virtual void onNotifyTxError();

	void setAttrMdReadOnly(ble_gatts_attr_md_t& md, char vloc);
	void setAttrMdReadWrite(ble_gatts_attr_md_t& md, char vloc);

};

/* The templated version of ble_type
 * @T The class / primitive to template ble_type with
 */
template<typename T>
inline uint8_t ble_type() {
	return BLE_GATT_CPF_FORMAT_STRUCT;
}
// A ble_type for strings
template<>
inline uint8_t ble_type<std::string>() {
	return BLE_GATT_CPF_FORMAT_UTF8S;
}
// A ble_type for 8-bit unsigned values
template<>
inline uint8_t ble_type<uint8_t>() {
	return BLE_GATT_CPF_FORMAT_UINT8;
}
// A ble_type for 16-bit unsigned values
template<>
inline uint8_t ble_type<uint16_t>() {
	return BLE_GATT_CPF_FORMAT_UINT16;
}
// A ble_type for 32-bit unsigned values
template<>
inline uint8_t ble_type<uint32_t>() {
	return BLE_GATT_CPF_FORMAT_UINT32;
}
// A ble_type for 8-bit signed values
template<>
inline uint8_t ble_type<int8_t>() {
	return BLE_GATT_CPF_FORMAT_SINT8;
}
// A ble_type for 16-bit signed values
template<>
inline uint8_t ble_type<int16_t>() {
	return BLE_GATT_CPF_FORMAT_SINT16;
}
// A ble_type for 32-bit signed values
template<>
inline uint8_t ble_type<int32_t>() {
	return BLE_GATT_CPF_FORMAT_SINT32;
}
// A ble_type for floats (32 bits)
template<>
inline uint8_t ble_type<float>() {
	return BLE_GATT_CPF_FORMAT_FLOAT32;
}
// A ble_type for doubles (64 bits)
template<>
inline uint8_t ble_type<double>() {
	return BLE_GATT_CPF_FORMAT_FLOAT64;
}
// A ble_type for booleans (8 bits)
template<>
inline uint8_t ble_type<bool>() {
	return BLE_GATT_CPF_FORMAT_BOOLEAN;
}

/* Characteristic of generic type T
 * @T Generic type T
 *
 * A characteristic first of all contains a templated "value" which might be a string, an integer, or a
 * buffer, depending on the need at hand.
 * It allows also for callbacks to be defined on writing to the characteristic, or reading from the
 * characteristic.
 */
template<class T>
class CharacteristicGeneric : public CharacteristicBase {

	friend class Service;

public:
	// Format of callback on write (from user)
	typedef function<void(const T&)> callback_on_write_t;
	// Format of read callback
	typedef function<T()> callback_on_read_t;

protected:
	// The generic type is physically located in this field in this class (by value, not just by reference)
	T                          _value;
	// The callback to call on a write coming from the softdevice (and originating from the user)
	callback_on_write_t        _callbackOnWrite;
	// Callback on read
	callback_on_read_t         _callbackOnRead;

	/* Flag to indicate if notification is pending to be sent once currently waiting
	 * tx operations are completed
	 */
	bool _notificationPending;

public:
	CharacteristicGeneric() : _notificationPending(false) {};

	// Default empty destructor
	virtual ~CharacteristicGeneric() {};

	T&  __attribute__((optimize("O0"))) getValue() {
		return _value;
	}

	void setValue(const T& value) {
		_value = value;
	}

	CharacteristicGeneric<T>& setUUID(const UUID& uuid) {
		CharacteristicBase::setUUID(uuid);
		return *this;
	}

	CharacteristicGeneric<T>& setName(const char * const name) {
		CharacteristicBase::setName(name);
		return *this;
	}

	CharacteristicGeneric<T>& setUnit(uint16_t unit) {
		CharacteristicBase::setUnit(unit);
		return *this;
	}

	CharacteristicGeneric<T>& setWritable(bool writable) {
		CharacteristicBase::setWritable(writable);
		return *this;
	}

	CharacteristicGeneric<T>& setNotifies(bool notifies) {
		CharacteristicBase::setNotifies(notifies);
		return *this;
	}

	CharacteristicGeneric<T>& onWrite(const callback_on_write_t& closure) {
		_callbackOnWrite = closure;
		return *this;
	}

	CharacteristicGeneric<T>& onRead(const callback_on_read_t& closure) {
		_callbackOnRead = closure;
		return *this;
	}

	CharacteristicGeneric<T>& setUpdateIntervalMSecs(uint32_t msecs) {
		CharacteristicBase::setUpdateIntervalMSecs(msecs);
		return *this;
	}

	/* CharacteristicGeneric() returns value object
	 *
	 * @return value object
	 */
	operator T&() {
		return _value;
	}

	CharacteristicGeneric<T>& operator=(const T& val) {
		_value = val;

		notify();

		return *this;
	}

#ifdef ADVANCED_OPERATORS
	CharacteristicGeneric<T>& operator+=(const T& val) {
		_value += val;

		notify();

		return *this;
	}

	CharacteristicGeneric<T>& operator-=(const T& val) {
		_value -= val;

		notify();

		return *this;
	}

	CharacteristicGeneric<T>& operator*=(const T& val) {
		_value *= val;

		notify();

		return *this;
	}

	CharacteristicGeneric<T>& operator/=(const T& val) {
		_value /= val;

		notify();

		return *this;
	}
#endif

	CharacteristicGeneric<T>& setDefaultValue(const T& t) {
		if (_status.inited) BLE_THROW("Already inited.");
		_value = t;
		return *this;
	}

	/* Callback function if a notify tx error occurs
	 *
	 * This is called when the notify operation fails with a tx error. This
	 * can occur when too many tx operations are taking place at the same time.
	 *
	 * A <BLEpp::CharacteristicGenericBase::notify> is called when the master device
	 * connected to the Crownstone requests automatic notifications whenever
	 * the value changes.
	 */
	void onNotifyTxError() {
		_notificationPending = true;
	}

	/* Callback function once tx operations complete
	 * @p_ble_evt the event object which triggered the onTxComplete callback
	 *
	 * This is called whenever tx operations complete. If a notification is pending
	 * <BLEpp::CharacteristicGenericBase::notify> is called again and the notification
	 * is cleared if the call eas successful. If not successful, it will be tried
	 * again during the next callback call
	 */
	void onTxComplete(ble_common_evt_t * p_ble_evt) {
		// if we have a notification pending, try to send it
		if (_notificationPending) {
			// this-> is necessary so that the call of notify depends on the template
			// parameter T
			uint32_t err_code = this->notify();
			if (err_code == NRF_SUCCESS) {
				_notificationPending = false;
			}
		}
	}

protected:

	void written(uint16_t len) {
		setDataLength(len);

		LOGd("%s: onWrite()", _name);
		_callbackOnWrite(getValue());
	}

	void read() {
		LOGd("%s: onRead()", _name);
		T newValue = _callbackOnRead();
		if (newValue != _value) {
			operator=(newValue);
		}
	}


	virtual void configurePresentationFormat(ble_gatts_char_pf_t& presentation_format) {
		presentation_format.format = ble_type<T>();
		presentation_format.name_space = BLE_GATT_CPF_NAMESPACE_BTSIG;
		presentation_format.desc = BLE_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN;
		presentation_format.exponent = 1;
	}

private:
};

/* A default characteristic
 * @T type of the value
 * @E default type (subdefined for example for built-in types)
 *
 * Comes with an assignment operator, so we can assign characteristics to each other which copies the internal value.
 */
template<typename T, typename E = void>
class Characteristic : public CharacteristicGeneric<T> {
public:
	Characteristic() : CharacteristicGeneric<T>() {}

	Characteristic& operator=(const T& val) {
		CharacteristicGeneric<T>::operator=(val);
		return *this;
	}
};

// A characteristic for built-in arithmetic types (int, float, etc)
template<typename T >
class Characteristic<T, typename std::enable_if<std::is_arithmetic<T>::value >::type> : public CharacteristicGeneric<T> {
public:
	/* @inherit */
	Characteristic& operator=(const T& val) {
		CharacteristicGeneric<T>::operator=(val);
		return *this;
	}

	/* Return the actual length of the value
	 *
	 * The length of a basic data type is the size of the type
	 */
	uint16_t getValueLength() {
		return sizeof(T);
	}

	/* Return the maximum length of the value
	 *
	 * The maximum length of a basic data type is the size of the type
	 */
	uint16_t getValueMaxLength() {
		return sizeof(T);
	}

	/* Return the pointer to the memory where the value is stored
	 *
	 * For a basic data type return the address where the first byte
	 * is stored
	 */
	uint8_t* getValuePtr() {
		return (uint8_t*)&this->getValue();
	}
};

// A characteristic for strings
template<>
class Characteristic<std::string> : public CharacteristicGeneric<std::string> {
public:
	/* @inherit */
	Characteristic& operator=(const std::string& val) {
		CharacteristicGeneric<std::string>::operator=(val);
		return *this;
	}

	/* Return the actual length of the value
	 *
	 * Return the length of the string
	 */
	uint16_t getValueLength() {
		return getValue().length();
	}

	/* Return the maximum length of the value
	 *
	 * The maximum length of a string is defined as
	 * <MAX_STRING_LENGTH>
	 */
	uint16_t getValueMaxLength() {
		return MAX_STRING_LENGTH;
	}

	/* Return the pointer to the memory where the value is stored
	 *
	 * For a string return pointer to null-terminated content of the
	 * string
	 */
	uint8_t* getValuePtr() {
		return (uint8_t*)getValue().c_str();
	}
};

/* This template implements the functions specific for a pointer to a buffer.
 */
template<>
class Characteristic<buffer_ptr_t> : public CharacteristicGeneric<buffer_ptr_t> {

private:
	// maximum length for this characteristic in bytes
	uint16_t _maxLength;

	// actual length of data stored in the buffer in bytes
	uint16_t _dataLength;

public:

	/* Set the value for this characteristic
	 * @value the pointer to the buffer in memory
	 *
	 * only valid pointers are allowed, NULL is NOT allowed
	 */
	void setValue(const buffer_ptr_t& value) {
		assert(value, "Error: Don't assign NULL pointers! Pointer to buffer has to be valid from the start!");
		_value = value;
	}

	/* Set the maximum length of the buffer
	 * @length maximum length in bytes
	 *
	 * This defines how many bytes were allocated for the buffer
	 * so how many bytes can max be stored in the buffer
	 */
	void setMaxLength(uint16_t length) {
		_maxLength = length;
	}

	/* Set the length of data stored in the buffer
	 * @length length of data in bytes
	 */
	void setDataLength(uint16_t length) {
		_dataLength = length;
	}

	/* Return the maximum possible length of the buffer
	 *
	 * Checks the object assigned to this characteristics for the maximum
	 * possible length
	 *
	 * @return the maximum possible length
	 */
	uint16_t getValueMaxLength() {
		return _maxLength;
	}

	/* Return the actual length of the value
	 *
	 * For a buffer, this is the length of data stored in the buffer in bytes
	 *
	 * @return number of bytes
	 */
	uint16_t getValueLength() {
		return _dataLength;
	}

	/* Return the pointer to the memory where the value is stored
	 *
	 * For a buffer, this is the value itself
	 */
	uint8_t* getValuePtr() {
		return getValue();
	}

protected:

};

} // end of namespace



