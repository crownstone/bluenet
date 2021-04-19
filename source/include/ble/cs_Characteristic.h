/**
 * @file
 * Bluetooth Low Energy characteristics.
 *
 * @authors Crownstone Team, Christopher Mason
 * @copyright Crownstone B.V.
 * @date Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <ble/cs_Service.h>
#include <ble/cs_UUID.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_KeysAndAccess.h>
#include <structs/buffer/cs_EncryptionBuffer.h>
#include <third/std/function.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

/** General BLE name service
 *
 * All functionality that is just general BLE functionality is encapsulated in the BLEpp namespace.
 */
class Service;

/** CharacteristicInit collects fields required to define a BLE characteristic
 */
struct CharacteristicInit {
	ble_gatts_attr_t          attr_char_value;
	//! pointer to a presentation format structure (p_char_pf)
	ble_gatts_char_pf_t       presentation_format;
	//! characteristic metadata
	ble_gatts_char_md_t       char_md;
	//! attribute metadata for client characteristic configuration  (p_cccd_md)
	ble_gatts_attr_md_t       cccd_md;
	//! attributed metadata for server characteristic configuration (p_sccd_md)
	ble_gatts_attr_md_t       sccd_md;
	ble_gatts_attr_md_t       attr_md;
	//! attribute metadata for user description (p_user_desc_md)
	ble_gatts_attr_md_t       user_desc_metadata_md;

	CharacteristicInit() : presentation_format({}), char_md({}), cccd_md({}), attr_md({}) {}
};

#define STATUS_INITED

/** Status of a Characteristic.
 *
 * The status can be initialized, with notifications, writable, etc.
 */
struct Status {
	bool initialized                             : 1;
	bool notifies                                : 1; //! Whether this characteristic has notifications
	bool writable                                : 1;
	bool notifyingEnabled                        : 1; //! Whether server registered for notifications
	bool indicates                               : 1;
	bool aesEncrypted                            : 1;

	/** Flag to indicate if notification is pending to be sent once currently waiting
	 * tx operations are completed
	 */
	bool notificationPending                     : 1;
	/** shared encryption buffer, if false, a buffer is allocated for the characteristic, if true,
	 * the global EncryptionBuffer is used. In particular, big characteristics should use the global EncryptionBuffer
	 */
	bool sharedEncryptionBuffer                  : 1;
};

/** Non-template base class for Characteristics.
 *
 * A non-templated base class saves on code size. Note that every characteristic however does still
 * contribute to code size.
 */
class CharacteristicBase {

public:

protected:
	//! UUID of this characteristic.
	UUID                      _uuid;
	//! Name (4 bytes)
	const char *              _name;
	//! Read permission (1 byte)
	ble_gap_conn_sec_mode_t   _readperm;
	//! Write permission (1 byte)
	ble_gap_conn_sec_mode_t   _writeperm;
	//! Handles (8 bytes)
	ble_gatts_char_handles_t  _handles;
	//! Reference to corresponding service (4 bytes)
	Service*                  _service;

	//! Status of CharacteristicBase (basically a bunch of 1-bit flags)
	Status                    _status;

	buffer_ptr_t              _encryptionBuffer;

	/** used for encryption. If the characteristic is being read, it will encrypt itself with the lowest
	 * allowed userlevel key.
	 */
	EncryptionAccessLevel _minAccessLevel = ADMIN;

	//! Unit
//	uint16_t                  _unit;

public:
	/** Default constructor for CharacteristicBase
	 */
	CharacteristicBase();

	/** Empty destructor
	 */
	virtual ~CharacteristicBase() {}

	/** Initialize the characteristic.
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

	/** Set this characteristic to be writable.
	 */
	void setWritable(bool writable) {
		_status.writable = writable;
		setWritePermission(1, 1);
	}

	void setupWritePermissions(CharacteristicInit& ci);

	/** Set this characteristic to be notifiable.
	 */
	void setNotifies(bool notifies) {
		_status.notifies = notifies;
	}

	bool isNotifyingEnabled() {
		return _status.notifyingEnabled;
	}

