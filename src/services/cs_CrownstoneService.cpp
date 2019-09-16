/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <services/cs_CrownstoneService.h>

#include <storage/cs_State.h>
#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include <protocol/cs_ErrorCodes.h>

CrownstoneService::CrownstoneService() : EventListener(),
_controlCharacteristic(NULL),
_configurationControlCharacteristic(NULL),
_configurationReadCharacteristic(NULL),
_streamBuffer(NULL),
_stateControlCharacteristic(NULL),
_stateReadCharacteristic(NULL),
_sessionNonceCharacteristic(NULL),
_factoryResetCharacteristic(NULL)
{
	EventDispatcher::getInstance().addListener(this);
	LOGi("Add crownstone service");

	setUUID(UUID(CROWNSTONE_UUID));
	setName(BLE_SERVICE_CROWNSTONE);
}

StreamBuffer<uint8_t>* CrownstoneService::getStreamBuffer(buffer_ptr_t& buffer, uint16_t& maxLength) {
	// if it is not created yet, create a new stream buffer and assign the master buffer to it
	if (_streamBuffer == NULL) {
		_streamBuffer = new StreamBuffer<uint8_t>();

		MasterBuffer& mb = MasterBuffer::getInstance();
		uint16_t size = 0;
		mb.getBuffer(buffer, size);

//		LOGd("Assign buffer of size %i to stream buffer", size);
		_streamBuffer->assign(buffer, size);
		maxLength = _streamBuffer->getMaxLength();
	} else {
		// otherwise use the same buffer
		_streamBuffer->getBuffer(buffer, maxLength);
		maxLength = _streamBuffer->getMaxLength();
	}
	return _streamBuffer;
}

void CrownstoneService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_CROWNSTONE);

	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;

#if CHAR_CONTROL==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_CONTROL);
	_streamBuffer = getStreamBuffer(buffer, maxLength);
	addControlCharacteristic(buffer, maxLength, CONTROL_UUID);
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_CONTROL);
#endif

#if CHAR_CONFIGURATION==1
	{
		LOGi(FMT_CHAR_ADD, STR_CHAR_CONFIGURATION);
		_streamBuffer = getStreamBuffer(buffer, maxLength);
		addConfigurationControlCharacteristic(buffer, maxLength, ADMIN);
		addConfigurationReadCharacteristic(buffer, maxLength, ADMIN);
	}
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_CONFIGURATION);
#endif

#if CHAR_STATE==1
	{
		// TODO: Does this not conflict with CHAR_CONFIGURATION?
		LOGi(FMT_CHAR_ADD, STR_CHAR_STATE);
		_streamBuffer = getStreamBuffer(buffer, maxLength);
		addStateControlCharacteristic(buffer, maxLength);
		addStateReadCharacteristic(buffer, maxLength);
	}
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_STATE);
#endif

	LOGi(FMT_CHAR_ADD, STR_CHAR_SESSION_NONCE);
	addSessionNonceCharacteristic(_nonceBuffer, maxLength);

	LOGi(FMT_CHAR_ADD, STR_CHAR_FACTORY_RESET);
	addFactoryResetCharacteristic();

	updatedCharacteristics();
}

