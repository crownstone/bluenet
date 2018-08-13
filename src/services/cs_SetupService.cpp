/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <services/cs_SetupService.h>

#include <storage/cs_State.h>
#include <cfg/cs_UuidConfig.h>
#include <processing/cs_CommandHandler.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include "nrf_soc.h"

SetupService::SetupService() : _macAddressCharacteristic(NULL), _setupKeyCharacteristic(NULL) {
	setUUID(UUID(SETUP_UUID));
	setName(BLE_SERVICE_SETUP);
}

void SetupService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, "Setup");

	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;

	_streamBuffer = getStreamBuffer(buffer, maxLength);
	addControlCharacteristic(buffer, maxLength, SETUP_CONTROL_UUID, SETUP);
	LOGi(FMT_CHAR_ADD, STR_CHAR_CONTROL);

	addMacAddressCharacteristic();
	LOGi(FMT_CHAR_ADD, STR_CHAR_MAC_ADDRESS);

	addSetupKeyCharacteristic(_keyBuffer, SOC_ECB_KEY_LENGTH);
	LOGi(FMT_CHAR_ADD, STR_CHAR_SETUP_KEY);


//	LOGi(FMT_CHAR_ADD, STR_CHAR_CONFIGURATION);
//	addConfigurationControlCharacteristic(buffer, maxLength);
//	addConfigurationReadCharacteristic(buffer, maxLength, SETUP);

	LOGi(FMT_CHAR_ADD, BLE_CHAR_GOTO_DFU);
	addGoToDfuCharacteristic();

	LOGi(FMT_CHAR_ADD, STR_CHAR_SESSION_NONCE);
	addSessionNonceCharacteristic(_nonceBuffer, SESSION_NONCE_LENGTH, ENCRYPTION_DISABLED);

	addCharacteristicsDone();
}


void SetupService::addMacAddressCharacteristic() {
    sd_ble_gap_addr_get(&_myAddr);

    _macAddressCharacteristic = new Characteristic<buffer_ptr_t>();
    addCharacteristic(_macAddressCharacteristic);

    _macAddressCharacteristic->setUUID(UUID(getUUID(), MAC_ADDRESS_UUID));
    _macAddressCharacteristic->setName(BLE_CHAR_MAC_ADDRES);
    _macAddressCharacteristic->setWritable(false);
    _macAddressCharacteristic->setValue(_myAddr.addr);
    _macAddressCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
    _macAddressCharacteristic->setMaxGattValueLength(BLE_GAP_ADDR_LEN);
    _macAddressCharacteristic->setValueLength(BLE_GAP_ADDR_LEN);
}

void SetupService::addSetupKeyCharacteristic(buffer_ptr_t buffer, uint16_t size) {
    _setupKeyCharacteristic = new Characteristic<buffer_ptr_t>();
    addCharacteristic(_setupKeyCharacteristic);

    _setupKeyCharacteristic->setUUID(UUID(getUUID(), SETUP_KEY_UUID));
    _setupKeyCharacteristic->setName(BLE_CHAR_SETUP_KEY);
    _setupKeyCharacteristic->setWritable(false);
    _setupKeyCharacteristic->setValue(buffer);
    _setupKeyCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
    _setupKeyCharacteristic->setMaxGattValueLength(size);
    _setupKeyCharacteristic->setValueLength(0);
}

void SetupService::addGoToDfuCharacteristic() {
	_gotoDfuCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_gotoDfuCharacteristic);

	_gotoDfuCharacteristic->setUUID(UUID(getUUID(), GOTO_DFU_UUID));
	_gotoDfuCharacteristic->setName(BLE_CHAR_GOTO_DFU);
	_gotoDfuCharacteristic->setWritable(true);
	_gotoDfuCharacteristic->setDefaultValue(0);
	_gotoDfuCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_gotoDfuCharacteristic->onWrite([&](const uint8_t accessLevel, const uint8_t& value, uint16_t length) -> void {
		if (value == GPREGRET_DFU_RESET) {
			LOGi("goto dfu");
			CommandHandler::getInstance().resetDelayed(value);
		}
		else {
			LOGe("goto dfu failed, wrong value: %d", value);
		}
	});
}

void SetupService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	// make sure the session nonce is poplated.
	CrownstoneService::handleEvent(evt, p_data, length);
	switch (evt) {
	case EVT_BLE_CONNECT: {
		uint8_t* key = EncryptionHandler::getInstance().generateNewSetupKey();
		_setupKeyCharacteristic->setValueLength(SOC_ECB_KEY_LENGTH);
		_setupKeyCharacteristic->setValue(key);
		_setupKeyCharacteristic->updateValue();
		break;
	}
	case EVT_BLE_DISCONNECT: {
		EncryptionHandler::getInstance().invalidateSetupKey();
		break;
	}
	}
}