	void setNotifyingEnabled(bool enabled) {
//		LOGd("[%s] notfying enabled: %s", _name.c_str(), enabled ? "true" : "false");
		_status.notifyingEnabled = enabled;
	}

	void setIndicates(bool indicates) {
		_status.indicates = indicates;
	}

	/** Security Mode 0 Level 0: No access permissions at all (this level is not defined by the Bluetooth Core specification).\n
	 * Security Mode 1 Level 1: No security is needed (aka open link).\n
	 * Security Mode 1 Level 2: Encrypted link required, MITM protection not necessary.\n
	 * Security Mode 1 Level 3: MITM protected encrypted link required.\n
	 * Security Mode 2 Level 1: Signing or encryption required, MITM protection not necessary.\n
	 * Security Mode 2 Level 2: MITM protected signing required, unless link is MITM protected encrypted.\n
	 */
	void setWritePermission(uint8_t securityMode, uint8_t securityLevel) {
		_writeperm.sm = securityMode;
		_writeperm.lv = securityLevel;
	}

	void setUUID(const UUID& uuid) {
		if (_status.initialized) BLE_THROW("Already inited.");
		_uuid = uuid;
	}

	void setName(const char * const name);

	const char* getName() {
		return _name;
	}

	uint16_t getValueHandle() {
		return _handles.value_handle;
	}

	uint16_t getCccdHandle() {
		return _handles.cccd_handle;
	}

	void setSharedEncryptionBuffer(bool val) {
		_status.sharedEncryptionBuffer = val;
	}

	/** Return the maximum length of the value used by the gatt server
	 *  In the case of aes encryption, this is the maximum length that
	 *  the value can be when encrypted. for normal types (arithmetic
	 *  types and strings) this is a fixed value, for dynamic types, the
	 *  max length has to be set at construction
	 *  In the case of no encryption, for normal types, this is the same
	 *  as the value length
	  */
	virtual uint16_t getGattValueMaxLength() = 0;

	/** Set the length of the value used by the gatt server
	 *  In the case of aes encryption, this is the length of the encrypted
	 *  value
	 *  otherwise it's the same as the value length which is a fixed value
	 *  for normal types (arithmetic types and strings) but has to be set
	 *  for buffer values because they have dynamic length
	 *
	 *  This is only necessary for buffer values because they have a
	 *  dynamic length. When a value is received over BT we need to update
	 *  the length of the data
	 */
	virtual void setGattValueLength(uint16_t ) {}

	/** Return the actual length of the value used by the gatt server
	 *  In the case of aes encryption, this is the length of the encrypted
	 *  value
	 *  otherwise it's the same as the value length
	 */
	virtual uint16_t getGattValueLength() = 0;

	/** Return the pointer to the memory where the value is accessed by the gatt server
	 *
	 *  In the case of aes encryption, this is the pointer to the memory with the encrypted
	 *  value
	 *  Otherwise, this will return the value of getValuePtr
	 *
	 *  For a basic data type return the address where the first byte
	 *  is stored
	 */
	uint8_t* getGattValuePtr() {
		if (this->isAesEnabled() && _minAccessLevel < ENCRYPTION_DISABLED) {
			return this->getEncryptionBuffer();
		} else {
			return getValuePtr();
		}
	}

	/** Return the pointer to the memory where the value is stored
	 *  In the case of aes encryption, this is the pointer to the unencrypted
	 *  value
	 */
	virtual uint8_t* getValuePtr() = 0;

	/** Set the actual length of the data
	 *  @length the length of the data to which the value points
	 *
	 *  In the case of aes encryption, this is the length of the unencrypted
	 *  data which is a fixed value for normal types, but dynamic for buffer
	 *  types. so it has to be set when writing/updating the characteristic.
	 *
	 *  This is only necessary for buffer values because they have a
	 *  dynamic length. When a value is received over BT we need to update
	 *  the length of the data. same if we update a value to be read, the
	 *  length of the available data has to be set.
	 */
	virtual void setValueLength(uint16_t ) {};

	/** Return the actual length of the value
	 *  In the case of aes encryption, this is the length of the unencrypted
	 *  data.
	 *  For normal types (arithmetic and strings) this is a fixed value. For
	 *  buffer types it is dynamic
	 */
	virtual uint16_t getValueLength() = 0;

