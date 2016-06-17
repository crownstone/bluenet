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
#include <mesh/cs_MeshControl.h>
#include <processing/cs_CommandHandler.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_MasterBuffer.h>

using namespace BLEpp;

CrownstoneService::CrownstoneService() : EventListener(),
		_controlCharacteristic(NULL),
		_meshControlCharacteristic(NULL), _configurationControlCharacteristic(NULL), _configurationReadCharacteristic(NULL),
		_stateControlCharacteristic(NULL), _stateReadCharacteristic(NULL),
		_streamBuffer(NULL), _meshCommand(NULL)
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

		LOGd("Assign buffer of size %i to stream buffer", size);
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
	LOGi("Service Crownstone init");

	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;

#if CHAR_CONTROL==1
	LOGi(MSG_CHAR_CONTROL_ADD);
	_streamBuffer = getStreamBuffer(buffer, maxLength);
	addControlCharacteristic(buffer, maxLength);
#else
	LOGi(MSG_CHAR_CONTROL_SKIP);
#endif

#if CHAR_MESHING==1
//	if (Settings::getInstance().isEnabled(CONFIG_MESH_ENABLED)) {
		LOGi(MSG_CHAR_MESH_ADD);
		addMeshCharacteristic();
//	} else {
//		LOGi(MSG_CHAR_MESH_SKIP);
//	}
#else
	LOGi(MSG_CHAR_MESH_SKIP);
#endif

#if CHAR_CONFIGURATION==1 || DEVICE_TYPE==DEVICE_FRIDGE
	{
		LOGi(MSG_CHAR_CONFIGURATION_ADD);
		_streamBuffer = getStreamBuffer(buffer, maxLength);
		addConfigurationControlCharacteristic(buffer, maxLength);
		addConfigurationReadCharacteristic(buffer, maxLength);

		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(MSG_CHAR_CONFIGURATION_SKIP);
#endif

#if CHAR_STATE==1
	{
		LOGi(MSG_CHAR_STATE_ADD);
		_streamBuffer = getStreamBuffer(buffer, maxLength);
		addStateControlCharacteristic(buffer, maxLength);
		addStateReadCharacteristic(buffer, maxLength);

		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(MSG_CHAR_STATEVARIABLES_SKIP);
#endif

	addCharacteristicsDone();
}

//void CrownstoneService::tick() {
//	scheduleNextTick();
//}

//void CrownstoneService::scheduleNextTick() {
//	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(GENERAL_SERVICE_UPDATE_FREQUENCY), this);
//}

void CrownstoneService::addMeshCharacteristic() {

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
	_meshControlCharacteristic->setMaxLength(size);
	_meshControlCharacteristic->setDataLength(0);
	_meshControlCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {
		LOGi(MSG_MESH_MESSAGE_WRITE);

		uint8_t handle = _meshCommand->type();
		uint8_t* p_data = _meshCommand->payload();
		uint16_t length = _meshCommand->length();

		MeshControl::getInstance().send(handle, p_data, length);
	});
}

void CrownstoneService::addControlCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_controlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_controlCharacteristic);

	_controlCharacteristic->setUUID(UUID(getUUID(), CONTROL_UUID));
	_controlCharacteristic->setName(BLE_CHAR_CONTROL);
	_controlCharacteristic->setWritable(true);
	_controlCharacteristic->setValue(buffer);
	_controlCharacteristic->setMaxLength(size);
	_controlCharacteristic->setDataLength(0);
	_controlCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {

		MasterBuffer& mb = MasterBuffer::getInstance();
		// at this point it is too late to check if mb was locked, because the softdevice doesn't care
		// if the mb was locked, it writes to the buffer in any case
		if (!mb.isLocked()) {
			LOGi(MSG_CHAR_VALUE_WRITE);
			CommandHandlerTypes type = (CommandHandlerTypes) _streamBuffer->type();
			uint8_t *payload = _streamBuffer->payload();
			uint8_t length = _streamBuffer->length();

			ERR_CODE code = CommandHandler::getInstance().handleCommand(type, payload, length);

			LOGi("err code: %d", code);
			memcpy(value, &code, sizeof(code));
			_controlCharacteristic->setDataLength(sizeof(code));
			_controlCharacteristic->notify();

			mb.unlock();
		} else {
			LOGe(MSG_BUFFER_IS_LOCKED);
		}
	});

}