void CrownstoneService::addControlCharacteristic(buffer_ptr_t buffer, uint16_t size, uint16_t charUuid, EncryptionAccessLevel minimumAccessLevel) {
	if (_controlCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_CONTROL);
		return;
	}
	_controlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_controlCharacteristic);
	_controlCharacteristic->setUUID(UUID(getUUID(), charUuid));
	_controlCharacteristic->setName(BLE_CHAR_CONTROL);
	_controlCharacteristic->setWritable(true);
	_controlCharacteristic->setNotifies(true);
	_controlCharacteristic->setValue(buffer);
	_controlCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_controlCharacteristic->setMaxGattValueLength(size);
	_controlCharacteristic->setValueLength(0);
	_controlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length) -> void {
		// encryption in the write stage verifies if the key is at least GUEST, command specific permissions are
		// handled in the commandHandler
		cs_ret_code_t errCode;
		CommandHandlerTypes type = CTRL_CMD_UNKNOWN;
		MasterBuffer& mb = MasterBuffer::getInstance();
		// at this point it is too late to check if mb was locked, because the softdevice doesn't care
		// if the mb was locked, it writes to the buffer in any case
		if (!mb.isLocked()) {
			LOGi(MSG_CHAR_VALUE_WRITE);
			type = (CommandHandlerTypes) _streamBuffer->type();
			uint8_t *payload = _streamBuffer->payload();
			uint8_t payloadLength = _streamBuffer->length();
			if (length < _streamBuffer->getDataLength()) {
				LOGw("len=%u  sb len=%u", length, _streamBuffer->getDataLength());
				errCode = ERR_WRONG_PAYLOAD_LENGTH;
			}
			else {
				errCode = CommandHandler::getInstance().handleCommand(type, payload, payloadLength, cmd_source_t(CS_CMD_SOURCE_CONNECTION), accessLevel);
			}

			mb.unlock();
		}
		else {
			LOGe(MSG_BUFFER_IS_LOCKED);
			errCode = ERR_BUFFER_LOCKED;
		}
		// App doesn't use the error code, except for some commands.
		// When writing multiple control commands, the return code sometimes overwrites the command.
		// So only write the return code for some commands.
		switch (type) {
		case CTRL_CMD_SETUP:
		case CTRL_CMD_FACTORY_RESET:
			controlWriteErrorCode(type, errCode);
			break;
		default:
			break;
		}
	});
}

void CrownstoneService::addConfigurationControlCharacteristic(buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel minimumAccessLevel) {
	if (_configurationControlCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_CONFIGURATION);
		return;
	}
	_configurationControlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_configurationControlCharacteristic);
	_configurationControlCharacteristic->setUUID(UUID(getUUID(), CONFIG_CONTROL_UUID));
	_configurationControlCharacteristic->setName(BLE_CHAR_CONFIG_CONTROL);
	_configurationControlCharacteristic->setWritable(true);
	_configurationControlCharacteristic->setValue(buffer);
	_configurationControlCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_configurationControlCharacteristic->setMaxGattValueLength(size);
	_configurationControlCharacteristic->setValueLength(0);
	_configurationControlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length) -> void {
		bool writeErrCode = true;
		cs_ret_code_t errCode = configOnWrite(accessLevel, value, length, writeErrCode);
		if (writeErrCode) {
			// Leave type as is.
			_streamBuffer->setOpCode(OPCODE_ERR_VALUE);
			_streamBuffer->setPayload((uint8_t*)&errCode, sizeof(errCode));
			_configurationControlCharacteristic->setValueLength(_streamBuffer->getDataLength());
			_configurationControlCharacteristic->updateValue();
		}
	});
}


void CrownstoneService::addConfigurationReadCharacteristic(buffer_ptr_t buffer, uint16_t size,
		EncryptionAccessLevel minimumAccessLevel) {
	if (_configurationReadCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, BLE_CHAR_CONFIG_READ);
		return;
	}
	_configurationReadCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_configurationReadCharacteristic);
	_configurationReadCharacteristic->setUUID(UUID(getUUID(), CONFIG_READ_UUID));
	_configurationReadCharacteristic->setName(BLE_CHAR_CONFIG_READ);
	_configurationReadCharacteristic->setWritable(false);
	_configurationReadCharacteristic->setNotifies(true);
	_configurationReadCharacteristic->setValue(buffer);
	_configurationReadCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_configurationReadCharacteristic->setMaxGattValueLength(size);
	_configurationReadCharacteristic->setValueLength(0);
}