	/** Helper function which is called when a characteristic is written over
	 *  BLE.
	 *  @len the number of bytes that were written
	 */
	virtual void written(uint16_t len) = 0;

	/** Update the value in the gatt server so that the value can be read over BLE
	 *  If somebody is also listening to notifications for the characteristic
	 *  notifications will be sent.
	 */
	uint32_t updateValue(ConnectionEncryptionType encryptionType = ConnectionEncryptionType::CTR);

	/** Notify any listening party.
	 *
	 * If this characteristic can notify, and if notification is enabled, and if there is a connection calling
	 * this function will notify the listening party.
	 *
	 * Notification quickly fills an outgoing buffer in BLE. When this buffer gets full, an error code
	 * <BLE_ERROR_NO_TX_BUFFERS> is generated by the SoftDevice. In that case <onNotifyTxError> is called.
	 *
	 * @return err_code (which should be NRF_SUCCESS if no error occurred)
	 */
	virtual uint32_t notify();

	/** Callback function if a notify tx error occurs
	 *
	 * This is called when the notify operation fails with a tx error. This
	 * can occur when too many tx operations are taking place at the same time.
	 *
	 * A <BLEpp::CharacteristicGenericBase::notify> is called when the master device
	 * connected to the Crownstone requests automatic notifications whenever
	 * the value changes.
	 */
	void onNotifyTxError() {
		_status.notificationPending = true;
	}

	/** Callback function once tx operations complete
	 * @p_ble_evt the event object which triggered the onTxComplete callback
	 *
	 * This is called whenever tx operations complete. If a notification is pending
	 * <BLEpp::CharacteristicGenericBase::notify> is called again and the notification
	 * is cleared if the call eas successful. If not successful, it will be tried
	 * again during the next callback call
	 */
	virtual void onTxComplete(const ble_common_evt_t * p_ble_evt);

	/** Enable / Disable aes encryption. If enabled, a buffer is created to hold the encrypted
	 *  value. If disabled, the buffer is freed again.
	 */
	void setAesEncrypted(bool encrypted) {
		_status.aesEncrypted = encrypted;
		if (encrypted) {
			initEncryptionBuffer();
		} else {
			freeEncryptionBuffer();
		}
	}

	/** Check if aes encryption is enabled */
	bool isAesEnabled() {
		return _status.aesEncrypted;
	}



	void setMinAccessLevel(EncryptionAccessLevel level) {
		_minAccessLevel = level;
	}


	/** get the buffer used for encryption */
	buffer_ptr_t& getEncryptionBuffer() {
		return _encryptionBuffer;
	}

	Service* getService() {
		return _service;
	}

protected:

	/** Initialize the encryption buffer to hold the encrypted value. */
	virtual void initEncryptionBuffer() = 0;

	/** Free / release the encryption buffer */
	virtual void freeEncryptionBuffer() = 0;

	/** Configure the characteristic value format (ble specific) */
	virtual bool configurePresentationFormat(ble_gatts_char_pf_t &) { return false; }

};

/** The templated version of ble_type
 * @param T The class / primitive to template ble_type with
 */
template<typename T>
inline uint8_t ble_type() {
	return BLE_GATT_CPF_FORMAT_STRUCT;
}
//! A ble_type for strings
template<>
inline uint8_t ble_type<std::string>() {
	return BLE_GATT_CPF_FORMAT_UTF8S;
}
//! A ble_type for 8-bit unsigned values
template<>
inline uint8_t ble_type<uint8_t>() {
	return BLE_GATT_CPF_FORMAT_UINT8;
}
//! A ble_type for 16-bit unsigned values
template<>
inline uint8_t ble_type<uint16_t>() {
	return BLE_GATT_CPF_FORMAT_UINT16;
}
//! A ble_type for 32-bit unsigned values
template<>
inline uint8_t ble_type<uint32_t>() {
	return BLE_GATT_CPF_FORMAT_UINT32;
}
//! A ble_type for 8-bit signed values
template<>
inline uint8_t ble_type<int8_t>() {
	return BLE_GATT_CPF_FORMAT_SINT8;
}
//! A ble_type for 16-bit signed values
template<>
inline uint8_t ble_type<int16_t>() {
	return BLE_GATT_CPF_FORMAT_SINT16;
}
//! A ble_type for 32-bit signed values
template<>
inline uint8_t ble_type<int32_t>() {
	return BLE_GATT_CPF_FORMAT_SINT32;
}
//! A ble_type for floats (32 bits)
template<>
inline uint8_t ble_type<float>() {
	return BLE_GATT_CPF_FORMAT_FLOAT32;
}
//! A ble_type for doubles (64 bits)
template<>
inline uint8_t ble_type<double>() {
	return BLE_GATT_CPF_FORMAT_FLOAT64;
}
//! A ble_type for booleans (8 bits)
template<>
inline uint8_t ble_type<bool>() {
	return BLE_GATT_CPF_FORMAT_BOOLEAN;
}

