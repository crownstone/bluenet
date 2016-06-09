/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/cs_SetupService.h>

#include <storage/cs_Settings.h>
#include <cfg/cs_UuidConfig.h>
#include <processing/cs_CommandHandler.h>
#include <structs/buffer/cs_MasterBuffer.h>

using namespace BLEpp;

SetupService::SetupService() :
		EventListener(),
		_streamBuffer(NULL)

{
//	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(SETUP_UUID));
	setName(BLE_SERVICE_SETUP);

	addCharacteristics();

//	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) GeneralService::staticTick);
}

StreamBuffer<uint8_t>* SetupService::getStreamBuffer(buffer_ptr_t& buffer, uint16_t& maxLength) {
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

void SetupService::addCharacteristics() {
	LOGi("Create setup service");

	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;

	LOGi(MSG_CHAR_CONTROL_ADD);
	_streamBuffer = getStreamBuffer(buffer, maxLength);
	addControlCharacteristic(buffer, maxLength);

	addMacAddressCharacteristic();

#if CHAR_CONFIGURATION==1 || DEVICE_TYPE==DEVICE_FRIDGE
	{
		LOGi(MSG_CHAR_CONFIGURATION_ADD);
		_streamBuffer = getStreamBuffer(buffer, maxLength);
		addSetConfigurationCharacteristic(buffer, maxLength);
		addGetConfigurationCharacteristic(buffer, maxLength);

		LOGd("Set both set/get charac to buffer at %p", buffer);
	}
#else
	LOGi(MSG_CHAR_CONFIGURATION_SKIP);
#endif

//#if CHAR_STATE_VARIABLES==1
//	{
//		LOGi(MSG_CHAR_STATEVARIABLES_ADD);
//		_streamBuffer = getCharacBuffer(buffer, maxLength);
//		addSelectStateVarCharacteristic(buffer, maxLength);
//		addReadStateVarCharacteristic(buffer, maxLength);
//
//		LOGd("Set both set/get charac to buffer at %p", buffer);
//	}
//#else
//	LOGi(MSG_CHAR_STATEVARIABLES_SKIP);
//#endif
//

	addCharacteristicsDone();
}

//void GeneralService::tick() {
//	scheduleNextTick();
//}

//void GeneralService::scheduleNextTick() {
//	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(GENERAL_SERVICE_UPDATE_FREQUENCY), this);
//}

void SetupService::addControlCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_controlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_controlCharacteristic);

	_controlCharacteristic->setUUID(UUID(getUUID(), CONTROL_UUID));
	_controlCharacteristic->setName(BLE_CHAR_CONTROL);
	_controlCharacteristic->setWritable(true);
	_controlCharacteristic->setValue(buffer);
	_controlCharacteristic->setMaxLength(size);
	_controlCharacteristic->setDataLength(size);
	_controlCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {

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

void SetupService::addMacAddressCharacteristic() {
    sd_ble_gap_address_get(&_myAddr);

	_macAddressCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_macAddressCharacteristic);

	_macAddressCharacteristic->setUUID(UUID(getUUID(), MAC_ADDRESS_UUID));
	_macAddressCharacteristic->setName(BLE_CHAR_MAC_ADDRES);
	_macAddressCharacteristic->setWritable(false);
	_macAddressCharacteristic->setValue(_myAddr.addr);
	_macAddressCharacteristic->setMaxLength(BLE_GAP_ADDR_LEN);
	_macAddressCharacteristic->setDataLength(BLE_GAP_ADDR_LEN);

}

void SetupService::addSetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_setConfigurationCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_setConfigurationCharacteristic);

	_setConfigurationCharacteristic->setUUID(UUID(getUUID(), CONFIG_CONTROL_UUID));
	_setConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_CONTROL);
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

void SetupService::addGetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	_getConfigurationCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_getConfigurationCharacteristic);

	_getConfigurationCharacteristic->setUUID(UUID(getUUID(), CONFIG_READ_UUID));
	_getConfigurationCharacteristic->setName(BLE_CHAR_CONFIG_READ);
	_getConfigurationCharacteristic->setWritable(false);
	_getConfigurationCharacteristic->setNotifies(true);
	_getConfigurationCharacteristic->setValue(buffer);
	_getConfigurationCharacteristic->setMaxLength(size);
	_getConfigurationCharacteristic->setDataLength(size);
}
