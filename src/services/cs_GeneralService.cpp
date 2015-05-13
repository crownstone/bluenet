/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "services/cs_GeneralService.h"

#include "drivers/cs_Timer.h"

#include "cfg/cs_UuidConfig.h"
#include "structs/buffer/cs_MasterBuffer.h"
#include "drivers/cs_Temperature.h"
//#include "common/cs_Config.h"
//#include "common/cs_Strings.h"
#include "drivers/cs_RTC.h"
//
#if CHAR_MESHING==1
#include <protocol/cs_Mesh.h>
#include <structs/cs_MeshMessage.h>
#endif

#include <cfg/cs_Settings.h>

using namespace BLEpp;

GeneralService::GeneralService() :
		_temperatureCharacteristic(NULL),
		_resetCharacteristic(NULL),
		_selectConfiguration(0xFF) {

	setUUID(UUID(GENERAL_UUID));
	setName(BLE_SERVICE_GENERAL);

	Settings::getInstance();

//	Storage::getInstance().getHandle(PS_ID_GENERAL_SERVICE, _storageHandle);
//	loadPersistentStorage();

	init();

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)GeneralService::staticTick);
}

void GeneralService::init() {
	LOGi(MSG_SERVICE_GENERAL_INIT);

#if CHAR_TEMPERATURE==1
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
	LOGi(MSG_CHAR_MESH_ADD);
	addMeshCharacteristic();
#else
	LOGi(MSG_CHAR_MESH_SKIP);
#endif

#if CHAR_CONFIGURATION==1
	LOGi(MSG_CHAR_CONFIGURATION_ADD);

	// if we use configuration characteristics, set up a buffer
	_streamBuffer = new StreamBuffer<uint8_t>();
	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t size = 0;
	mb.getBuffer(buffer, size);

	LOGd("Assign buffer of size %i to stream buffer", size);
	_streamBuffer->assign(buffer, size);

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
#else
	LOGi(MSG_CHAR_CONFIGURATION_SKIP);
#endif
	
}

//void GeneralService::startAdvertising(Nrf51822BluetoothStack* stack) {
//	Service::startAdvertising(stack);
//	std::string str = ConfigHelper::getInstance().getBLEName();
//	LOGi("setDeviceName");
//	BLEpp::Nrf51822BluetoothStack::getInstance().setDeviceName(str);
////	ps_configuration_t cfg = ConfigHelper::getInstance().getConfig();
////	Storage::getString(cfg.device_name, str, getBLEName());
////	setBLEName(str);
//}

void GeneralService::tick() {
//	LOGi("Tick: %d", RTC::now());

	if (_temperatureCharacteristic) {
		int32_t temp;
		temp = getTemperature();
		writeToTemperatureCharac(temp);
#ifdef MICRO_VIEW
		// Update temperature at the display
		write("1 %i\r\n", temp);
#endif
	}

	if (_getConfigurationCharacteristic) {
		if (_selectConfiguration != 0xFF) {
			bool success = Settings::getInstance().readFromStorage(_selectConfiguration, _streamBuffer);
			if (success) {
				writeToConfigCharac();
			}
			// only write once
			_selectConfiguration = 0xFF;
		}
	}

	scheduleNextTick();
}

void GeneralService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(GENERAL_SERVICE_UPDATE_FREQUENCY), this);
}

//void GeneralService::loadPersistentStorage() {
//	Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
//}

//void GeneralService::savePersistentStorage() {
//	Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
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
			if (value != 0) {
				// copy to make sure this is nothing more than one value
				uint32_t cmd = value;
				if (cmd == COMMAND_ENTER_RADIO_BOOTLOADER) {
					LOGi(MSG_FIRMWARE_UPDATE);
				} else {
					LOGi(MSG_RESET);
				}
				uint8_t err_code;
				err_code = sd_power_gpregret_clr(0xFF);
				APP_ERROR_CHECK(err_code);
				err_code = sd_power_gpregret_set(cmd);
				APP_ERROR_CHECK(err_code);
				sd_nvic_SystemReset();				
			} else {
				LOGw("To reset write a nonzero value");
			}
		});
}

#if CHAR_MESHING==1
void GeneralService::addMeshCharacteristic() {
	buffer_ptr_t buffer = MasterBuffer::getInstance().getBuffer();

	_meshCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_meshCharacteristic);

	_meshCharacteristic->setUUID(UUID(getUUID(), MESH_UUID));
	_meshCharacteristic->setName(BLE_CHAR_MESH);
	_meshCharacteristic->setWritable(true);
	_meshCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {
			LOGi(MSG_MESH_MESSAGE_WRITE);

			MeshMessage msg;
			msg.assign(_meshCharacteristic->getValue(), _meshCharacteristic->getValueLength());

			uint8_t handle = msg.handle();
			uint8_t val = msg.value();
			CMesh &mesh = CMesh::getInstance();
			mesh.send(handle, val);
		});

	_meshCharacteristic->setValue(buffer);
	_meshCharacteristic->setMaxLength(MM_SERIALIZED_SIZE);
	_meshCharacteristic->setDataLength(0);

}
#endif

void GeneralService::addSetConfigurationCharacteristic() {
	_setConfigurationCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_setConfigurationCharacteristic);

	_setConfigurationCharacteristic->setUUID(UUID(getUUID(), SET_CONFIGURATION_UUID));
	_setConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_SET);
	_setConfigurationCharacteristic->setWritable(true);
	_setConfigurationCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {

			if (!value) {
				log(WARNING, MSG_CHAR_VALUE_UNDEFINED);
			} else {
				log(INFO, MSG_CHAR_VALUE_WRITE);
				MasterBuffer& mb = MasterBuffer::getInstance();
				if (!mb.isLocked()) {
					mb.lock();
					uint8_t type = _streamBuffer->type();
					LOGi("Write configuration type: %i", (int)type);
//					uint8_t *payload = _streamBuffer->payload();
//					uint8_t length = _streamBuffer->length();
//					writeToStorage(type, length, payload);
					Settings::getInstance().writeToStorage(type, _streamBuffer);
					mb.unlock();
				} else {
					log(ERROR, MSG_BUFFER_IS_LOCKED);
				}
			}
		});
}

void GeneralService::writeToConfigCharac() {
	/*
	uint16_t len = _streamBuffer->getDataLength();
	uint8_t value = _streamBuffer->payload()[0];
	buffer_ptr_t pntr = _getConfigurationCharacteristic->getValue();
	LOGd("Write to config length %i and value %i", len, value);
	struct stream_t<uint8_t> *stream = (struct stream_t<uint8_t>*) pntr;
	LOGd("Write to config at %p with payload length %i and value %i", pntr, stream->length, stream->payload[0]);
	*/
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
				_selectConfiguration = value;
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
}

//std::string & GeneralService::getBLEName() {
//	_name = "Unknown";
//	if (_stack) {
//		return _name = _stack->getDeviceName();
//	}
//	return _name;
//}

//void GeneralService::setBLEName(const std::string &name) {
//	if (name.length() > 31) {
//		log(ERROR, MSG_NAME_TOO_LONG);
//		return;
//	}
//	if (_stack) {
//		_stack->updateDeviceName(name);
//	} else {
//		log(ERROR, MSG_STACK_UNDEFINED);
//	}
//}

void GeneralService::writeToTemperatureCharac(int32_t temperature) {
	*_temperatureCharacteristic = temperature;
}
