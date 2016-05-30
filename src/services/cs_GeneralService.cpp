/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/cs_GeneralService.h>

#include <cfg/cs_Settings.h>
#include <cfg/cs_StateVars.h>
#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <protocol/cs_MeshControl.h>
#include <processing/cs_CommandHandler.h>
#include <structs/buffer/cs_MasterBuffer.h>

using namespace BLEpp;

GeneralService::GeneralService() : EventListener(),
		_commandCharacteristic(NULL), _temperatureCharacteristic(NULL), _resetCharacteristic(NULL),
		_meshCharacteristic(NULL), _setConfigurationCharacteristic(NULL), _getConfigurationCharacteristic(NULL),
		_selectStateVarCharacteristic(NULL), _readStateVarCharacteristic(NULL),
		_streamBuffer(NULL), _meshMessage(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(GENERAL_UUID));
	setName(BLE_SERVICE_GENERAL);

	init();

//	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) GeneralService::staticTick);
}

CharacBuffer<uint8_t>* GeneralService::getCharacBuffer(buffer_ptr_t& buffer, uint16_t& maxLength) {
	//! if it is not created yet, create a new stream buffer and assign the master buffer to it
	if (_streamBuffer == NULL) {
		_streamBuffer = new CharacBuffer<uint8_t>();

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

void GeneralService::init() {
	LOGi(MSG_SERVICE_GENERAL_INIT);

	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;

#if CHAR_COMMAND==1
	LOGi(MSG_CHAR_COMMAND_ADD);
	_streamBuffer = getCharacBuffer(buffer, maxLength);
	addCommandCharacteristic(buffer, maxLength);
#else
	LOGi(MSG_CHAR_COMMAND_SKIP);
#endif

#if CHAR_MESHING==1
	if (Settings::getInstance().isEnabled(CONFIG_MESH_ENABLED)) {
		LOGi(MSG_CHAR_MESH_ADD);
		addMeshCharacteristic();
	} else {
		LOGi(MSG_CHAR_MESH_SKIP);
	}
#else
	LOGi(MSG_CHAR_MESH_SKIP);
#endif

#if CHAR_CONFIGURATION==1 || DEVICE_TYPE==DEVICE_FRIDGE
	{
		LOGi(MSG_CHAR_CONFIGURATION_ADD);
		_streamBuffer = getCharacBuffer(buffer, maxLength);
		addSetConfigurationCharacteristic(buffer, maxLength);
		addGetConfigurationCharacteristic(buffer, maxLength);

		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(MSG_CHAR_CONFIGURATION_SKIP);
#endif

#if CHAR_STATE_VARIABLES==1
	{
		LOGi(MSG_CHAR_STATEVARIABLES_ADD);
		_streamBuffer = getCharacBuffer(buffer, maxLength);
		addSelectStateVarCharacteristic(buffer, maxLength);
		addReadStateVarCharacteristic(buffer, maxLength);

		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(MSG_CHAR_STATEVARIABLES_SKIP);
#endif

#if CHAR_TEMPERATURE==1 || DEVICE_TYPE==DEVICE_FRIDGE
	LOGi(MSG_CHAR_TEMPERATURE_ADD);
	addTemperatureCharacteristic();
#else
	LOGi(MSG_CHAR_TEMPERATURE_SKIP);
#endif

#if CHAR_RESET==1
	LOGi(MSG_CHAR_RESET_ADD);
	addResetCharacteristic();
#else
	LOGi(MSG_CHAR_RESET_SKIP);
#endif

	addCharacteristicsDone();
}

//void GeneralService::tick() {
//	scheduleNextTick();
//}

//void GeneralService::scheduleNextTick() {
//	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(GENERAL_SERVICE_UPDATE_FREQUENCY), this);
//}

void GeneralService::addTemperatureCharacteristic() {
	_temperatureCharacteristic = new Characteristic<int32_t>();
	addCharacteristic(_temperatureCharacteristic);

	_temperatureCharacteristic->setUUID(UUID(getUUID(), TEMPERATURE_UUID));
	_temperatureCharacteristic->setName(BLE_CHAR_TEMPERATURE);
	_temperatureCharacteristic->setDefaultValue(0);
	_temperatureCharacteristic->setNotifies(true);
}

void GeneralService::addResetCharacteristic() {
	_resetCharacteristic = new Characteristic<int32_t>();
	addCharacteristic(_resetCharacteristic);

	_resetCharacteristic->setUUID(UUID(getUUID(), RESET_UUID));
	_resetCharacteristic->setName(BLE_CHAR_RESET);
	_resetCharacteristic->setDefaultValue(0);
	_resetCharacteristic->setWritable(true);
	_resetCharacteristic->onWrite([&](const int32_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_RESET, (buffer_ptr_t)&value, 4);
	});
}

void GeneralService::addMeshCharacteristic() {

	if (_meshCharacteristic) {
		LOGe("mesh characteristic already added");
		return;
	}

	_meshMessage = new MeshCharacteristicMessage();

	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t size = 0;
	mb.getBuffer(buffer, size);

	_meshMessage->assign(buffer, size);

	_meshCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_meshCharacteristic);

	_meshCharacteristic->setUUID(UUID(getUUID(), MESH_CONTROL_UUID));
	_meshCharacteristic->setName(BLE_CHAR_MESH);
	_meshCharacteristic->setWritable(true);
	_meshCharacteristic->setValue(buffer);
	_meshCharacteristic->setMaxLength(size);
	_meshCharacteristic->setDataLength(0);
	_meshCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {
		LOGi(MSG_MESH_MESSAGE_WRITE);

		uint8_t handle = _meshMessage->channel();
		uint8_t* p_data;
		uint16_t length;
		_meshMessage->data(p_data, length);

		MeshControl::getInstance().send(handle, p_data, length);
	});
}

void GeneralService::removeMeshCharacteristic() {
	if (_meshMessage) free(_meshMessage);
	if (_meshCharacteristic) {
		removeCharacteristic(_meshCharacteristic);
		free(_meshCharacteristic);
	}
}

void GeneralService::addCommandCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_commandCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_commandCharacteristic);

	_commandCharacteristic->setUUID(UUID(getUUID(), COMMAND_UUID));
	_commandCharacteristic->setName("Command");
	_commandCharacteristic->setWritable(true);
	_commandCharacteristic->setValue(buffer);
	_commandCharacteristic->setMaxLength(size);
	_commandCharacteristic->setDataLength(size);
	_commandCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {

		MasterBuffer& mb = MasterBuffer::getInstance();
		// at this point it is too late to check if mb was locked, because the softdevice doesn't care
		// if the mb was locked, it writes to the buffer in any case
		if (!mb.isLocked()) {
			LOGi(MSG_CHAR_VALUE_WRITE);
			CommandHandlerTypes type = (CommandHandlerTypes) _streamBuffer->type();
			uint8_t *payload = _streamBuffer->payload();
			uint8_t length = _streamBuffer->length();

			CommandHandler::getInstance().handleCommand(type, payload, length);

			mb.unlock();
		} else {
			LOGe(MSG_BUFFER_IS_LOCKED);
		}
	});

}

