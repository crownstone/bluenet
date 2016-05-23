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

GeneralService::GeneralService() :
		EventListener(), _temperatureCharacteristic(NULL), _resetCharacteristic(NULL), _streamBuffer(NULL)
{

	setUUID(UUID(GENERAL_UUID));
	setName(BLE_SERVICE_GENERAL);

	init();

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) GeneralService::staticTick);
}

void GeneralService::init() {
	LOGi(MSG_SERVICE_GENERAL_INIT);

	// set up the stream buffer which will be used by the command and configuration characteristics
	_streamBuffer = new StreamBuffer<uint8_t>();
	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t size = 0;
	mb.getBuffer(buffer, size);

	LOGd("Assign buffer of size %i to stream buffer", size);
	_streamBuffer->assign(buffer, size);

	addCommandCharacteristic();

	_commandCharacteristic->setValue(buffer);
	_commandCharacteristic->setMaxLength(size);
	_commandCharacteristic->setDataLength(size);

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

		uint16_t size = 0;
		buffer_ptr_t buffer = NULL;

		//! if we don't use configuration characteristics, set up a buffer
		if (_streamBuffer == NULL) {
			_streamBuffer = new StreamBuffer<uint8_t>();

			LOGd("Assign buffer of size %i to stream buffer", size);
			_streamBuffer->assign(buffer, size);
		} else {
			//! otherwise use the same buffer
			_streamBuffer->getBuffer(buffer, size);
			size = _streamBuffer->getMaxLength();
		}

		addSetConfigurationCharacteristic();
		addSelectConfigurationCharacteristic();
		addGetConfigurationCharacteristic();

		_setConfigurationCharacteristic->setValue(buffer);
		_setConfigurationCharacteristic->setMaxLength(size);
		_setConfigurationCharacteristic->setDataLength(size);

		_getConfigurationCharacteristic->setValue(buffer);
		_getConfigurationCharacteristic->setMaxLength(size);
		_getConfigurationCharacteristic->setDataLength(size);

		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(MSG_CHAR_CONFIGURATION_SKIP);
#endif

#if CHAR_STATE_VARIABLES==1
	{
		LOGi(MSG_CHAR_STATEVARIABLES_ADD);

		uint16_t size = 0;
		buffer_ptr_t buffer = NULL;

		//! if we don't use configuration characteristics, set up a buffer
		if (_streamBuffer == NULL) {
			_streamBuffer = new StreamBuffer<uint8_t>();

			LOGd("Assign buffer of size %i to stream buffer", size);
			_streamBuffer->assign(buffer, size);
		} else {
			//! otherwise use the same buffer
			_streamBuffer->getBuffer(buffer, size);
			size = _streamBuffer->getMaxLength();
		}

		addSelectStateVarCharacteristic();
		addReadStateVarCharacteristic();

		_selectStateVarCharacteristic->setValue(buffer);
		_selectStateVarCharacteristic->setMaxLength(size);
		_selectStateVarCharacteristic->setDataLength(size);

		_readStateVarCharacteristic->setValue(buffer);
		_readStateVarCharacteristic->setMaxLength(size);
		_readStateVarCharacteristic->setDataLength(size);

		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(MSG_CHAR_STATEVARIABLES_SKIP);
#endif

	addCharacteristicsDone();
}

void GeneralService::tick() {
//	LOGi("Tick: %d", RTC::now());

	if (_temperatureCharacteristic) {
		int32_t temp;
		temp = getTemperature();
		writeToTemperatureCharac(temp);
#ifdef MICRO_VIEW
		//! Update temperature at the display
		write("1 %i\r\n", temp);
#endif
	}

	scheduleNextTick();
}

void GeneralService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(GENERAL_SERVICE_UPDATE_FREQUENCY), this);
}

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

void GeneralService::addCommandCharacteristic() {
	_commandCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_commandCharacteristic);

	_commandCharacteristic->setUUID(UUID(getUUID(), COMMAND_UUID));
	_commandCharacteristic->setName("Command");
	_commandCharacteristic->setWritable(true);
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

void GeneralService::addSetConfigurationCharacteristic() {
	_setConfigurationCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_setConfigurationCharacteristic);

	_setConfigurationCharacteristic->setUUID(UUID(getUUID(), SET_CONFIGURATION_UUID));
	_setConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_SET);
	_setConfigurationCharacteristic->setWritable(true);
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
				LOGi("Write configuration type: %i", (int)type);
				uint8_t *payload = _streamBuffer->payload();
				uint8_t length = _streamBuffer->length();

				Settings::getInstance().writeToStorage(type, payload, length);

				mb.unlock();
			} else {
				LOGe(MSG_BUFFER_IS_LOCKED);
			}
		}
	});
}

void GeneralService::writeToConfigCharac() {
	_getConfigurationCharacteristic->setDataLength(_streamBuffer->getDataLength());
	_getConfigurationCharacteristic->notify();
}

void GeneralService::addSelectConfigurationCharacteristic() {
	_selectConfigurationCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_selectConfigurationCharacteristic);

	_selectConfigurationCharacteristic->setUUID(UUID(getUUID(), SELECT_CONFIGURATION_UUID));
	_selectConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_SELECT);
	_selectConfigurationCharacteristic->setWritable(true);
	_selectConfigurationCharacteristic->onWrite([&](const uint8_t& value) -> void {
		if (value < CONFIG_TYPES) {
			LOGd("Select configuration type: %i", (int)value);

			bool success = Settings::getInstance().readFromStorage(value, _streamBuffer);
			if (success) {
				writeToConfigCharac();
			} else {
				LOGe("Failed to read from storage");
			}
		} else {
			LOGe("Cannot select %i", value);
		}
	});
}

void GeneralService::addGetConfigurationCharacteristic() {
	_getConfigurationCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_getConfigurationCharacteristic);

	_getConfigurationCharacteristic->setUUID(UUID(getUUID(), GET_CONFIGURATION_UUID));
	_getConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_GET);
	_getConfigurationCharacteristic->setWritable(false);
	_getConfigurationCharacteristic->setNotifies(true);
}

void GeneralService::writeToTemperatureCharac(int32_t temperature) {
	*_temperatureCharacteristic = temperature;
}

void GeneralService::addSelectStateVarCharacteristic() {
	_selectStateVarCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_selectStateVarCharacteristic);

	_selectStateVarCharacteristic->setUUID(UUID(getUUID(), SELECT_STATEVAR_UUID));
	_selectStateVarCharacteristic->setName(BLE_CHAR_STATEVAR_SELECT);
	_selectStateVarCharacteristic->setWritable(true);
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

				if (_streamBuffer->length() == 0) {
					StateVars::getInstance().readFromStorage(type, _streamBuffer);

					_readStateVarCharacteristic->setDataLength(_streamBuffer->getDataLength());
					_readStateVarCharacteristic->notify();
				} else {
					LOGi("write to state vars");
					StateVars::getInstance().writeToStorage(type, _streamBuffer->payload(), _streamBuffer->length());
				}

				mb.unlock();
			} else {
				LOGe(MSG_BUFFER_IS_LOCKED);
			}
		}
	});
}

void GeneralService::addReadStateVarCharacteristic() {
	_readStateVarCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_readStateVarCharacteristic);

	_readStateVarCharacteristic->setUUID(UUID(getUUID(), READ_STATEVAR_UUID));
	_readStateVarCharacteristic->setName(BLE_CHAR_STATEVAR_READ);
	_readStateVarCharacteristic->setWritable(false);
	_readStateVarCharacteristic->setNotifies(true);
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
	}
}

