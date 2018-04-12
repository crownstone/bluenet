/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/cs_CrownstoneService.h>

#include <storage/cs_Settings.h>
#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
#endif
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include <protocol/cs_ErrorCodes.h>

CrownstoneService::CrownstoneService() : EventListener(),
		_controlCharacteristic(NULL),
		_configurationControlCharacteristic(NULL), _configurationReadCharacteristic(NULL),
		_streamBuffer(NULL),
#if BUILD_MESHING == 1
		_meshControlCharacteristic(NULL), _meshCommand(NULL),
#endif
		_stateControlCharacteristic(NULL), _stateReadCharacteristic(NULL),
		_sessionNonceCharacteristic(NULL), _factoryResetCharacteristic(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(CROWNSTONE_UUID));
	setName(BLE_SERVICE_CROWNSTONE);

}


StreamBuffer<uint8_t>* CrownstoneService::getStreamBuffer(buffer_ptr_t& buffer, uint16_t& maxLength) {
	//! if it is not created yet, create a new stream buffer and assign the master buffer to it
	if (_streamBuffer == NULL) {
		_streamBuffer = new StreamBuffer<uint8_t>();

		MasterBuffer& mb = MasterBuffer::getInstance();
		uint16_t size = 0;
		mb.getBuffer(buffer, size);

//		LOGd("Assign buffer of size %i to stream buffer", size);
		_streamBuffer->assign(buffer, size);
		maxLength = _streamBuffer->getMaxLength();
	} else {
		//! otherwise use the same buffer
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
	addControlCharacteristic(buffer, maxLength);
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_CONTROL);
#endif

#if (BUILD_MESHING == 1) && (CHAR_MESHING == 1)
	LOGi(FMT_CHAR_ADD, STR_CHAR_MESH);
	addMeshCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_MESH);
#endif

#if CHAR_CONFIGURATION==1
	{
		LOGi(FMT_CHAR_ADD, STR_CHAR_CONFIGURATION);
		_streamBuffer = getStreamBuffer(buffer, maxLength);
		addConfigurationControlCharacteristic(buffer, maxLength, ADMIN);
		addConfigurationReadCharacteristic(buffer, maxLength, ADMIN);

//		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_CONFIGURATION);
#endif

#if CHAR_STATE==1
	{
		LOGi(FMT_CHAR_ADD, STR_CHAR_STATE);
		_streamBuffer = getStreamBuffer(buffer, maxLength);
		addStateControlCharacteristic(buffer, maxLength);
		addStateReadCharacteristic(buffer, maxLength);

//		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_STATE);
#endif

	LOGi(FMT_CHAR_ADD, STR_CHAR_SESSION_NONCE);
	addSessionNonceCharacteristic(_nonceBuffer, maxLength);

	LOGi(FMT_CHAR_ADD, STR_CHAR_FACTORY_RESET);
	addFactoryResetCharacteristic();

	addCharacteristicsDone();
}


void CrownstoneService::addMeshCharacteristic() {
#if BUILD_MESHING == 1
	_meshCommand = new MeshCommand();
//	_meshCommand = new StreamBuffer<uint8_t, MAX_MESH_MESSAGE_PAYLOAD_LENGTH>();

	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t size = 0;
	mb.getBuffer(buffer, size);

	_meshCommand->assign(buffer, size);

	_meshControlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_meshControlCharacteristic);

	_meshControlCharacteristic->setUUID(UUID(getUUID(), MESH_CONTROL_UUID));
	_meshControlCharacteristic->setName(BLE_CHAR_MESH_CONTROL);
	_meshControlCharacteristic->setWritable(true);
	_meshControlCharacteristic->setValue(buffer);
	_meshControlCharacteristic->setMinAccessLevel(MEMBER);
	_meshControlCharacteristic->setMaxGattValueLength(size);
	_meshControlCharacteristic->setValueLength(0);
	_meshControlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length) -> void {
		// encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
		// this is only for characteristics that the user writes to. The ones that are read are encrypted using the setMinAccessLevel level.
		// If the user writes to this characteristic with insufficient rights, this method is not called

		LOGi(MSG_MESH_MESSAGE_WRITE);

		uint8_t handle = _meshCommand->type();
		uint8_t* p_data = _meshCommand->payload();
		ERR_CODE error_code;
		if (length < _meshCommand->getDataLength()) {
			error_code = ERR_WRONG_PAYLOAD_LENGTH;
		}
		else {
			error_code = MeshControl::getInstance().send(handle, p_data, length);
		}

//		LOGi("err error_code: %d", error_code);
		memcpy(value, &error_code, sizeof(error_code));
		_meshControlCharacteristic->setValueLength(sizeof(error_code));
		_meshControlCharacteristic->updateValue();

	});
