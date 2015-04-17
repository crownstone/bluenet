/**
 * Author: Christopher Mason
 * Author: Anne van Rossum
 * License: TODO
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "cs_Handlers.h"

extern "C" {

// The header files for the SoftDevice. We're trying really hard to only them only here.
// The user should not need to include files under a Nordic license. 
// This means that we can distribute our files in the end without the corresponding Nordic header files.
#include "ble_gap.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_gatts.h"
#include "ble_gatt.h"

// Refering to files in the Nordic SDK
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_sdm.h"

}

// Local wrapper files
#include <drivers/cs_PWM.h>

#include <util/cs_BleError.h>
#include <drivers/cs_Serial.h>
#include <cs_UUID.h>
#include <cs_AllocatedBuffer.h>
#include <cs_iBeacon.h>
#include <common/cs_MasterBuffer.h>
#include <common/cs_Config.h>

#include <third/std/function.h>

// TODO: replace std::vector with a fixed, in place array of size capacity.

/* A tuple is a vector with a templated type and a public constructor.
 * @T templated element which goes into the vector
 */
template<typename T> class tuple : public std::vector<T> {
public:
	// Default constructor
	tuple() {}
};

/* A fixed tuple is a vector with a templated type and a reserved capacity.
 * @T templated type which goes into the vector
 * @capacity Predefined capacity of the underlying std::vector.
 */
template<typename T, uint8_t capacity> class fixed_tuple : public tuple<T> {
public:
	// Constructor reserves capacity in vector
	fixed_tuple<T, capacity>() : tuple<T>() {this->reserve(capacity);}

};

/*
 * Interrupt service requests are C, so need to be unmangled (and are henced wrapped in an extern "C" block)
 */
extern "C" {

	/* Interrupt request for SoftDevice
	 */
	void SWI1_IRQHandler(void);

}

/* General BLE name service
 *
 * All functionality that is just general BLE functionality is encapsulated in the BLEpp namespace.
 */
namespace BLEpp {

class Service;
class Nrf51822BluetoothStack;

/* CharacteristicInit collects fields required to define a BLE characteristic
 */
struct CharacteristicInit {
	ble_gatts_attr_t          attr_char_value;
	// pointer to a presentation format structure (p_char_pf)
	ble_gatts_char_pf_t       presentation_format;
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
	std::string               _name;
	// Read permission (1 byte)
	ble_gap_conn_sec_mode_t   _readperm;
	// Write permission (1 byte)
	ble_gap_conn_sec_mode_t   _writeperm;
	// Handles (8 bytes)
	ble_gatts_char_handles_t  _handles;
	// Reference to corresponding service (4 bytes)
	Service*                  _service;

	bool                      _inited;

	/* This characteristic can be set to notify at regular intervals.
	 *
	 * This interval cannot be set from the client side.
	 */
	bool                      _notifies;

	/* This characteristic can be written by another device.
	 */
	bool                      _writable;

	// Unit
	uint16_t                  _unit;

	/* Interval for updates (4 bytes), 0 means don't update
	 *
	 * TODO: Currently, this is not in use. 
	 */
	uint32_t                  _updateIntervalMsecs;

	/* If this characteristic can notify a listener (<_notifies>), this field enables it.
	 */
	bool                      _notifyingEnabled;

	/* This characteristic can be set to indicate at regular intervals.
	 *
	 * Indication is different from notification, in the sense that it requires ACKs.
	 * https://devzone.nordicsemi.com/question/310/notificationindication-difference/
	 */
	bool                      _indicates;

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
	 * Side effect: sets member field <_inited>.
	 */
	void init(Service* svc);

	/* Set this characteristic to be writable.
	 */
	CharacteristicBase& setWritable(bool writable) {
		_writable = writable;
		setWritePermission(1, 1);
		return *this;
	}

	void setupWritePermissions(CharacteristicInit& ci);

	/* Set this characteristic to be notifiable.
	 */
	CharacteristicBase& setNotifies(bool notifies) {
		_notifies = notifies;
		return *this;
	}

	bool isNotifyingEnabled() {
		return _notifyingEnabled;
	}

	void setNotifyingEnabled(bool enabled) {
		//        	LOGd("[%s] notfying enabled: %s", _name.c_str(), enabled ? "true" : "false");
		_notifyingEnabled = enabled;
	}