void CrownstoneService::addStateControlCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	if (_stateControlCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_STATE_CONTROL);
		return;
	}
	_stateControlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_stateControlCharacteristic);
	_stateControlCharacteristic->setUUID(UUID(getUUID(), STATE_CONTROL_UUID));
	_stateControlCharacteristic->setName(BLE_CHAR_STATE_CONTROL);
	_stateControlCharacteristic->setWritable(true);
	_stateControlCharacteristic->setValue(buffer);
	_stateControlCharacteristic->setMinAccessLevel(MEMBER);
	_stateControlCharacteristic->setMaxGattValueLength(size);
	_stateControlCharacteristic->setValueLength(0);
	_stateControlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length) -> void {
		bool writeErrCode = true;
		cs_ret_code_t errCode = stateOnWrite(accessLevel, value, length, writeErrCode);
		if (writeErrCode) {
			// Leave type as is.
			_streamBuffer->setOpCode(OPCODE_ERR_VALUE);
			_streamBuffer->setPayload((uint8_t*)&errCode, sizeof(errCode));
			_stateControlCharacteristic->setValueLength(_streamBuffer->getDataLength());
			_stateControlCharacteristic->updateValue();
		}
	});
}

void CrownstoneService::addStateReadCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	if (_stateReadCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_STATE);
		return;
	}

	_stateReadCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_stateReadCharacteristic);
	_stateReadCharacteristic->setUUID(UUID(getUUID(), STATE_READ_UUID));
	_stateReadCharacteristic->setName(BLE_CHAR_STATE_READ);
	_stateReadCharacteristic->setWritable(false);
	_stateReadCharacteristic->setNotifies(true);
	_stateReadCharacteristic->setValue(buffer);
	_stateReadCharacteristic->setMinAccessLevel(MEMBER);
	_stateReadCharacteristic->setMaxGattValueLength(size);
	_stateReadCharacteristic->setValueLength(0);
}


void CrownstoneService::addSessionNonceCharacteristic(buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel minimumAccessLevel) {
	if (_sessionNonceCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_SESSION_NONCE);
		return;
	}
	_sessionNonceCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_sessionNonceCharacteristic);
	_sessionNonceCharacteristic->setUUID(UUID(getUUID(), SESSION_NONCE_UUID));
	_sessionNonceCharacteristic->setName(BLE_CHAR_SESSION_NONCE);
	_sessionNonceCharacteristic->setWritable(false);
	_sessionNonceCharacteristic->setNotifies(true);
	_sessionNonceCharacteristic->setValue(buffer);
	_sessionNonceCharacteristic->setSharedEncryptionBuffer(false);
	_sessionNonceCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_sessionNonceCharacteristic->setMaxGattValueLength(size);
	_sessionNonceCharacteristic->setValueLength(0);
}

void CrownstoneService::addFactoryResetCharacteristic() {
	_factoryResetCharacteristic = new Characteristic<uint32_t>();
	addCharacteristic(_factoryResetCharacteristic);
	_factoryResetCharacteristic->setUUID(UUID(getUUID(), FACTORY_RESET_UUID));
	_factoryResetCharacteristic->setName(BLE_CHAR_FACTORY_RESET);
	_factoryResetCharacteristic->setWritable(true);
	_factoryResetCharacteristic->setNotifies(true);
	_factoryResetCharacteristic->setDefaultValue(0);
	_factoryResetCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_factoryResetCharacteristic->onWrite([&](const uint8_t accessLevel, const uint32_t& value, uint16_t length) -> void {
		// TODO if settings --> factory reset disabled, we set the value to 2 to indicate reset is not possible.
		// No need to check length, as value is not a pointer.
		bool success = FactoryReset::getInstance().recover(value);
		uint32_t val = 0;
		if (success) {
			val = 1;
		}
		_factoryResetCharacteristic->operator=(val);
	});
}

/*
 * Encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
 * This is only for characteristics that the user writes to. The ones that are read are encrypted using the
 * setMinAccessLevel level.
 * If the user writes to this characteristic with insufficient rights, this method will not be called.
 */
