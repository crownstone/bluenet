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
 * This template implements the functions specific for a pointer to a buffer.
 */
template <>
class Characteristic<buffer_ptr_t> : public CharacteristicGeneric<buffer_ptr_t> {

private:
	/**
	 * Maximum length for this characteristic.
	 * With encryption, this is the size of the encrypted buffer.
	 * Without encryption, this is the size of the value buffer.
	 */
	uint16_t _maxGattValueLength = 0;

	//! Actual length of (plain text) data stored in the buffer.
	uint16_t _valueLength = 0;

	//! Actual length of the (encrypted) data stored in the buffer.
	uint16_t _gattValueLength = 0;

	uint16_t _notificationPendingOffset = 0;

public:
	Characteristic<buffer_ptr_t>() {}

	uint32_t notify();

	// forward declaration
	void onTxComplete(const ble_common_evt_t* p_ble_evt);

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
	void setValueLength(uint16_t length) { _valueLength = length; }

	uint8_t* getValuePtr() { return getValue(); }

	uint16_t getValueLength() { return _valueLength; }

	/** Set the maximum length of the buffer
	 * @length maximum length in bytes
	 *
	 * This defines how many bytes were allocated for the buffer
	 * so how many bytes can max be stored in the buffer
	 */
	// todo ENCRYPTION: if aes encryption is enabled, maxGattValue length needs to be updated
	// could be automatically done based on the getValueLength and the overhead required for
	// encryption, or even a fixed value.
	void setMaxGattValueLength(uint16_t length) { _maxGattValueLength = length; }

	/** Return the maximum possible length of the buffer
	 *
	 * Checks the object assigned to this characteristics for the maximum
	 * possible length
	 *
	 * @return the maximum possible length
	 */
	// todo ENCRYPTION: max length of encrypted data
	uint16_t getGattValueMaxLength() { return _maxGattValueLength; }

	/** @inherit */
	virtual void setGattValueLength(uint16_t length) {
		if (_maxGattValueLength < length) {
			_gattValueLength = _maxGattValueLength;
		}
		_gattValueLength = length;
	}

	/** @inherit */
	virtual uint16_t getGattValueLength() { return _gattValueLength; }

protected:
};