	CharacteristicBase& setIndicates(bool indicates) {
		_indicates = indicates;
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
		if (_inited) BLE_THROW("Already inited.");
		_uuid = uuid;
		return *this;
	}

	CharacteristicBase& setName(const std::string& name);

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
	virtual void written(uint16_t len, uint16_t offset, uint8_t* data) = 0;

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

	CharacteristicGeneric<T>& setName(const std::string& name) {
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
		if (_inited) BLE_THROW("Already inited.");
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

	void written(uint16_t len, uint16_t offset, uint8_t* data) {
		setDataLength(len);

		LOGd("%s: onWrite()", _name.c_str());
		_callbackOnWrite(getValue());
	}

	void read() {
		LOGd("%s: onRead()", _name.c_str());
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
	virtual uint16_t getValueLength() {
		return sizeof(T);
	}

	/* Return the maximum length of the value
	 *
	 * The maximum length of a basic data type is the size of the type
	 */
	virtual uint16_t getValueMaxLength() {
		return sizeof(T);
	}

	/* Return the pointer to the memory where the value is stored
	 *
	 * For a basic data type return the address where the first byte
	 * is stored
	 */
	virtual uint8_t* getValuePtr() {
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
	virtual uint16_t getValueLength() {
		return getValue().length();
	}

	/* Return the maximum length of the value
	 *
	 * The maximum length of a string is defined as
	 * <MAX_STRING_LENGTH>
	 */
	virtual uint16_t getValueMaxLength() {
		return MAX_STRING_LENGTH;
	}

	/* Return the pointer to the memory where the value is stored
	 *
	 * For a string return pointer to null-terminated content of the
	 * string
	 */
	virtual uint8_t* getValuePtr() {
		return (uint8_t*)getValue().c_str();
	}
};

/* Service as defined in the GATT Specification.
 */
class Service {
	friend class Nrf51822BluetoothStack;

public:
	static const char* defaultServiceName; // "Generic Service"

	typedef tuple<CharacteristicBase*> Characteristics_t;

protected:

	Nrf51822BluetoothStack*  _stack;
	UUID                     _uuid;
	std::string              _name;
	bool                     _primary;
	uint16_t                 _service_handle; // provided by stack.
	bool                     _started;

public:
	Service() :
		_stack(NULL),
		_name(""),
		_primary(true),
		_service_handle(BLE_CONN_HANDLE_INVALID),
		_started(false)
	{

	}

	/* Default empty destructor
	 *
	 * We don't currently delete our characteristics as we don't really support dynamic service destruction.
	 * If we wanted to allow services to be removed at runtime, we would need to, amongst many other things,
	 * keep track of whether we allocated the characteristic or whether it was passed into us.
	 */
	virtual ~Service() {
	}

	Service& setName(const std::string& name) {
		_name = name;
		return *this;
	}

	Service& setUUID(const UUID& uuid) {
		if (_started) BLE_THROW("Already started.");

		_uuid = uuid;
		return *this;
	}

	Service& setIsPrimary(bool isPrimary) {
		if (_started) BLE_THROW("Already started.");

		_primary = isPrimary;
		return *this;
	}

	Nrf51822BluetoothStack* getStack() {
		return _stack;
	}

	UUID& getUUID() {
		return _uuid;
	}

	uint16_t getHandle() {
		return _service_handle;
	}

	// internal:

	virtual void start(Nrf51822BluetoothStack* stack);
	virtual void stop() {}

	virtual Service& addCharacteristic(CharacteristicBase* characteristic) = 0;

	virtual Characteristics_t& getCharacteristics() = 0;

	virtual void on_ble_event(ble_evt_t * p_ble_evt);

	virtual void on_connect(uint16_t conn_handle, ble_gap_evt_connected_t& gap_evt);  // FIXME NRFAPI  friend??

	virtual void on_disconnect(uint16_t conn_handle, ble_gap_evt_disconnected_t& gap_evt);  // FIXME NRFAPI

	virtual void on_write(ble_gatts_evt_write_t& write_evt);  // FIXME NRFAPI

	virtual void onTxComplete(ble_common_evt_t * p_ble_evt);
};

/* Generic Service is a <Service> with characteristics
 *
 * Currently the number of characteristics is limited. By having this container in a separate class, in the future
 * services that have more than MAX_CHARACTERISTICS can be defined.
 *
 * This class defines only two methods:
 * + <getCharacteristics>
 * + <addCharacteristic>
 */
class GenericService : public Service {
protected:
	// Currently maximum number of characteristics per service
	static const uint8_t MAX_CHARACTERISTICS = 6;

	// List of characteristics
	fixed_tuple<CharacteristicBase*, MAX_CHARACTERISTICS> _characteristics;

public:

	/* Constructor of GenericService 
	 *
	 * Sets GenericService as BLE name
	 */
	GenericService() : Service()
	{
		setName(std::string("GenericService"));
	}

	/* Get list of characteristics
	 *
	 * @return list of characteristics
	 */
	virtual Characteristics_t & getCharacteristics() {
		return _characteristics;
	}

	/* Add a single characteristic to the list
	 * @characteristic Characteristic to add
	 */
	virtual GenericService& addCharacteristic(CharacteristicBase* characteristic) {
		if (_characteristics.size() == MAX_CHARACTERISTICS) {
			BLE_THROW("Too many characteristics.");
		}
		_characteristics.push_back(characteristic);

		return *this;
	}
};

/* Battery service
 *
 * Defines a single characteristic to read a battery level. This is a predefined UUID, stored at
 * <BLE_UUID_BATTERY_LEVEL_CHAR>. The name is "battery", and the default value is 100.
 */
class BatteryService : public GenericService {

public:
	// Define func_t as a templated function with an unsigned byte
	typedef function<uint8_t()> func_t;

protected:
	// A single characteristic with an unsigned 8-bit value
	Characteristic<uint8_t> *_characteristic;
	// A function for callback, not in use
	func_t _func;
public:
	// Constructor sets name, allocate characteristic, sets UUID, and sets default value.
	BatteryService(): GenericService() {
		setUUID(UUID(BLE_UUID_BATTERY_SERVICE));
		setName(std::string("BatteryService"));

		_characteristic = new Characteristic<uint8_t>();

		(*_characteristic).setUUID(UUID(BLE_UUID_BATTERY_LEVEL_CHAR))
                        				   .setName(std::string("battery"))
										   .setDefaultValue(100);
		addCharacteristic(_characteristic);
	}

	/* Set the battery level
	 * @batteryLevel level of the battery in percentage
	 *
	 * Indicates the level of the battery in a percentage to the user. This is of no use for a device attached to
	 * the main line. This function writes to the characteristic to show it to the user.
	 */
	void setBatteryLevel(uint8_t batteryLevel){
		(*_characteristic) = batteryLevel;
	}

	/* Set a callback function for a battery level change
	 * @func callback function
	 *
	 * Not in use
	 */
	void setBatteryLevelHandler(func_t func) {
		_func = func;
	}
};

/* BLEStack defines a chip-agnostic Bluetooth Low-Energy stack
 *
 * Currently, this class does not leverage much of the general Bluetooth Low-Energy functionality into
 * chip-agnostic code. However, this might be recommendable in the future.
 */
class BLEStack {
public:
	/* Connected?
	 *
	 * @return true if connected, false if not connected
	 */
	virtual bool connected() = 0;

	/* Handle to connection
	 *
	 * @return 16-bit value that unique identifies the connection
	 */
	virtual uint16_t getConnectionHandle() = 0;
};

/* nRF51822 specific implementation of the BLEStack
 *
 * The Nrf51822BluetoothStack class is a direct descendant from BLEStack. It is implemented as a singleton, such
 * that it can only be allocated once and it can be reached from everywhere in the code, especially in interrupt
 * handlers. However, please, if an object depends on it, try to make this dependency explicit, and use this
 * stack object as an argument w.r.t. this object. This makes dependencies traceable for the user.
 */
class Nrf51822BluetoothStack : public BLEStack {
	// Friend for BLE stack event handling
	friend void SWI2_IRQHandler();

	// Friend for radio notification handling
	friend void ::SWI1_IRQHandler(); 

private:
	/* Constructor of the BLE stack on the NRF51822
	 *
	 * The constructor sets up very little! Only enough memory is allocated. Also there are a lot of defaults set. However,
	 * the SoftDevice is not enabled yet, nor any function on the SoftDevice is called. This is done in the init() 
	 * function.
	 */
	Nrf51822BluetoothStack(); 
	Nrf51822BluetoothStack(Nrf51822BluetoothStack const&); 
	void operator=(Nrf51822BluetoothStack const &); 

	/**
	 * The destructor shuts down the stack. 
	 *
	 * TODO: The SoftDevice should be disabled as well.
	 */
	virtual ~Nrf51822BluetoothStack();
public:
	static Nrf51822BluetoothStack& getInstance() {
		static Nrf51822BluetoothStack instance;
		return instance;
	}
	// Format of the callback when a connection has been made
	typedef function<void(uint16_t conn_handle)>   callback_connected_t;
	// Format of the callback after a disconnection event
	typedef function<void(uint16_t conn_handle)>   callback_disconnected_t;
	// Format of the callback of any radio event
	typedef function<void(bool radio_active)>   callback_radio_t;

	// Maximum number of services (currently set to 5)
	static const uint8_t MAX_SERVICE_COUNT = 5;

	//static const uint16_t                  defaultAppearance = BLE_APPEARANCE_UNKNOWN;
	
	// The default BLE appearance is currently set to a Generic Keyring (576)
	static const uint16_t                  defaultAppearance = BLE_APPEARANCE_GENERIC_KEYRING;
	// The low-frequency clock, currently generated from the high frequency clock
	static const nrf_clock_lfclksrc_t      defaultClockSource = NRF_CLOCK_LFCLKSRC_SYNTH_250_PPM;
	// The default MTU (Maximum Transmission Unit), 672 bytes is the default MTU, but can range from 48 bytes to 64kB.
	static const uint8_t                   defaultMtu = BLE_L2CAP_MTU_DEF;
	// Minimum connection interval in 1.25 ms (400*1.25=500ms)
	static const uint16_t                  defaultMinConnectionInterval_1_25_ms = 400;
	// Maximum connection interval in 1.25 ms (800*1.25=1 sec)
	static const uint16_t                  defaultMaxConnectionInterval_1_25_ms = 800;
	// Default slave latency
	static const uint16_t                  defaultSlaveLatencyCount = 0;
	// Connection timeout in 10ms (400*10=4 sec)
	static const uint16_t                  defaultConnectionSupervisionTimeout_10_ms = 400;
	// Advertising interval in 0.625 ms (40*0.625=25 ms)
	static const uint16_t                  defaultAdvertisingInterval_0_625_ms = 40;
	// Advertising timeout in seconds (180 sec)
	static const uint16_t                  defaultAdvertisingTimeout_seconds = 180;
	// Default transmission power
	static const int8_t                    defaultTxPowerLevel = -8;

protected:
	std::string                                 _device_name; // 4
	uint16_t                                    _appearance;
	fixed_tuple<Service*, MAX_SERVICE_COUNT>    _services;  // 32

	nrf_clock_lfclksrc_t                        _clock_source; //4
	uint8_t                                     _mtu_size;
	int8_t                                      _tx_power_level;
	uint8_t *                                   _evt_buffer;
	uint32_t                                    _evt_buffer_size;
	ble_gap_conn_sec_mode_t                     _sec_mode;  //1
	//ble_gap_sec_params_t                        _sec_params; //6
	//ble_gap_adv_params_t                        _adv_params; // 20
	uint16_t                                    _interval;
	uint16_t                                    _timeout;
	ble_gap_conn_params_t                       _gap_conn_params; // 8

	bool                                        _inited;
	bool                                        _started;
	bool                                        _advertising;
	bool                                        _scanning;

	uint16_t                                    _conn_handle;

	callback_connected_t                        _callback_connected;  // 16
	callback_disconnected_t                     _callback_disconnected;  // 16
	callback_radio_t                            _callback_radio;  // 16
	volatile uint8_t                            _radio_notify; // 0 = no notification (radio off), 1 = notify radio on, 2 = no notification (radio on), 3 = notify radio off.
public:
	/* Initialization of the BLE stack
	 *
	 * Performs a series of tasks:
	 *   - disables softdevice if it is currently enabled
	 *   - enables softdevice with own clock and assertion handler
	 *   - enable service changed characteristic for S110
	 *   - disable automatic address recycling for S110
	 *   - enable IRQ (SWI2_IRQn) for the softdevice
	 *   - set BLE device name
	 *   - set appearance (e.g. used in GUIs to interface with BLE devices)
	 *   - set connection parameters
	 *   - set Tx power level
	 *   - set the callback for BLE events (if we use Source/sd_common/softdevice_handler.c in Nordic's SDK)
	 */
	Nrf51822BluetoothStack& init();

	/* Start the BLE stack
	 *
	 * Start can only be called once. It starts all services. If one of these services cannot be started, there is 
	 * currently no exception handling. The stack does not start the Softdevice. This needs to be done before in 
	 * init().
	 */
	Nrf51822BluetoothStack& start();

	/* Shutdown the BLE stack
	 *
	 * The function shutdown() is the counterpart of start(). It does stop all services. It does not check if these 
	 * services have actually been started. 
	 *
	 * It will also stop advertising. The SoftDevice will not be shut down.
	 *
	 * After a shutdown() the function start() can be called again.
	 */
	Nrf51822BluetoothStack& shutdown();

	Nrf51822BluetoothStack& setAppearance(uint16_t appearance) {
		if (_inited) BLE_THROW("Already initialized.");
		_appearance = appearance;
		return *this;
	}

	Nrf51822BluetoothStack& setDeviceName(const std::string& deviceName) {
		if (_inited) BLE_THROW("Already initialized.");
		_device_name = deviceName;
		return *this;
	}

	Nrf51822BluetoothStack& setClockSource(nrf_clock_lfclksrc_t clockSource) {
		if (_inited) BLE_THROW("Already initialized.");
		_clock_source = clockSource;
		return *this;
	}

	Nrf51822BluetoothStack& setAdvertisingInterval(uint16_t advertisingInterval){
		// TODO: stop advertising?
		_interval = advertisingInterval;
		return *this;
	}

	Nrf51822BluetoothStack& setAdvertisingTimeoutSeconds(uint16_t advertisingTimeoutSeconds) {
		// TODO: stop advertising?
		_timeout = advertisingTimeoutSeconds;
		return *this;
	}

	/* Update device name
	 * @deviceName limited string for device name
	 *
	 * We want to change the device name halfway. This can be done through a characteristic, which is easy during
	 * development (you can separate otherwise similar devices). It is probably not functionality you want to have for
	 * the end-user.
	 */
	Nrf51822BluetoothStack& updateDeviceName(const std::string& deviceName);
	std::string & getDeviceName() { return _device_name; }

	/** Set radio transmit power in dBm (accepted values are -40, -20, -16, -12, -8, -4, 0, and 4 dBm). */
	Nrf51822BluetoothStack& setTxPowerLevel(int8_t powerLevel);

	int8_t getTxPowerLevel() {
		return _tx_power_level;
	}

	/** Set the minimum connection interval in units of 1.25 ms. */
	Nrf51822BluetoothStack& setMinConnectionInterval(uint16_t connectionInterval_1_25_ms) {
		if (_gap_conn_params.min_conn_interval != connectionInterval_1_25_ms) {
			_gap_conn_params.min_conn_interval = connectionInterval_1_25_ms;
			if (_inited) setConnParams();
		}
		return *this;
	}

	/** Set the maximum connection interval in units of 1.25 ms. */
	Nrf51822BluetoothStack& setMaxConnectionInterval(uint16_t connectionInterval_1_25_ms) {
		if (_gap_conn_params.max_conn_interval != connectionInterval_1_25_ms) {
			_gap_conn_params.max_conn_interval = connectionInterval_1_25_ms;
			if (_inited) setConnParams();
		}
		return *this;
	}

	/** Set the slave latency count. */
	Nrf51822BluetoothStack& setSlaveLatency(uint16_t slaveLatency) {
		if ( _gap_conn_params.slave_latency != slaveLatency ) {
			_gap_conn_params.slave_latency = slaveLatency;
			if (_inited) setConnParams();
		}
		return *this;
	}

	/** Set the connection supervision timeout in units of 10 ms. */
	Nrf51822BluetoothStack& setConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms) {
		if (_gap_conn_params.conn_sup_timeout != conSupTimeout_10_ms) {
			_gap_conn_params.conn_sup_timeout = conSupTimeout_10_ms;
			if (_inited) setConnParams();
		}
		return *this;
	}

	Nrf51822BluetoothStack& onConnect(const callback_connected_t& callback) {
		//if (callback && _callback_connected) BLE_THROW("Connected callback already registered.");

		_callback_connected = callback;
		if (_inited) setConnParams();
		return *this;
	}
	Nrf51822BluetoothStack& onDisconnect(const callback_disconnected_t& callback) {
		//if (callback && _callback_disconnected) BLE_THROW("Disconnected callback already registered.");

		_callback_disconnected = callback;

		return *this;
	}

	Service& createService() {
		GenericService* svc = new GenericService();
		addService(svc);
		return *svc;
	}

	BatteryService& createBatteryService() {
		BatteryService* svc = new BatteryService();
		addService(svc);
		return *svc;
	}

	Service& getService(std::string name);
	Nrf51822BluetoothStack& addService(Service* svc);

	/* Start advertising as an iBeacon
	 *
	 * @beacon the object defining the parameters for the
	 *   advertisement package. See <IBeacon> for an explanation
	 *   of the parameters and values
	 *
	 * Initiates the advertisement package and fills the manufacturing
	 * data array with the values of the <IBeacon> object, then starts
	 * advertising as an iBeacon.
	 *
	 * **Note**: An iBeacon requires that the company identifier is
	 *   set to the Apple Company ID, otherwise it's not an iBeacon.
	 */
	Nrf51822BluetoothStack& startIBeacon(IBeacon beacon);

	/* Start sending advertisement packets.
	 * This can not be called while scanning, start scanning while advertising is possible though.
	 */
	Nrf51822BluetoothStack& startAdvertising();

	Nrf51822BluetoothStack& stopAdvertising();

	bool isAdvertising();

	/* Start scanning for devices
	 *
	 * Only call the following functions with a S120 or S130 device that can play a central role. The following functions
	 * are probably the ones your recognize from implementing BLE functionality on Android or iOS if you are a smartphone
	 * developer.
	 */
	Nrf51822BluetoothStack& startScanning();

	/* Stop scanning for devices
	 */
	Nrf51822BluetoothStack& stopScanning();

	/* Returns true if currently scanning
	 */
	bool isScanning();

	/* Set radion notification interrupts
	 *
	 * Function that sets up radio notification interrupts. It sets the IRQ priority, enables it, and sets some 
	 * configuration values related to distance.
	 *
	 * Currently not used. 
	 */
	Nrf51822BluetoothStack& onRadioNotificationInterrupt(uint32_t distanceUs, callback_radio_t callback);

	virtual bool connected() {
		return _conn_handle != BLE_CONN_HANDLE_INVALID;
	}
	virtual uint16_t getConnectionHandle() {  // TODO are multiple connections supported?
		return _conn_handle;
	}

	/* Not time-critical functionality can be done in the tick
	 *
	 * Every module on the system gets a tick in which it regularly gets some attention. Of course, everything that is
	 * important should be done within interrupt handlers. 
	 *
	 * This function goes through the buffer and calls on_ble_evt for every BLE message in the buffer, till the buffer is
	 * empty. It then returns.
	 */
	void tick();

protected:
	void setTxPowerLevel();
	void setConnParams();

	/* Function that handles BLE events
	 *
	 * A BLE event is generated, these can be connect or disconnect events. It can also be RSSI values that changed, or
	 * an authorization request. Not all event structures are exactly the same over the different SoftDevices, so there
	 * are some defines for minor changes. And of course, e.g. the S110 softdevice cannot listen to advertisements at all,
	 * so BLE_GAP_EVT_ADV_REPORT is entirely disabled.
	 *
	 * TODO: Currently we loop through every service and send e.g. BLE_GATTS_EVT_WRITE only when some handle matches. It
	 * is faster to set up maps from handles to directly the right function.
	 */
	void on_ble_evt(ble_evt_t * p_ble_evt);

	/* Connection request
	 *
	 * On a connection request send it to all services.
	 */
	void on_connected(ble_evt_t * p_ble_evt);
	void on_disconnected(ble_evt_t * p_ble_evt);
	void on_advertisement(ble_evt_t * p_ble_evt);

	/* Transmission complete event
	 *
	 * Inform all services that transmission was completed in case they have notifications pending
	 */
	void onTxComplete(ble_evt_t * p_ble_evt);

};


} // end of namespace