cs_ret_code_t CrownstoneService::configOnWrite(const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length, bool& writeErrCode) {
	if (!value) {
		LOGw(MSG_CHAR_VALUE_UNDEFINED);
		return ERR_BUFFER_UNASSIGNED;
	}
	LOGi(MSG_CHAR_VALUE_WRITE);
	MasterBuffer& mb = MasterBuffer::getInstance();
	// at this point it is too late to check if mb was locked, because the softdevice doesn't care
	// if the mb was locked, it writes to the buffer in any case
	if (mb.isLocked()) {
		LOGe(MSG_BUFFER_IS_LOCKED);
		return ERR_BUFFER_LOCKED;
	}
	mb.lock();
	if (length < _streamBuffer->getDataLength()) {
		mb.unlock();
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	cs_ret_code_t errCode;
	CS_TYPE type = toCsType(_streamBuffer->type());
	uint8_t opCode = _streamBuffer->opCode();
	switch (opCode) {
	case OPCODE_READ_VALUE: {
		LOGd(FMT_SELECT_TYPE, STR_CHAR_CONFIGURATION, type);
		if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelGet(type), accessLevel)) {
			errCode = ERR_NO_ACCESS;
			break;
		}
		// TODO: don't temporarily put data on stack
//		size16_t sbSize = _streamBuffer->getMaxPayloadSize();
//		uint8_t* sbPayload = _streamBuffer->payload();
//		cs_state_data_t stateData(type, sbPayload, sbSize);
		uint8_t stateDataValue[TypeSize(type)];
		cs_state_data_t stateData(type, stateDataValue, sizeof(stateDataValue));
		errCode = State::getInstance().verifySizeForGet(stateData);
		if (FAILURE(errCode)) {
			break;
		}
		errCode = State::getInstance().get(stateData);
		if (SUCCESS(errCode)) {
//			_streamBuffer->setPayloadSize(stateData.size);
			_streamBuffer->setPayload(stateData.value, stateData.size);
			_streamBuffer->setOpCode(OPCODE_READ_VALUE);
			_configurationReadCharacteristic->setValueLength(_streamBuffer->getDataLength());
			_configurationReadCharacteristic->updateValue();
			// Writing the error code would overwrite the value on the read characteristic.
			writeErrCode = false;
		}
		break;
	}
	case OPCODE_WRITE_VALUE: {
		LOGd(FMT_WRITE_TYPE, STR_CHAR_CONFIGURATION, type);
		if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelSet(type), accessLevel)) {
			errCode = ERR_NO_ACCESS;
			break;
		}
		cs_state_data_t stateData(type, _streamBuffer->payload(), _streamBuffer->length());
		errCode = State::getInstance().verifySizeForSet(stateData);
		if (FAILURE(errCode)) {
			break;
		}
		errCode = State::getInstance().set(stateData);
		break;
	}
	default:
		errCode = ERR_UNKNOWN_OP_CODE;
	}
	mb.unlock();
	return errCode;
}

void CrownstoneService::controlWriteErrorCode(uint8_t type, cs_ret_code_t errCode) {
	if (_controlCharacteristic == NULL) {
		return;
	}
	if (_streamBuffer == NULL) {
		LOGe("Streambuffer is null");
		return;
	}
	_streamBuffer->setType(type);
	_streamBuffer->setOpCode(OPCODE_ERR_VALUE);
	_streamBuffer->setPayload((uint8_t*)&errCode, sizeof(errCode));
	_controlCharacteristic->setValueLength(_streamBuffer->getDataLength());
	_controlCharacteristic->updateValue();
}

/*
 * Encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
 * This is only for characteristics that the user writes to. The ones that are read are encrypted using the
 * setMinAccessLevel level.
 * If the user writes to this characteristic with insufficient rights, this method will not be called.
 */
