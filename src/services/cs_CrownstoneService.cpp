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

//	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) CrownstoneService::staticTick);
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

#if CHAR_MESHING==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_MESH);
	addMeshCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_MESH);
#endif

#if CHAR_CONFIGURATION==1 || DEVICE_TYPE==DEVICE_FRIDGE
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
	addSessionNonceCharacteristic(buffer, maxLength);

	LOGi(FMT_CHAR_ADD, STR_CHAR_FACTORY_RESET);
	addFactoryResetCharacteristic();

	addCharacteristicsDone();
}

//void CrownstoneService::tick() {
//	scheduleNextTick();
//}

//void CrownstoneService::scheduleNextTick() {
//	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(GENERAL_SERVICE_UPDATE_FREQUENCY), this);
//}

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
	_meshControlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value) -> void {
		// encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
		// this is only for characteristics that the user writes to. The ones that are read are encrypted using the setMinAccessLevel level.
		// If the user writes to this characteristic with insufficient rights, this method is not called

		LOGi(MSG_MESH_MESSAGE_WRITE);

		uint8_t handle = _meshCommand->type();
		uint8_t* p_data = _meshCommand->payload();
		uint16_t length = _meshCommand->length();

		ERR_CODE error_code = MeshControl::getInstance().send(handle, p_data, length);

//		LOGi("err error_code: %d", error_code);
		memcpy(value, &error_code, sizeof(error_code));
		_meshControlCharacteristic->setValueLength(sizeof(error_code));
		_meshControlCharacteristic->updateValue();

	});
#endif
}

void CrownstoneService::addControlCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_controlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_controlCharacteristic);

	_controlCharacteristic->setUUID(UUID(getUUID(), CONTROL_UUID));
	_controlCharacteristic->setName(BLE_CHAR_CONTROL);
	_controlCharacteristic->setWritable(true);
	_controlCharacteristic->setValue(buffer);
	_controlCharacteristic->setMinAccessLevel(GUEST);
	_controlCharacteristic->setMaxGattValueLength(size);
	_controlCharacteristic->setValueLength(0);
	_controlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value) -> void {
		// encryption in the write stage verifies if the key is at least GUEST, command specific permissions are
		// handled in the commandHandler

		ERR_CODE error_code;
		MasterBuffer& mb = MasterBuffer::getInstance();
		// at this point it is too late to check if mb was locked, because the softdevice doesn't care
		// if the mb was locked, it writes to the buffer in any case
		if (!mb.isLocked()) {
			LOGi(MSG_CHAR_VALUE_WRITE);
			CommandHandlerTypes type = (CommandHandlerTypes) _streamBuffer->type();
			uint8_t *payload = _streamBuffer->payload();
			uint8_t length = _streamBuffer->length();

			error_code = CommandHandler::getInstance().handleCommand(type, payload, length, accessLevel);

			mb.unlock();
		} else {
			LOGe(MSG_BUFFER_IS_LOCKED);
			error_code = ERR_BUFFER_LOCKED;
		}

//		LOGi("err error_code: %d", error_code);
		memcpy(value, &error_code, sizeof(error_code));
		_controlCharacteristic->setValueLength(sizeof(error_code));
		_controlCharacteristic->updateValue();
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
	_configurationControlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value) -> void {
		// encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
		// this is only for characteristics that the user writes to. The ones that are read are encrypted using the setMinAccessLevel level.
		// If the user writes to this characteristic with insufficient rights, this method is not called

		ERR_CODE error_code;
		if (!value) {
			LOGw(MSG_CHAR_VALUE_UNDEFINED);
			error_code = ERR_VALUE_UNDEFINED;
		} else {
			LOGi(MSG_CHAR_VALUE_WRITE);
			MasterBuffer& mb = MasterBuffer::getInstance();
			// at this point it is too late to check if mb was locked, because the softdevice doesn't care
			// if the mb was locked, it writes to the buffer in any case
			if (!mb.isLocked()) {
				mb.lock();

				//! TODO: check lenght with actual payload length!
				uint8_t type = _streamBuffer->type();
				uint8_t opCode = _streamBuffer->opCode();

//				LOGi("opCode: %d", opCode);
//				LOGi("type: %d", type);

				if (opCode == READ_VALUE) {
					LOGd(FMT_SELECT_TYPE, STR_CHAR_CONFIGURATION, type);

					error_code = Settings::getInstance().readFromStorage(type, _streamBuffer);
					if (SUCCESS(error_code)) {
						_streamBuffer->setOpCode(READ_VALUE);
						_configurationReadCharacteristic->setValueLength(_streamBuffer->getDataLength());
						_configurationReadCharacteristic->updateValue();
//						LOGi("success");
						mb.unlock();
						return; // need to return here to avoid writing error_code, which would overwrite
								// the read value on the read characteristic !!
					} else {
						LOGe(STR_FAILED);
					}
				} else if (opCode == WRITE_VALUE) {
					LOGd(FMT_WRITE_TYPE, STR_CHAR_CONFIGURATION, type);

					uint8_t *payload = _streamBuffer->payload();
					uint8_t length = _streamBuffer->length();

					error_code = Settings::getInstance().writeToStorage(type, payload, length);
				} else {
					error_code = ERR_UNKNOWN_OP_CODE;
				}

				mb.unlock();
			} else {
				LOGe(MSG_BUFFER_IS_LOCKED);
				error_code = ERR_BUFFER_LOCKED;
			}
		}

//		LOGi("err error_code: %d", error_code);
		memcpy(value, &error_code, sizeof(error_code));
		_configurationControlCharacteristic->setValueLength(sizeof(error_code));
		_configurationControlCharacteristic->updateValue();
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
	_stateControlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value) -> void {
		// encryption level authentication is done in the decrypting step based on the setMinAccessLevel level.
		// this is only for characteristics that the user writes to. The ones that are read are encrypted using the setMinAccessLevel level.
		// If the user writes to this characteristic with insufficient rights, this method is not called

		ERR_CODE error_code;
		if (!value) {
			LOGw(MSG_CHAR_VALUE_UNDEFINED);
			error_code = ERR_VALUE_UNDEFINED;
		} else {
			LOGi(MSG_CHAR_VALUE_WRITE);
			MasterBuffer& mb = MasterBuffer::getInstance();
			// at this point it is too late to check if mb was locked, because the softdevice doesn't care
			// if the mb was locked, it writes to the buffer in any case
			if (!mb.isLocked()) {
				mb.lock();

				uint8_t type = _streamBuffer->type();
				uint8_t opCode = _streamBuffer->opCode();

				if (opCode == READ_VALUE || opCode == NOTIFY_VALUE) {
					if (opCode == NOTIFY_VALUE) {
//						LOGi("State notification, len: %d", _streamBuffer->length());
						if (_streamBuffer->length() == 1) {
							bool enable = *((bool*) _streamBuffer->payload());
							State::getInstance().setNotify(type, enable);
							error_code = ERR_SUCCESS;
						} else {
							LOGe(FMT_WRONG_PAYLOAD_LENGTH, _streamBuffer->length());
							error_code = ERR_WRONG_PAYLOAD_LENGTH;
						}
					}

					error_code = State::getInstance().readFromStorage(type, _streamBuffer);
					LOGd(FMT_SELECT_TYPE, STR_CHAR_STATE, type);
					if (SUCCESS(error_code)) {
						_streamBuffer->setOpCode(READ_VALUE);
						_stateReadCharacteristic->setValueLength(_streamBuffer->getDataLength());
						_stateReadCharacteristic->updateValue();
//						LOGi("success");
						mb.unlock();
						return; // need to return here to avoid writing error_code, which would overwrite
								// the read value on the read characteristic !!
					}
				} else if (opCode == WRITE_VALUE) {
					LOGd(FMT_WRITE_TYPE, STR_CHAR_STATE, type);
					error_code = State::getInstance().writeToStorage(type, _streamBuffer->payload(), _streamBuffer->length());
				} else {
					error_code = ERR_UNKNOWN_OP_CODE;
				}

				mb.unlock();
			} else {
				LOGe(MSG_BUFFER_IS_LOCKED);
				error_code = ERR_BUFFER_LOCKED;
			}
		}

//		LOGi("err error_code: %d", error_code);
		memcpy(value, &error_code, sizeof(error_code));
		_stateControlCharacteristic->setValueLength(sizeof(error_code));
		_stateControlCharacteristic->updateValue();
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


void CrownstoneService::addSessionNonceCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_sessionNonceCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_sessionNonceCharacteristic);

	_sessionNonceCharacteristic->setUUID(UUID(getUUID(), SESSION_NONCE_UUID));
	_sessionNonceCharacteristic->setName(BLE_CHAR_SESSION_NONCE);
	_sessionNonceCharacteristic->setWritable(false);
	_sessionNonceCharacteristic->setNotifies(false);
	_sessionNonceCharacteristic->setValue(buffer);
	_sessionNonceCharacteristic->setMinAccessLevel(GUEST);
	_sessionNonceCharacteristic->setMaxGattValueLength(size);
	_sessionNonceCharacteristic->setValueLength(0);
}

