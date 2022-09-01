/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Strings.h>
#include <cfg/cs_UuidConfig.h>
#include <cs_GpRegRetConfig.h>
#include <encryption/cs_KeysAndAccess.h>
#include <events/cs_EventDispatcher.h>
#include <nrf_soc.h>
#include <processing/cs_CommandHandler.h>
#include <services/cs_SetupService.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <util/cs_BleError.h>
#include <structs/cs_ControlPacketAccessor.h>
#include <structs/cs_ResultPacketAccessor.h>

SetupService::SetupService() : _macAddressCharacteristic(NULL), _setupKeyCharacteristic(NULL) {
	setUUID(UUID(SETUP_UUID));
}

void SetupService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT "Setup");

	cs_data_t writeBuf     = CharacteristicWriteBuffer::getInstance().getBuffer();
	_controlPacketAccessor = new ControlPacketAccessor<>();
	_controlPacketAccessor->assign(writeBuf.data, writeBuf.len);
	addControlCharacteristic(writeBuf.data, writeBuf.len, SETUP_CONTROL_UUID, SETUP);
	LOGi(FMT_CHAR_ADD STR_CHAR_CONTROL);

	cs_data_t readBuf     = CharacteristicReadBuffer::getInstance().getBuffer();
	_resultPacketAccessor = new ResultPacketAccessor<>();
	_resultPacketAccessor->assign(readBuf.data, readBuf.len);
	addResultCharacteristic(readBuf.data, readBuf.len, SETUP_RESULT_UUID, SETUP);
	LOGi(FMT_CHAR_ADD STR_CHAR_RESULT);

	addMacAddressCharacteristic();
	LOGi(FMT_CHAR_ADD STR_CHAR_MAC_ADDRESS);

	addSetupKeyCharacteristic(_keyBuffer, sizeof(_keyBuffer));
	LOGi(FMT_CHAR_ADD STR_CHAR_SETUP_KEY);

	addSessionDataCharacteristic(readBuf.data, readBuf.len, SETUP);
	LOGi(FMT_CHAR_ADD STR_CHAR_SESSION_DATA);
}

void SetupService::addMacAddressCharacteristic() {
	if (_macAddressCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS STR_CHAR_MAC_ADDRESS);
		return;
	}

	characteristic_options_t config = {
			.read = true,
			.write = false,
			.notify = false,
			.encrypted = false,
	};

	sd_ble_gap_addr_get(&_myAddr);

	_macAddressCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_macAddressCharacteristic);
	_macAddressCharacteristic->setName(BLE_CHAR_MAC_ADDRES);
	_macAddressCharacteristic->setUuid(MAC_ADDRESS_UUID);
	_macAddressCharacteristic->setOptions(config);
	_macAddressCharacteristic->setValueBuffer(_myAddr.addr, sizeof(_myAddr.addr));
	_macAddressCharacteristic->setInitialValueLength(sizeof(_myAddr.addr));
}

void SetupService::addSetupKeyCharacteristic(buffer_ptr_t buffer, uint16_t size) {
	if (_setupKeyCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS STR_CHAR_SETUP_KEY);
		return;
	}

	characteristic_options_t config = {
			.read = true,
			.write = false,
			.notify = false,
			.encrypted = false,
	};

	sd_ble_gap_addr_get(&_myAddr);

	_setupKeyCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_setupKeyCharacteristic);
	_setupKeyCharacteristic->setName(BLE_CHAR_SETUP_KEY);
	_setupKeyCharacteristic->setUuid(SETUP_KEY_UUID);
	_setupKeyCharacteristic->setOptions(config);
	_setupKeyCharacteristic->setValueBuffer(buffer, size);
}

void SetupService::handleEvent(event_t& event) {
	// make sure the session key is populated.
	CrownstoneService::handleEvent(event);
	switch (event.type) {
		case CS_TYPE::EVT_SESSION_DATA_SET: {
			cs_data_t key = KeysAndAccess::getInstance().getSetupKey();
			if (key.data != nullptr && sizeof(_keyBuffer) == key.len) {
				memcpy(_keyBuffer, key.data, sizeof(_keyBuffer));
				_setupKeyCharacteristic->updateValue(key.len);
			}
			break;
		}
		default: {
		}
	}
}
