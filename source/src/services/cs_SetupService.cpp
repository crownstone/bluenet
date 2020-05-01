/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Strings.h>
#include <cfg/cs_UuidConfig.h>
#include <cs_GpRegRetConfig.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandHandler.h>
#include <services/cs_SetupService.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <util/cs_BleError.h>

#include <nrf_soc.h>

SetupService::SetupService() :
    _macAddressCharacteristic(NULL),
    _setupKeyCharacteristic(NULL),
    _gotoDfuCharacteristic(NULL)
{
	setUUID(UUID(SETUP_UUID));
	setName(BLE_SERVICE_SETUP);
}

void SetupService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, "Setup");

	cs_data_t writeBuf = CharacteristicWriteBuffer::getInstance().getBuffer();
	_controlPacketAccessor = new ControlPacketAccessor<>();
	_controlPacketAccessor->assign(writeBuf.data, writeBuf.len);
	addControlCharacteristic(writeBuf.data, writeBuf.len, SETUP_CONTROL_UUID, SETUP);
	LOGi(FMT_CHAR_ADD, STR_CHAR_CONTROL);

	cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
	_resultPacketAccessor = new ResultPacketAccessor<>();
	_resultPacketAccessor->assign(readBuf.data, readBuf.len);
	addResultCharacteristic(readBuf.data, readBuf.len, SETUP_RESULT_UUID, SETUP);
	LOGi(FMT_CHAR_ADD, STR_CHAR_RESULT);

	addMacAddressCharacteristic();
	LOGi(FMT_CHAR_ADD, STR_CHAR_MAC_ADDRESS);

	addSetupKeyCharacteristic(_keyBuffer, sizeof(_keyBuffer));
	LOGi(FMT_CHAR_ADD, STR_CHAR_SETUP_KEY);

	LOGi(FMT_CHAR_ADD, BLE_CHAR_GOTO_DFU);
	addGoToDfuCharacteristic();

	LOGi(FMT_CHAR_ADD, STR_CHAR_SESSION_DATA);
	addSessionDataCharacteristic(readBuf.data, readBuf.len, SETUP);

	updatedCharacteristics();
}

void SetupService::addMacAddressCharacteristic() {
	if (_macAddressCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_MAC_ADDRESS);
		return;
	}
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
	if (_setupKeyCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_SETUP_KEY);
		return;
	}
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
	if (_gotoDfuCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_GOTO_DFU);
		return;
	}
	_gotoDfuCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_gotoDfuCharacteristic);

	_gotoDfuCharacteristic->setUUID(UUID(getUUID(), GOTO_DFU_UUID));
	_gotoDfuCharacteristic->setName(BLE_CHAR_GOTO_DFU);
	_gotoDfuCharacteristic->setWritable(true);
	_gotoDfuCharacteristic->setDefaultValue(0);
	_gotoDfuCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_gotoDfuCharacteristic->onWrite([&](const uint8_t accessLevel, const uint8_t& value, uint16_t length) -> void {
		if (value == CS_GPREGRET_LEGACY_DFU_RESET) {
			LOGi("goto dfu");
			TYPIFY(CMD_RESET_DELAYED) resetCmd;
			resetCmd.resetCode = CS_RESET_CODE_SOFT_RESET;
			resetCmd.delayMs = 2000;
			event_t eventReset(CS_TYPE::CMD_RESET_DELAYED, &resetCmd, sizeof(resetCmd));
			EventDispatcher::getInstance().dispatch(eventReset);
		}
		else {
			LOGe("goto dfu failed, wrong value: %d", value);
		}
	});
}

void SetupService::handleEvent(event_t & event) {
	// make sure the session key is populated.
	CrownstoneService::handleEvent(event);
	switch(event.type) {
	case CS_TYPE::EVT_BLE_CONNECT: {
		uint8_t* key = EncryptionHandler::getInstance().getSetupKey();
		if (key != nullptr) {
			_setupKeyCharacteristic->setValueLength(SOC_ECB_KEY_LENGTH);
			_setupKeyCharacteristic->setValue(key);
			_setupKeyCharacteristic->updateValue();
		}
		break;
	}
	case CS_TYPE::EVT_BLE_DISCONNECT: {
		EncryptionHandler::getInstance().invalidateSetupKey();
		break;
	}
	default: {}
	}
}