#endif
}


void CrownstoneService::addControlCharacteristic(buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel minimumAccessLevel) {
	_controlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_controlCharacteristic);

	_controlCharacteristic->setUUID(UUID(getUUID(), CONTROL_UUID));
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

		ERR_CODE errCode;
		CommandHandlerTypes type = CMD_UNKNOWN;
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
				errCode = CommandHandler::getInstance().handleCommand(type, payload, payloadLength, accessLevel);
			}

			mb.unlock();
		}
		else {
			LOGe(MSG_BUFFER_IS_LOCKED);
			errCode = ERR_BUFFER_LOCKED;
		}
		controlWriteErrorCode(type, errCode);
	});

}

void CrownstoneService::addConfigurationControlCharacteristic(buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel minimumAccessLevel) {
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
		ERR_CODE errCode = configOnWrite(accessLevel, value, length, writeErrCode);
		if (writeErrCode) {
			// Leave type as is.
//			_streamBuffer->setType()
			_streamBuffer->setOpCode(OPCODE_ERR_VALUE);
			_streamBuffer->setPayload((uint8_t*)&errCode, sizeof(errCode));
			_configurationControlCharacteristic->setValueLength(_streamBuffer->getDataLength());
//			memcpy(value, &errCode, sizeof(errCode));
//			_configurationControlCharacteristic->setValueLength(sizeof(errCode));
			_configurationControlCharacteristic->updateValue();
		}
	});
}


void CrownstoneService::addConfigurationReadCharacteristic(buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel minimumAccessLevel) {
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
		ERR_CODE errCode = stateOnWrite(accessLevel, value, length, writeErrCode);
		if (writeErrCode) {
			// Leave type as is.
//			_streamBuffer->setType()
			_streamBuffer->setOpCode(OPCODE_ERR_VALUE);
			_streamBuffer->setPayload((uint8_t*)&errCode, sizeof(errCode));
			_stateControlCharacteristic->setValueLength(_streamBuffer->getDataLength());
//			memcpy(value, &errCode, sizeof(errCode));
//			_stateControlCharacteristic->setValueLength(sizeof(errCode));
			_stateControlCharacteristic->updateValue();
		}
	});
}