void GeneralService::addSetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_setConfigurationCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_setConfigurationCharacteristic);

	_setConfigurationCharacteristic->setUUID(UUID(getUUID(), WRITE_CONFIGURATION_UUID));
	_setConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_WRITE);
	_setConfigurationCharacteristic->setWritable(true);
	_setConfigurationCharacteristic->setValue(buffer);
	_setConfigurationCharacteristic->setMaxLength(size);
	_setConfigurationCharacteristic->setDataLength(size);
	_setConfigurationCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {

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

				LOGi("opCode: %d", opCode);
				LOGi("type: %d", type);

				if (opCode == READ_VALUE) {
					LOGd("Select configuration type: %d", type);

					bool success = Settings::getInstance().readFromStorage(type, _streamBuffer);
					if (success) {
						_streamBuffer->setOpCode(READ_VALUE);
						_getConfigurationCharacteristic->setDataLength(_streamBuffer->getDataLength());
						_getConfigurationCharacteristic->notify();
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

void GeneralService::addGetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_getConfigurationCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_getConfigurationCharacteristic);

	_getConfigurationCharacteristic->setUUID(UUID(getUUID(), READ_CONFIGURATION_UUID));
	_getConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_READ);
	_getConfigurationCharacteristic->setWritable(false);
	_getConfigurationCharacteristic->setNotifies(true);
	_getConfigurationCharacteristic->setValue(buffer);
	_getConfigurationCharacteristic->setMaxLength(size);
	_getConfigurationCharacteristic->setDataLength(size);
}

