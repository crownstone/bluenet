/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 30, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <ble/cs_Service.h>
#include <ble/cs_UUID.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <common/cs_Types.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_KeysAndAccess.h>
#include <logging/cs_Logger.h>
#include <structs/buffer/cs_EncryptedBuffer.h>
#include <third/std/function.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>
#include <ble/cs_CharacteristicBase.h>

#define LOGCharacteristicDebug LOGvv
#define LogLevelCharacteristicDebug SERIAL_VERY_VERBOSE

/**
 * The templated version of ble_type
 * @param T The class / primitive to template ble_type with
 */
template <typename T>
inline uint8_t ble_type() {
	return BLE_GATT_CPF_FORMAT_STRUCT;
}
//! A ble_type for strings
template <>
inline uint8_t ble_type<std::string>() {
	return BLE_GATT_CPF_FORMAT_UTF8S;
}
//! A ble_type for 8-bit unsigned values
template <>
inline uint8_t ble_type<uint8_t>() {
	return BLE_GATT_CPF_FORMAT_UINT8;
}
//! A ble_type for 16-bit unsigned values
template <>
inline uint8_t ble_type<uint16_t>() {
	return BLE_GATT_CPF_FORMAT_UINT16;
}
//! A ble_type for 32-bit unsigned values
template <>
inline uint8_t ble_type<uint32_t>() {
	return BLE_GATT_CPF_FORMAT_UINT32;
}
//! A ble_type for 8-bit signed values
template <>
inline uint8_t ble_type<int8_t>() {
	return BLE_GATT_CPF_FORMAT_SINT8;
}
//! A ble_type for 16-bit signed values
template <>
inline uint8_t ble_type<int16_t>() {
	return BLE_GATT_CPF_FORMAT_SINT16;
}
//! A ble_type for 32-bit signed values
template <>
inline uint8_t ble_type<int32_t>() {
	return BLE_GATT_CPF_FORMAT_SINT32;
}
//! A ble_type for floats (32 bits)
template <>
inline uint8_t ble_type<float>() {
	return BLE_GATT_CPF_FORMAT_FLOAT32;
}
//! A ble_type for doubles (64 bits)
template <>
inline uint8_t ble_type<double>() {
	return BLE_GATT_CPF_FORMAT_FLOAT64;
}
//! A ble_type for booleans (8 bits)
template <>
inline uint8_t ble_type<bool>() {
	return BLE_GATT_CPF_FORMAT_BOOLEAN;
}

/**
 * Characteristic of generic type T
 *
 * A characteristic first of all contains a templated "value" which might be a string, an integer, or a
 * buffer, depending on the need at hand.
 * It allows also for callbacks to be defined on writing to the characteristic, or reading from the
 * characteristic.
 */
template <class T>
class CharacteristicGeneric : public CharacteristicBase {

	friend class Service;

public:
	//! Format of callback on write (from user)
	typedef function<void(CharacteristicBase* characteristic, const EncryptionAccessLevel, const T&, uint16_t length)> callback_on_write_t;

protected:
	//! The (plain text) value.
	T _value;

	//! The callback to call on a write coming from the softdevice (and originating from the user)
	callback_on_write_t _callbackOnWrite;

public:
	CharacteristicGeneric(){};

	//! Default empty destructor
	virtual ~CharacteristicGeneric(){};

	/** Return the value
	 *  In the case of aes encryption, this is the unencrypted value
	 */
	T& __attribute__((optimize("O0"))) getValue() { return _value; }

	/** Register an on write callback which will be triggered when a characteristic
	 *  is written over ble
	 */
	void onWrite(const callback_on_write_t& closure) { _callbackOnWrite = closure; }