void CrownstoneService::addFactoryResetCharacteristic() {
	_factoryResetCharacteristic = new Characteristic<uint32_t>();
	addCharacteristic(_factoryResetCharacteristic);

	_factoryResetCharacteristic->setUUID(UUID(getUUID(), FACTORY_RESET_UUID));
	_factoryResetCharacteristic->setName(BLE_CHAR_FACTORY_RESET);
	_factoryResetCharacteristic->setWritable(true);
	_factoryResetCharacteristic->setNotifies(false);
	_factoryResetCharacteristic->setDefaultValue(0);
	_factoryResetCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_factoryResetCharacteristic->onWrite([&](const uint8_t accessLevel, const uint32_t& value) -> void {
		FactoryReset::getInstance().recover(value);
	});
}


void CrownstoneService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_SESSION_NONCE_SET: {
		_sessionNonceCharacteristic->setValueLength(SESSION_NONCE_LENGTH);
		_sessionNonceCharacteristic->setValue((uint8_t*)p_data);
		_sessionNonceCharacteristic->updateValue(false);
		break;
	}
	case EVT_BLE_DISCONNECT: {
		_sessionNonceCharacteristic->setValueLength(0);
		break;
	}
	case EVT_STATE_NOTIFICATION: {
		if (_stateReadCharacteristic) {
			state_vars_notifaction notification = *(state_vars_notifaction*)p_data;
//			log(SERIAL_DEBUG, "send notification for %d, value:", notification.type);
//			BLEutil::printArray(notification.data, notification.dataLength);

			_streamBuffer->setPayload(notification.data, notification.dataLength);
			_streamBuffer->setType(notification.type);
			_streamBuffer->setOpCode(NOTIFY_VALUE);

			_stateReadCharacteristic->setValueLength(_streamBuffer->getDataLength());
			_stateReadCharacteristic->updateValue();
		}
	}
	}
}

