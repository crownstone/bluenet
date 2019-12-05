/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <services/cs_CrownstoneService.h>

#include <storage/cs_State.h>
#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <storage/cs_State.h>
#include <protocol/cs_ErrorCodes.h>
#include <structs/cs_PacketsInternal.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>


CrownstoneService::CrownstoneService() : EventListener()
{
	EventDispatcher::getInstance().addListener(this);
	LOGi("Add crownstone service");

	setUUID(UUID(CROWNSTONE_UUID));
	setName(BLE_SERVICE_CROWNSTONE);
}

void CrownstoneService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_CROWNSTONE);

#if CHAR_CONTROL==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_CONTROL);
	cs_data_t writeBuf = CharacteristicWriteBuffer::getInstance().getBuffer();
	_controlPacketAccessor = new ControlPacketAccessor<>();
	_controlPacketAccessor->assign(writeBuf.data, writeBuf.len);
	addControlCharacteristic(writeBuf.data, writeBuf.len, CONTROL_UUID);

	cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
	_resultPacketAccessor = new ResultPacketAccessor<>();
	_resultPacketAccessor->assign(readBuf.data, readBuf.len);
	addResultCharacteristic(readBuf.data, readBuf.len, RESULT_UUID);
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_CONTROL);
#endif

	{
		LOGi(FMT_CHAR_ADD, STR_CHAR_SESSION_NONCE);
		cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
		addSessionNonceCharacteristic(readBuf.data, readBuf.len);
	}

	LOGi(FMT_CHAR_ADD, STR_CHAR_FACTORY_RESET);
	addFactoryResetCharacteristic();

	updatedCharacteristics();
}

void CrownstoneService::addControlCharacteristic(buffer_ptr_t buffer, cs_buffer_size_t size, uint16_t charUuid, EncryptionAccessLevel minimumAccessLevel) {
	if (_controlCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_CONTROL);
		return;
	}
	_controlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_controlCharacteristic);
	_controlCharacteristic->setUUID(UUID(getUUID(), charUuid));
	_controlCharacteristic->setName(BLE_CHAR_CONTROL);
	_controlCharacteristic->setWritable(true);
	_controlCharacteristic->setNotifies(false);
	_controlCharacteristic->setValue(buffer);
	_controlCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_controlCharacteristic->setMaxGattValueLength(size);
	_controlCharacteristic->setValueLength(0);
	_controlCharacteristic->onWrite([&](const EncryptionAccessLevel accessLevel, const buffer_ptr_t& value, uint16_t length) -> void {
		// Encryption in the write stage verifies if the key is at the lowest level, command specific permissions are handled in the CommandHandler.
		command_result_t result;
		CommandHandlerTypes type = CTRL_CMD_UNKNOWN;
		CharacteristicWriteBuffer& writeBuffer = CharacteristicWriteBuffer::getInstance();
		LOGd("controlCharacteristic onWrite");
		// At this point it is too late to check if writeBuffer was locked, because the softdevice doesn't care if the writeBuffer was locked,
		// it writes to the buffer in any case.
		if (!writeBuffer.isLocked()) {
			LOGi(MSG_CHAR_VALUE_WRITE);
			type = (CommandHandlerTypes) _controlPacketAccessor->getType();
			cs_data_t payload = _controlPacketAccessor->getPayload();
			cs_data_t resultData;
//			if (_resultPacketAccessor != NULL) {
			assert(_resultPacketAccessor != NULL, "_resultPacketAccessor is null");
			resultData.data = _resultPacketAccessor->getPayloadBuffer();
			resultData.len = _resultPacketAccessor->getMaxPayloadSize();
//			}

			result = CommandHandler::getInstance().handleCommand(type, payload, cmd_source_t(CS_CMD_SOURCE_CONNECTION), accessLevel, resultData);
			writeBuffer.unlock();
		}
		else {
			LOGe(MSG_BUFFER_IS_LOCKED);
			result = command_result_t(ERR_BUFFER_LOCKED);
		}

		[[maybe_unused]] uint8_t* buf = _resultPacketAccessor->getSerializedBuffer().data;
		LOGd("addControlCharacteristic result.returnCode %d, data len: %d", result.returnCode, result.data.len);
		for(auto i = 0; i < 50; i+=10){
			LOGd("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			buf[i+0],buf[i+1],buf[i+2],buf[i+3],buf[i+4],
			buf[i+5],buf[i+6],buf[i+7],buf[i+8],buf[i+9]);
		}

		writeResult(type, result);
	});
}

