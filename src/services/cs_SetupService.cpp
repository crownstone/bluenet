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

SetupService::SetupService() : _macAddressCharacteristic(NULL) {
	setUUID(UUID(SETUP_UUID));
	setName(BLE_SERVICE_SETUP);
}


void SetupService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, "Setup");

	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;

	LOGi(FMT_CHAR_ADD, STR_CHAR_CONTROL);
	_streamBuffer = getStreamBuffer(buffer, maxLength);
	addControlCharacteristic(buffer, maxLength);

	LOGi(FMT_CHAR_ADD, STR_CHAR_MAC_ADDRESS);
	addMacAddressCharacteristic();

	LOGi(FMT_CHAR_ADD, STR_CHAR_CONFIGURATION);
	_streamBuffer = getStreamBuffer(buffer, maxLength);
	addConfigurationControlCharacteristic(buffer, maxLength, ENCRYPTION_DISABLED);
	addConfigurationReadCharacteristic(buffer, maxLength, ENCRYPTION_DISABLED);

	addCharacteristicsDone();
}


void SetupService::addMacAddressCharacteristic() {
    sd_ble_gap_address_get(&_myAddr);

	_macAddressCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_macAddressCharacteristic);

	_macAddressCharacteristic->setUUID(UUID(getUUID(), MAC_ADDRESS_UUID));
	_macAddressCharacteristic->setName(BLE_CHAR_MAC_ADDRES);
	_macAddressCharacteristic->setWritable(false);
	_macAddressCharacteristic->setValue(_myAddr.addr);
	_macAddressCharacteristic->setMaxGattValueLength(BLE_GAP_ADDR_LEN);
	_macAddressCharacteristic->setValueLength(BLE_GAP_ADDR_LEN);

}

