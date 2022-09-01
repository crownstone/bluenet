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

/**
 * Characteristic of generic type T.
 */
template <class T>
class Characteristic : public CharacteristicBase {
public:
	Characteristic() {
		setValueBuffer(reinterpret_cast<buffer_ptr_t>(&_value), sizeof(_value));
	};

	virtual ~Characteristic() {};

	/**
	 * Set the initial value.
	 * Must be set before init.
	 */
	cs_ret_code_t setInitialValue(const T& value) {
		cs_ret_code_t retCode = setInitialValueLength(sizeof(_value));
		if (retCode != ERR_SUCCESS) {
			return retCode;
		}
		_value = value;
		return ERR_SUCCESS;
	}

	/**
	 * Return the (plain text) value
	 */
	T& getValue() { return _value; }

	/**
	 * Set the new (plain text) value.
	 */
	cs_ret_code_t setValue(const T& value) {
		_value = value;
		return updateValue(sizeof(_value));
	}

protected:
	//! The (plain text) value.
	T _value;
};

/**
 * The base class is already using a buffer as value
 */
template <>
class Characteristic<buffer_ptr_t> : public CharacteristicBase {

};

/**
 * A string value.
 */
template <>
class Characteristic<const char*> : public CharacteristicBase {
private:
	//! 0 terminated string.
	char _value[DEFAULT_CHAR_VALUE_STRING_LENGTH];

public:
	Characteristic() {
		setValueBuffer(reinterpret_cast<uint8_t*>(_value), DEFAULT_CHAR_VALUE_STRING_LENGTH);
	}

	/**
	 * Set the initial value.
	 * Must be set before init.
	 */
	cs_ret_code_t setInitialValue(const char* value) {
		uint16_t size = strlen(value);
		cs_ret_code_t retCode = setInitialValueLength(size);
		if (retCode != ERR_SUCCESS) {
			return retCode;
		}
		memcpy(_value, value, size);
		return ERR_SUCCESS;
	}

	/**
	 * Set the new (plain text) value.
	 */
	cs_ret_code_t setValue(const char* value) {
		uint16_t size = strlen(value);
		if (size > sizeof(_value)) {
			return ERR_BUFFER_TOO_SMALL;
		}
		memcpy(_value, value, size);
		return updateValue(size);
	}

	/**
	 * Return the (plain text) value
	 */
	const char* getValue() { return _value; }
};