void GeneralService::addSelectStateVarCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_selectStateVarCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_selectStateVarCharacteristic);

	_selectStateVarCharacteristic->setUUID(UUID(getUUID(), WRITE_STATEVAR_UUID));
	_selectStateVarCharacteristic->setName(BLE_CHAR_STATEVAR_WRITE);
	_selectStateVarCharacteristic->setWritable(true);
	_selectStateVarCharacteristic->setValue(buffer);
	_selectStateVarCharacteristic->setMaxLength(size);
	_selectStateVarCharacteristic->setDataLength(size);
	_selectStateVarCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {
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

				if (opCode == READ_VALUE) {
					LOGi("Read state var");
					bool success = StateVars::getInstance().readFromStorage(type, _streamBuffer);
					if (success) {
						_streamBuffer->setOpCode(READ_VALUE);
						_readStateVarCharacteristic->setDataLength(_streamBuffer->getDataLength());
						_readStateVarCharacteristic->notify();
					}
				} else if (opCode == NOTIFY_VALUE) {
					LOGi("State var notification");
					if (_streamBuffer->length() == 1) {
						bool enable = *((bool*) _streamBuffer->payload());
						StateVars::getInstance().setNotify(type, enable);
					} else {
						LOGe("wrong length received!");
					}
				} else if (opCode == WRITE_VALUE) {
					LOGi("Write state var");
					StateVars::getInstance().writeToStorage(type, _streamBuffer->payload(), _streamBuffer->length());
				}

				mb.unlock();
			} else {
				LOGe(MSG_BUFFER_IS_LOCKED);
			}
		}
	});
}

void GeneralService::addReadStateVarCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_readStateVarCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_readStateVarCharacteristic);

	_readStateVarCharacteristic->setUUID(UUID(getUUID(), READ_STATEVAR_UUID));
	_readStateVarCharacteristic->setName(BLE_CHAR_STATEVAR_READ);
	_readStateVarCharacteristic->setWritable(false);
	_readStateVarCharacteristic->setNotifies(true);
	_readStateVarCharacteristic->setValue(buffer);
	_readStateVarCharacteristic->setMaxLength(size);
	_readStateVarCharacteristic->setDataLength(size);
}

void GeneralService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_ENABLED_MESH : {
//		bool enable = *(bool*)p_data;
//		if (enable) {
//			addMeshCharacteristic();
//		} else {
//			removeMeshCharacteristic();
//		}
//		EventDispatcher::getInstance().dispatch(EVT_CHARACTERISTICS_UPDATED);
		break;
	}
	case SV_TEMPERATURE: {
		if (_temperatureCharacteristic) {
			int32_t temperature = *(int32_t*)p_data;
//			LOGd("udpate temperature characteristic: %d", temperature);
			*_temperatureCharacteristic = temperature;
		}
#ifdef MICRO_VIEW
		//! Update temperature at the display
		write("1 %i\r\n", temp);
#endif
		break;
	}
	case EVT_STATE_VARS_NOTIFICATION: {
		if (_readStateVarCharacteristic) {
			state_vars_notifaction notification = *(state_vars_notifaction*)p_data;
//			log(DEBUG, "send notification for %d, value:", notification.type);
			BLEutil::printArray(notification.data, notification.dataLength);

			_streamBuffer->setPayload(notification.data, notification.dataLength);
			_streamBuffer->setType(notification.type);
			_streamBuffer->setOpCode(NOTIFY_VALUE);

			_readStateVarCharacteristic->setDataLength(_streamBuffer->getDataLength());
			_readStateVarCharacteristic->notify();
		}
	}
	}
}