void CrownstoneService::addStateReadCharacteristic(buffer_ptr_t buffer, uint16_t size) {
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

ERR_CODE CrownstoneService::configOnWrite(const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length, bool& writeErrCode) {
	// encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
	// this is only for characteristics that the user writes to. The ones that are read are encrypted using the setMinAccessLevel level.
	// If the user writes to this characteristic with insufficient rights, this method is not called

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

	ERR_CODE errCode;
	uint8_t type = _streamBuffer->type();
	uint8_t opCode = _streamBuffer->opCode();
	switch (opCode) {
	case OPCODE_READ_VALUE: {
		LOGd(FMT_SELECT_TYPE, STR_CHAR_CONFIGURATION, type);

		errCode = Settings::getInstance().readFromStorage(type, _streamBuffer);
		if (SUCCESS(errCode)) {
			_streamBuffer->setOpCode(OPCODE_READ_VALUE);
			_configurationReadCharacteristic->setValueLength(_streamBuffer->getDataLength());
			_configurationReadCharacteristic->updateValue();

			// Writing the error code would overwrite the value on the read characteristic.
			writeErrCode = false;
		}
		else {
			LOGe(STR_FAILED);
		}
		break;
	}
	case OPCODE_WRITE_VALUE: {
		LOGd(FMT_WRITE_TYPE, STR_CHAR_CONFIGURATION, type);

		uint8_t *payload = _streamBuffer->payload();
		uint16_t payloadLength = _streamBuffer->length();
		errCode = Settings::getInstance().writeToStorage(type, payload, payloadLength);
		break;
	}
	default:
		errCode = ERR_UNKNOWN_OP_CODE;
	}
	mb.unlock();
	return errCode;
}

void CrownstoneService::controlWriteErrorCode(uint8_t type, ERR_CODE errCode) {
	if (_controlCharacteristic != NULL) {
		_streamBuffer->setType(type);
		_streamBuffer->setOpCode(OPCODE_ERR_VALUE);
		_streamBuffer->setPayload((uint8_t*)&errCode, sizeof(errCode));

		_controlCharacteristic->setValueLength(_streamBuffer->getDataLength());
		_controlCharacteristic->updateValue();
	}
}

ERR_CODE CrownstoneService::stateOnWrite(const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length, bool& writeErrCode) {
	// encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
	// this is only for characteristics that the user writes to. The ones that are read are encrypted using the setMinAccessLevel level.
	// If the user writes to this characteristic with insufficient rights, this method is not called

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

	ERR_CODE errCode;
	uint8_t type = _streamBuffer->type();
	uint8_t opCode = _streamBuffer->opCode();

	switch (opCode) {
	case OPCODE_NOTIFY_VALUE: {
		if (_streamBuffer->length() == 1) {
			bool enable = *((bool*) _streamBuffer->payload());
			State::getInstance().setNotify(type, enable);
			errCode = ERR_SUCCESS;
		}
		else {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, _streamBuffer->length());
			errCode = ERR_WRONG_PAYLOAD_LENGTH;
		}
		// No break, fall through to read value.
	}
	case OPCODE_READ_VALUE: {
		errCode = State::getInstance().readFromStorage(type, _streamBuffer);
		LOGd(FMT_SELECT_TYPE, STR_CHAR_STATE, type);
		if (SUCCESS(errCode)) {
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
		errCode = State::getInstance().writeToStorage(type, _streamBuffer->payload(), _streamBuffer->length());
		break;
	}
	default:
		errCode = ERR_UNKNOWN_OP_CODE;
	}
	mb.unlock();
	return errCode;
}


void CrownstoneService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_SESSION_NONCE_SET: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_sessionNonceCharacteristic != NULL) {
			_sessionNonceCharacteristic->setValueLength(length);
			_sessionNonceCharacteristic->setValue((uint8_t *) p_data);
			_sessionNonceCharacteristic->updateValue(ECB_GUEST_CAFEBABE);
		}
		break;
	}
	case EVT_BLE_CONNECT: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_factoryResetCharacteristic != NULL) {
			_factoryResetCharacteristic->operator=(0);
		}
		break;
	}
	case EVT_BLE_DISCONNECT: {
		//! Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_sessionNonceCharacteristic != NULL) {
			_sessionNonceCharacteristic->setValueLength(0);
		}
		break;
	}
	case EVT_STATE_NOTIFICATION: {
		if (_stateReadCharacteristic != NULL) {
			state_vars_notifaction notification = *(state_vars_notifaction*)p_data;
//			log(SERIAL_DEBUG, "send notification for %d, value:", notification.type);
//			BLEutil::printArray(notification.data, notification.dataLength);

			_streamBuffer->setPayload(notification.data, notification.dataLength);
			_streamBuffer->setType(notification.type);
			_streamBuffer->setOpCode(OPCODE_NOTIFY_VALUE);

			_stateReadCharacteristic->setValueLength(_streamBuffer->getDataLength());
			_stateReadCharacteristic->updateValue();
		}
		break;
	}
	case EVT_SETUP_DONE: {
		controlWriteErrorCode(CMD_SETUP, ERR_SUCCESS);
		break;
	}
	}
}

