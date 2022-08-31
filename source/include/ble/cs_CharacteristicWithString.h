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

/** A characteristic for strings
 */
template <>
class Characteristic<std::string> : public CharacteristicGeneric<std::string> {
private:
	uint16_t _maxStringLength;

public:
	Characteristic<std::string>() : _maxStringLength(DEFAULT_CHAR_VALUE_STRING_LENGTH) {}

	/** @inherit */
	void operator=(const std::string& val) { CharacteristicGeneric<std::string>::operator=(val); }

	/** @inherit */
	uint8_t* getValuePtr() { return (uint8_t*)getValue().c_str(); }

	/** @inherit */
	uint16_t getValueLength() { return _maxStringLength; }

	void setMaxStringLength(uint16_t length) { _maxStringLength = length; }

	/** @inherit */
	uint16_t getGattValueLength() {
		if (this->isAesEnabled() && CharacteristicBase::_minAccessLevel != ENCRYPTION_DISABLED) {
			return ConnectionEncryption::getEncryptedBufferSize(_maxStringLength, ConnectionEncryptionType::CTR);
		}
		else {
			return getValueLength();
		}
	}

	/** @inherit */
	uint16_t getGattValueMaxLength() { return getGattValueLength(); }
};