	/** CharacteristicGeneric() returns value object
	 *  In the case of aes encryption, this is the unencrypted value
	 * @return value object
	 */
	operator T&() { return _value; }

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
	void onWrite(uint16_t len) {
		LOGd("onWrite %s", _name);

		setGattValueLength(len);

		EncryptionAccessLevel accessLevel = NOT_SET;
		// when using encryption, the packet needs to be decrypted
		if (_options.minAccessLevel <= ENCRYPTION_DISABLED) {

			// the arithmetic type- and the string type characteristics have their hardcoded length in the valueLength
			// the getValueLength() for those two types will always be the same and the setValueLength does nothing
			// there. In the case of a characteristic with a dynamic buffer length we need to set the length ourselves.
			// To do this we assume the length of the data is the same as the encrypted buffer minus the overhead for
			// the encryption. The result can be zero padded but generally the payload has it's own length indication.

			_log(LogLevelCharacteristicDebug,
				 false,
				 "gattPtr=%p gattLen=%u data=",
				 getGattValuePtr(),
				 getGattValueLength());
			_logArray(LogLevelCharacteristicDebug, true, getGattValuePtr(), getGattValueLength());

			uint16_t decryptionBufferLength =
					ConnectionEncryption::getPlaintextBufferSize(getGattValueLength(), ConnectionEncryptionType::CTR);
			setValueLength(decryptionBufferLength);

			cs_ret_code_t retCode = ConnectionEncryption::getInstance().decrypt(
					cs_data_t(getGattValuePtr(), getGattValueLength()),
					cs_data_t(getValuePtr(), getValueLength()),
					accessLevel,
					ConnectionEncryptionType::CTR);

			if (retCode != ERR_SUCCESS) {
				LOGi("failed to decrypt retCode=%u", retCode);
				ConnectionEncryption::getInstance().disconnect();
				return;
			}

			// disconnect on failure or if the user is not authenticated
			if (!KeysAndAccess::getInstance().allowAccess(_options.minAccessLevel, accessLevel)) {
				LOGi("insufficient access");
				ConnectionEncryption::getInstance().disconnect();
				return;
			}
		}
		else {
			accessLevel = ENCRYPTION_DISABLED;
			setValueLength(len);
		}

		_log(LogLevelCharacteristicDebug, false, "valuePtr=%p valueLen=%u data=", getValuePtr(), getValueLength());
		_logArray(LogLevelCharacteristicDebug, true, getValuePtr(), getValueLength());

		if (_callbackOnWrite) {
			_callbackOnWrite(this, accessLevel, getValue(), getValueLength());
		}
	}

	/** @inherit */
	virtual bool configurePresentationFormat(ble_gatts_char_pf_t& presentation_format) {
		presentation_format.format     = ble_type<T>();
		presentation_format.name_space = BLE_GATT_CPF_NAMESPACE_BTSIG;
		presentation_format.desc       = BLE_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN;
		presentation_format.exponent   = 1;
		return true;
	}

	/** Initialize / allocate a buffer for encryption */
	void initEncryptionBuffer() {
		if (_encryptionBuffer == NULL) {
			if (_options.sharedEncryptionBuffer) {
				uint16_t size;
				EncryptedBuffer::getInstance().getBuffer(_encryptionBuffer, size, CS_CHAR_BUFFER_DEFAULT_OFFSET);
				LOGCharacteristicDebug("%s: Use shared encryption buffer=%p size=%u", _name, _encryptionBuffer, size);
			}
			else {
				_encryptionBuffer = (buffer_ptr_t)calloc(getGattValueMaxLength(), sizeof(uint8_t));
				LOGCharacteristicDebug(
						"%s: Allocated encryption buffer=%p size=%u",
						_name,
						_encryptionBuffer,
						getGattValueMaxLength());
			}
		}
	}

	/** Free / release the encryption buffer */
	void freeEncryptionBuffer() {
		if (_encryptionBuffer != NULL) {
			if (_options.sharedEncryptionBuffer) {
				_encryptionBuffer = NULL;
			}
			else {
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
template <typename T, typename E = void>
class Characteristic : public CharacteristicGeneric<T> {
public:
	/** @inherit */
	Characteristic() : CharacteristicGeneric<T>() {}

	/** @inherit */
	void operator=(const T& val) { CharacteristicGeneric<T>::operator=(val); }
};

/** A characteristic for built-in arithmetic types (int, float, etc)
 */
template <typename T>
class Characteristic<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> : public CharacteristicGeneric<T> {
public:
	/** @inherit */
	void operator=(const T& val) { CharacteristicGeneric<T>::operator=(val); }

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
		}
		else {
			return getValueLength();
		}
	}

	/** @inherit */
	uint16_t getGattValueMaxLength() {
		return getGattValueLength();
	}
};