void CrownstoneService::addResultCharacteristic(buffer_ptr_t buffer, cs_buffer_size_t size, uint16_t charUuid, EncryptionAccessLevel minimumAccessLevel) {
	if (_resultCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_RESULT);
		return;
	}
	_resultCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_resultCharacteristic);
	_resultCharacteristic->setUUID(UUID(getUUID(), charUuid));
	_resultCharacteristic->setName(STR_CHAR_RESULT);
	_resultCharacteristic->setWritable(false);
	_resultCharacteristic->setNotifies(true);
	_resultCharacteristic->setValue(buffer);
	_resultCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_resultCharacteristic->setMaxGattValueLength(size);
	_resultCharacteristic->setValueLength(0);
}

void CrownstoneService::addSessionNonceCharacteristic(buffer_ptr_t buffer, cs_buffer_size_t size, EncryptionAccessLevel minimumAccessLevel) {
	if (_sessionNonceCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_SESSION_NONCE);
		return;
	}
	_sessionNonceCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_sessionNonceCharacteristic);
	_sessionNonceCharacteristic->setUUID(UUID(getUUID(), SESSION_NONCE_UUID));
	_sessionNonceCharacteristic->setName(BLE_CHAR_SESSION_NONCE);
	_sessionNonceCharacteristic->setWritable(false);
	_sessionNonceCharacteristic->setNotifies(true);
	_sessionNonceCharacteristic->setValue(buffer);
	_sessionNonceCharacteristic->setSharedEncryptionBuffer(false);
	_sessionNonceCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_sessionNonceCharacteristic->setMaxGattValueLength(size);
	_sessionNonceCharacteristic->setValueLength(0);
}

void CrownstoneService::addFactoryResetCharacteristic() {
	_factoryResetCharacteristic = new Characteristic<uint32_t>();
	addCharacteristic(_factoryResetCharacteristic);
	_factoryResetCharacteristic->setUUID(UUID(getUUID(), FACTORY_RESET_UUID));
	_factoryResetCharacteristic->setName(BLE_CHAR_FACTORY_RESET);
	_factoryResetCharacteristic->setWritable(true);
	_factoryResetCharacteristic->setNotifies(true);
	_factoryResetCharacteristic->setDefaultValue(0);
	_factoryResetCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_factoryResetCharacteristic->onWrite([&](const uint8_t accessLevel, const uint32_t& value, uint16_t length) -> void {
		// TODO if settings --> factory reset disabled, we set the value to 2 to indicate reset is not possible.
		// No need to check length, as value is not a pointer.
		bool success = FactoryReset::getInstance().recover(value);
		uint32_t val = 0;
		if (success) {
			val = 1;
		}
		_factoryResetCharacteristic->operator=(val);
	});
}

void CrownstoneService::writeResult(CommandHandlerTypes type, command_result_t result) {
	if (_resultCharacteristic == NULL) {
		return;
	}
	assert(_resultPacketAccessor != NULL, "_resultPacketAccessor is null");
	// Payload has already been set by command handler.
	// But the size hasn't been set yet.
//	cs_ret_code_t retVal = _resultPacketAccessor->setPayload(result.data.data, result.data.len);
	assert(result.data.len == 0 || result.data.data == _resultPacketAccessor->getPayloadBuffer(), "Wrong buffer");
	cs_ret_code_t retVal = _resultPacketAccessor->setPayloadSize(result.data.len);
	if (!SUCCESS(retVal)) {
		LOGe("Unable to set result: %u", retVal);
		result.returnCode = retVal;
		_resultPacketAccessor->setPayloadSize(0);
	}
	LOGd("Result: type=%u code=%u size=%u", type, result.returnCode, result.data.len);
	_resultPacketAccessor->setType(type);
	_resultPacketAccessor->setResult(result.returnCode);
	_resultCharacteristic->setValueLength(_resultPacketAccessor->getSerializedSize());
	_resultCharacteristic->updateValue();
}

void CrownstoneService::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::EVT_SESSION_NONCE_SET: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_sessionNonceCharacteristic != NULL) {
			_sessionNonceCharacteristic->setValueLength(event.size);
			_sessionNonceCharacteristic->setValue((uint8_t*)event.data);
			_sessionNonceCharacteristic->updateValue(ECB_GUEST_CAFEBABE);
		}
		break;
	}
	case CS_TYPE::EVT_BLE_CONNECT: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_factoryResetCharacteristic != NULL) {
			_factoryResetCharacteristic->operator=(0);
		}
		if (_resultCharacteristic != NULL) {
			_resultCharacteristic->setValueLength(0);
		}
		break;
	}
	case CS_TYPE::EVT_BLE_DISCONNECT: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_sessionNonceCharacteristic != NULL) {
			_sessionNonceCharacteristic->setValueLength(0);
		}
		break;
	}
	case CS_TYPE::EVT_SETUP_DONE: {
		writeResult(CTRL_CMD_SETUP, command_result_t(ERR_SUCCESS));
		break;
	}
	default: {}
	}
}