/** Characteristic of generic type T
 * @param T Generic type T
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
	//! Format of callback on write (from user)
	typedef function<void(const EncryptionAccessLevel, const T&, uint16_t length)> callback_on_write_t;

protected:
	/** The generic type is physically located in this field in this class (by value, not just by reference)
	 *  In the case of aes encryption, this is the unencrypted value
	 */
	T                          _value;

	//! The callback to call on a write coming from the softdevice (and originating from the user)
	callback_on_write_t        _callbackOnWrite;

public:
	CharacteristicGeneric() {};

	//! Default empty destructor
	virtual ~CharacteristicGeneric() {};

	/** Return the value
	 *  In the case of aes encryption, this is the unencrypted value
	 */
	T&  __attribute__((optimize("O0"))) getValue() {
		return _value;
	}

	/** Register an on write callback which will be triggered when a characteristic
	 *  is written over ble
	 */
	void onWrite(const callback_on_write_t& closure) {
		_callbackOnWrite = closure;
	}

	/** CharacteristicGeneric() returns value object
	 *  In the case of aes encryption, this is the unencrypted value
	 * @return value object
	 */
	operator T&() {
		return _value;
	}

	/** Assign a new value to the characteristic so that it can be read over ble.
	 *  In the case of aes encryption, pass the unencrypted value, which will then be encrypted
	 *  and updated at the gatt server
	 *
	 *  TODO: Alex - why don't we just use setValue for consistency??
	 */
	void operator=(const T& val) {
		_value = val;
		updateValue();
	}

	/** Set the default value */
	void setDefaultValue(const T& t) {
		if (_status.initialized) BLE_THROW("Already inited.");
		_value = t;
	}