cs_ret_code_t CrownstoneService::stateOnWrite(const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length, bool& writeErrCode) {
	if (!value) {
		LOGw(MSG_CHAR_VALUE_UNDEFINED);
		return ERR_BUFFER_UNASSIGNED;
	}
	LOGi(MSG_CHAR_VALUE_WRITE);
	MasterBuffer& mb = MasterBuffer::getInstance();
	// at this point it is too late to check if mb was locked, because the softdevice doesn't care
	// if the mb was locked, it writes to the buffer in any case
	if (mb.isLocked()) {
		LOGe(MSG_BUFFER_IS_LOCKED);
		return ERR_BUFFER_LOCKED;
	}
	mb.lock();

	if (length < _streamBuffer->getDataLength()) {
		mb.unlock();
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	cs_ret_code_t errCode;
	CS_TYPE type = toCsType(_streamBuffer->type());
	uint8_t opCode = _streamBuffer->opCode();
	switch (opCode) {
	case OPCODE_READ_VALUE: {
		LOGd(FMT_SELECT_TYPE, STR_CHAR_STATE, type);
		if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelGet(type), accessLevel)) {
			errCode = ERR_NO_ACCESS;
			break;
		}
		// TODO: don't temporarily put data on stack
//		size16_t sbSize = _streamBuffer->getMaxPayloadSize();
//		uint8_t* sbPayload = _streamBuffer->payload();
//		cs_state_data_t stateData(type, sbPayload, sbSize);
		uint8_t stateDataValue[TypeSize(type)];
		cs_state_data_t stateData(type, stateDataValue, sizeof(stateDataValue));
		errCode = State::getInstance().verifySizeForGet(stateData);
		if (FAILURE(errCode)) {
			break;
		}
		errCode = State::getInstance().get(stateData);
		if (SUCCESS(errCode)) {
//			_streamBuffer->setPayloadSize(stateData.size);
			_streamBuffer->setPayload(stateData.value, stateData.size);
			_streamBuffer->setOpCode(OPCODE_READ_VALUE);
			_stateReadCharacteristic->setValueLength(_streamBuffer->getDataLength());
			_stateReadCharacteristic->updateValue();
			// Writing the error code would overwrite the value on the read characteristic.
			writeErrCode = false;
		}
		break;
	}
	case OPCODE_WRITE_VALUE: {
		LOGd(FMT_WRITE_TYPE, STR_CHAR_STATE, type);
		if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelSet(type), accessLevel)) {
			errCode = ERR_NO_ACCESS;
			break;
		}
		cs_state_data_t stateData(type, _streamBuffer->payload(), _streamBuffer->length());
		errCode = State::getInstance().verifySizeForSet(stateData);
		if (FAILURE(errCode)) {
			break;
		}
		errCode = State::getInstance().set(stateData);
		break;
	}
	default:
		errCode = ERR_UNKNOWN_OP_CODE;
	}
	mb.unlock();
	return errCode;
}


void CrownstoneService::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::EVT_SESSION_NONCE_SET: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_sessionNonceCharacteristic != NULL) {
			_sessionNonceCharacteristic->setValueLength(event.size);
			_sessionNonceCharacteristic->setValue((uint8_t*)event.data);
			_sessionNonceCharacteristic->updateValue(ECB_GUEST_CAFEBABE);
		}
		break;
	}
	case CS_TYPE::EVT_BLE_CONNECT: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_factoryResetCharacteristic != NULL) {
			_factoryResetCharacteristic->operator=(0);
		}
		break;
	}
	case CS_TYPE::EVT_BLE_DISCONNECT: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_sessionNonceCharacteristic != NULL) {
			_sessionNonceCharacteristic->setValueLength(0);
		}
		break;
	}
	case CS_TYPE::EVT_SETUP_DONE: {
		controlWriteErrorCode(CTRL_CMD_SETUP, ERR_SUCCESS);
		break;
	}
	default: {}
	}
}