void CrownstoneService::addConfigurationControlCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_configurationControlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_configurationControlCharacteristic);

	_configurationControlCharacteristic->setUUID(UUID(getUUID(), CONFIG_CONTROL_UUID));
	_configurationControlCharacteristic->setName(BLE_CHAR_CONFIG_CONTROL);
	_configurationControlCharacteristic->setWritable(true);
	_configurationControlCharacteristic->setValue(buffer);
	_configurationControlCharacteristic->setMaxLength(size);
	_configurationControlCharacteristic->setDataLength(0);
	_configurationControlCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {

		if (!value) {
			LOGw(MSG_CHAR_VALUE_UNDEFINED);
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
					LOGd("Select configuration type: %d", type);

					bool success = Settings::getInstance().readFromStorage(type, _streamBuffer);
					if (success) {
						_streamBuffer->setOpCode(READ_VALUE);
						_configurationReadCharacteristic->setDataLength(_streamBuffer->getDataLength());
						_configurationReadCharacteristic->notify();
					} else {
						LOGe("Failed to read from storage");
					}
				} else if (opCode == WRITE_VALUE) {
					LOGi("Write configuration type: %d", (int)type);

					uint8_t *payload = _streamBuffer->payload();
					uint8_t length = _streamBuffer->length();

					Settings::getInstance().writeToStorage(type, payload, length);
				}
				mb.unlock();
			} else {
				LOGe(MSG_BUFFER_IS_LOCKED);
			}
		}
	});
}

void CrownstoneService::addConfigurationReadCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_configurationReadCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_configurationReadCharacteristic);

	_configurationReadCharacteristic->setUUID(UUID(getUUID(), CONFIG_READ_UUID));
	_configurationReadCharacteristic->setName(BLE_CHAR_CONFIG_READ);
	_configurationReadCharacteristic->setWritable(false);
	_configurationReadCharacteristic->setNotifies(true);
	_configurationReadCharacteristic->setValue(buffer);
	_configurationReadCharacteristic->setMaxLength(size);
	_configurationReadCharacteristic->setDataLength(0);
}

void CrownstoneService::addStateControlCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_stateControlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_stateControlCharacteristic);

	_stateControlCharacteristic->setUUID(UUID(getUUID(), STATE_CONTROL_UUID));
	_stateControlCharacteristic->setName(BLE_CHAR_STATE_CONTROL);
	_stateControlCharacteristic->setWritable(true);
	_stateControlCharacteristic->setValue(buffer);
	_stateControlCharacteristic->setMaxLength(size);
	_stateControlCharacteristic->setDataLength(0);
	_stateControlCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {
		if (!value) {
			LOGw(MSG_CHAR_VALUE_UNDEFINED);
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
					LOGi("Read state");
					bool success = State::getInstance().readFromStorage(type, _streamBuffer);
					if (success) {
						_streamBuffer->setOpCode(READ_VALUE);
						_stateReadCharacteristic->setDataLength(_streamBuffer->getDataLength());
						_stateReadCharacteristic->notify();
					}

					if (opCode == NOTIFY_VALUE) {
						LOGi("State notification");
						if (_streamBuffer->length() == 1) {
							bool enable = *((bool*) _streamBuffer->payload());
							State::getInstance().setNotify(type, enable);
						} else {
							LOGe("wrong length received!");
						}
					}
				} else if (opCode == WRITE_VALUE) {
					LOGi("Write state");
					State::getInstance().writeToStorage(type, _streamBuffer->payload(), _streamBuffer->length());
				}

				mb.unlock();
			} else {
				LOGe(MSG_BUFFER_IS_LOCKED);
			}
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
	_stateReadCharacteristic->setMaxLength(size);
	_stateReadCharacteristic->setDataLength(0);
}

void CrownstoneService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_STATE_NOTIFICATION: {
		if (_stateReadCharacteristic) {
			state_vars_notifaction notification = *(state_vars_notifaction*)p_data;
			log(DEBUG, "send notification for %d, value:", notification.type);
			BLEutil::printArray(notification.data, notification.dataLength);

			_streamBuffer->setPayload(notification.data, notification.dataLength);
			_streamBuffer->setType(notification.type);
			_streamBuffer->setOpCode(NOTIFY_VALUE);

			_stateReadCharacteristic->setDataLength(_streamBuffer->getDataLength());
			_stateReadCharacteristic->notify();
		}
	}
	}
}