protected:

	/** Helper function if a characteristic is written over ble.
	 *  updates the length values for dynamic length types, decrypts the value if
	 *  aes encryption enabled. then calls the on write callback.
	 */
	void written(uint16_t len) {
		// We can flood the chip with writes and a potential forced disconnect will be delayed and could crash the chip.
		//TODO: have this from the stack directly.
		if (ConnectionEncryption::getInstance().allowedToWrite()) {
			LOGi("Not allowed to write, disconnect in progress");
			return;
		}

		setGattValueLength(len);

		EncryptionAccessLevel accessLevel = NOT_SET;
		// when using encryption, the packet needs to be decrypted
		if (_status.aesEncrypted && _minAccessLevel < ENCRYPTION_DISABLED) {

			// the arithmetic type- and the string type characteristics have their hardcoded length in the valueLength
			// the getValueLength() for those two types will always be the same and the setValueLength does nothing there.
			// In the case of a characteristic with a dynamic buffer length we need to set the length ourselves.
			// To do this we assume the length of the data is the same as the encrypted buffer minus the overhead for the encryption.
			// The result can be zero padded but generally the payload has it's own length indication.
			uint16_t decryptionBufferLength = ConnectionEncryption::getPlaintextBufferSize(getGattValueLength(), ConnectionEncryptionType::CTR);
			setValueLength(decryptionBufferLength);

			cs_ret_code_t retCode = ConnectionEncryption::getInstance().decrypt(
				cs_data_t(getGattValuePtr(), getGattValueLength()),
				cs_data_t(getValuePtr(), getValueLength()),
				accessLevel,
				ConnectionEncryptionType::CTR
			);


			if (retCode != ERR_SUCCESS) {
				LOGi("failed to decrypt retCode=%u", retCode);
				ConnectionEncryption::getInstance().disconnect();
				return;
			}

			// disconnect on failure or if the user is not authenticated
			if (!KeysAndAccess::getInstance().allowAccess(_minAccessLevel , accessLevel)) {
				LOGi("insufficient access");
				ConnectionEncryption::getInstance().disconnect();
				return;
			}
		}
		else {
			accessLevel = ENCRYPTION_DISABLED;
			setValueLength(len);
		}



		LOGd("%s: onWrite()", _name);

		_callbackOnWrite(accessLevel, getValue(), getValueLength());
	}


	/** @inherit */
	virtual bool configurePresentationFormat(ble_gatts_char_pf_t& presentation_format) {
		presentation_format.format = ble_type<T>();
		presentation_format.name_space = BLE_GATT_CPF_NAMESPACE_BTSIG;
		presentation_format.desc = BLE_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN;
		presentation_format.exponent = 1;
		return true;
	}

	/** Initialize / allocate a buffer for encryption */
	void initEncryptionBuffer() {
		if (_encryptionBuffer == NULL) {
			if (_status.sharedEncryptionBuffer) {
				uint16_t size;
				EncryptionBuffer::getInstance().getBuffer(_encryptionBuffer, size);
				assert(_encryptionBuffer != NULL, "need to initialize encryption buffer for aes encryption");
			} else {
				_encryptionBuffer = (buffer_ptr_t)calloc(getGattValueMaxLength(), sizeof(uint8_t));
			}
		}
	}

	/** Free / release the encryption buffer */
	void freeEncryptionBuffer() {
		if (_encryptionBuffer != NULL) {
			if (_status.sharedEncryptionBuffer) {
				_encryptionBuffer = NULL;
			} else {
				free(_encryptionBuffer);
				_encryptionBuffer = NULL;
			}
		}
	}

private:
};

/** A default characteristic
 * @param T type of the value
 * @param E default type (subdefined for example for built-in types)
 *
 * Comes with an assignment operator, so we can assign characteristics to each other which copies the internal value.
 */
template<typename T, typename E = void>
class Characteristic : public CharacteristicGeneric<T> {
public:
	/** @inherit */
	Characteristic() : CharacteristicGeneric<T>() {}

	/** @inherit */
	void operator=(const T& val) {
		CharacteristicGeneric<T>::operator=(val);
	}

};

/** A characteristic for built-in arithmetic types (int, float, etc)
 */
template<typename T >
class Characteristic<T, typename std::enable_if<std::is_arithmetic<T>::value >::type> : public CharacteristicGeneric<T> {
public:
	/** @inherit */
	void operator=(const T& val) {
		CharacteristicGeneric<T>::operator=(val);
	}

#ifdef ADVANCED_OPERATORS
	void operator+=(const T& val) {
		_value += val;

		updateValue();

	}

	void operator-=(const T& val) {
		_value -= val;

		updateValue();

	}

	void operator*=(const T& val) {
		_value *= val;

		updateValue();

	}

	void operator/=(const T& val) {
		_value /= val;

		updateValue();

	}
#endif

	/** @inherit */
	uint8_t* getValuePtr() {
		return (uint8_t*)&this->getValue();
	}

	/** @inherit */
	uint16_t getValueLength() {
		return sizeof(T);
	}

	/** @inherit */
	uint16_t getGattValueLength() {
		if (this->isAesEnabled() && CharacteristicBase::_minAccessLevel != ENCRYPTION_DISABLED) {
			return ConnectionEncryption::getEncryptedBufferSize(sizeof(T), ConnectionEncryptionType::CTR);
		} else {
			return getValueLength();
		}
	}

	/** @inherit */
	uint16_t getGattValueMaxLength() {
		return getGattValueLength();
	}

};

/** A characteristic for strings
 */
template<>
class Characteristic<std::string> : public CharacteristicGeneric<std::string> {
private:

	uint16_t _maxStringLength;

public:

	Characteristic<std::string>() : _maxStringLength(DEFAULT_CHAR_VALUE_STRING_LENGTH) {

	}


	/** @inherit */
	void operator=(const std::string& val) {
		CharacteristicGeneric<std::string>::operator=(val);
	}

	/** @inherit */
	uint8_t* getValuePtr() {
		return (uint8_t*)getValue().c_str();
	}

	/** @inherit */
	uint16_t getValueLength() {
		return _maxStringLength;
	}

	void setMaxStringLength(uint16_t length) {
		_maxStringLength = length;
	}

	/** @inherit */
	uint16_t getGattValueLength() {
		if (this->isAesEnabled() && CharacteristicBase::_minAccessLevel != ENCRYPTION_DISABLED) {
			return ConnectionEncryption::getEncryptedBufferSize(_maxStringLength, ConnectionEncryptionType::CTR);
		} else {
			return getValueLength();
		}
	}

	/** @inherit */
	uint16_t getGattValueMaxLength() {
		return getGattValueLength();
	}

};

/** This template implements the functions specific for a pointer to a buffer.
 */
template<>
class Characteristic<buffer_ptr_t> : public CharacteristicGeneric<buffer_ptr_t> {

private:
	/** maximum length for this characteristic in bytes
	 *  In the case of aes encryption, this is the maximum number of bytes of the encrypted value
	 */
	uint16_t _maxGattValueLength;

	/** actual length of data stored in the buffer in bytes
	 *  In the case of aes encryption, this is the number of bytes of the unencrypted value
	 */
	uint16_t _valueLength;

	uint16_t _gattValueLength;

	uint16_t _notificationPendingOffset;

public:

	Characteristic<buffer_ptr_t>() : _maxGattValueLength(0), _valueLength(0), _gattValueLength(0),
		_notificationPendingOffset(0) {
		setSharedEncryptionBuffer(true);
	}

	uint32_t notify();

	// forward declaration
	void onTxComplete(const ble_common_evt_t * p_ble_evt);

	/** Set the value for this characteristic
	 * @value the pointer to the buffer in memory
	 *
	 * only valid pointers are allowed, NULL is NOT allowed
	 */
	void setValue(const buffer_ptr_t& value) {
		assert(value, "Error: Don't assign NULL pointers! Pointer to buffer has to be valid from the start!");
		_value = value;
	}

	/** Set the length of data stored in the buffer
	 * @length length of data in bytes
	 */
	void setValueLength(uint16_t length) {
		_valueLength = length;
	}

	uint8_t* getValuePtr() {
		return getValue();
	}

	uint16_t getValueLength() {
		return _valueLength;
	}

	/** Set the maximum length of the buffer
	 * @length maximum length in bytes
	 *
	 * This defines how many bytes were allocated for the buffer
	 * so how many bytes can max be stored in the buffer
	 */
	// todo ENCRYPTION: if aes encryption is enabled, maxGattValue length needs to be updated
	// could be automatically done based on the getValueLength and the overhead required for
	// encryption, or even a fixed value.
	void setMaxGattValueLength(uint16_t length) {
		_maxGattValueLength = length;
	}

	/** Return the maximum possible length of the buffer
	 *
	 * Checks the object assigned to this characteristics for the maximum
	 * possible length
	 *
	 * @return the maximum possible length
	 */
	// todo ENCRYPTION: max length of encrypted data
	uint16_t getGattValueMaxLength() {
		return _maxGattValueLength;
	}

	/** @inherit */
	virtual void setGattValueLength(uint16_t length) {
		_gattValueLength = length;
	}

	/** @inherit */
	virtual uint16_t getGattValueLength() {
		return _gattValueLength;
	}
//
//	void initEncryptionBuffer() {
//		if (_encryptionBufferUsed) {
//
//		} else {
//			uint16_t size;
//			EncryptionBuffer::getInstance().getBuffer(CharacteristicBase::_encryptionBuffer, size);
//			assert(CharacteristicBase::_encryptionBuffer != NULL, "need to initialize encryption buffer for aes encryption");
//		}
//	}
//
//	void freeEncryptionBuffer() {
//		CharacteristicBase::_encryptionBuffer = NULL;
//	}

protected:

};

