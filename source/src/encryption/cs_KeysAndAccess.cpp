/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 16, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_RNG.h>
#include <encryption/cs_KeysAndAccess.h>
#include <storage/cs_State.h>

#define LOGKeysAndAccessDebug LOGnone

KeysAndAccess::KeysAndAccess() {

}

void KeysAndAccess::init() {
	_encryptionEnabled = State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED);

	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	_operationMode = getOperationMode(mode);

	generateSetupKey();
}

bool KeysAndAccess::allowAccess(EncryptionAccessLevel minimum, EncryptionAccessLevel provided) {
	if (!_encryptionEnabled) {
		// always allow access when encryption is disabled.
		return true;
	}

	if (minimum == NOT_SET || minimum == NO_ONE) {
		return false;
	}

	if (minimum == ENCRYPTION_DISABLED) {
		return true;
	}

	if ((_operationMode == OperationMode::OPERATION_MODE_SETUP) && (provided == SETUP) && _setupKeyValid) {
		return true;
	}

	if (minimum == SETUP && (provided != SETUP || _operationMode != OperationMode::OPERATION_MODE_SETUP || !_setupKeyValid)) {
		LOGw("Setup mode required");
		return false;
	}

	// 0 is the highest possible value: ADMIN. If the provided is larger than the minimum, it is not allowed.
	if (provided > minimum) {
		LOGw("Insufficient access");
		return false;
	}

	return true;
}

bool KeysAndAccess::getKey(EncryptionAccessLevel accessLevel, buffer_ptr_t outBuf, cs_buffer_size_t outBufSize) {
	LOGKeysAndAccessDebug("getKey accessLevel=%u", accessLevel);
	if (outBufSize < ENCRYPTION_KEY_LENGTH) {
		LOGe("Buf size too small");
		return false;
	}

	CS_TYPE keyConfigType;
	switch (accessLevel) {
		case ADMIN:
			keyConfigType = CS_TYPE::CONFIG_KEY_ADMIN;
			break;
		case MEMBER:
			keyConfigType = CS_TYPE::CONFIG_KEY_MEMBER;
			break;
		case BASIC:
			keyConfigType = CS_TYPE::CONFIG_KEY_BASIC;
			break;
		case SERVICE_DATA:
			keyConfigType = CS_TYPE::CONFIG_KEY_SERVICE_DATA;
			break;
		case LOCALIZATION:
			keyConfigType = CS_TYPE::CONFIG_KEY_LOCALIZATION;
			break;
		case SETUP: {
			// Don't check if in setup mode: we want to use this for outgoing connections too.
			// The check _setupKeyValid == true should be enough.
			if (_setupKeyValid) {
				memcpy(outBuf, _setupKey, ENCRYPTION_KEY_LENGTH);
				return true;
			}
			// This error is generated once on boot
			LOGe("Can't use this setup key (yet)");
			return false;
		}
		default:
			LOGe("Invalid access level");
			return false;
	}

	State::getInstance().get(keyConfigType, outBuf, ENCRYPTION_KEY_LENGTH);
	return true;
}

cs_data_t KeysAndAccess::getSetupKey() {
	LOGKeysAndAccessDebug("getSetupKey valid=%u", _setupKeyValid);
	if (!_setupKeyValid) {
		return cs_data_t();
	}
	return cs_data_t(_setupKey, sizeof(_setupKey));
}

void KeysAndAccess::generateSetupKey() {
	LOGKeysAndAccessDebug("generateSetupKey opMode=%u", _operationMode);
	if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
		RNG::fillBuffer(_setupKey, SOC_ECB_KEY_LENGTH);
		_setupKeyValid = true;
	}
}

cs_ret_code_t KeysAndAccess::setSetupKey(cs_data_t data) {
	LOGKeysAndAccessDebug("Set setup key");
	if (data.len != sizeof(_setupKey)) {
		LOGw("Wrong key length: %u", data.len);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	memcpy(_setupKey, data.data, sizeof(_setupKey));
	_setupKeyValid = true;
	return ERR_SUCCESS;
}

void KeysAndAccess::invalidateSetupKey() {
	_setupKeyValid = false;
}

