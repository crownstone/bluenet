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
	addControlCharacteristic(writeBuf.data, writeBuf.len, CONTROL_UUID, BASIC);

	cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
	_resultPacketAccessor = new ResultPacketAccessor<>();
	_resultPacketAccessor->assign(readBuf.data, readBuf.len);
	addResultCharacteristic(readBuf.data, readBuf.len, RESULT_UUID, BASIC);
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_CONTROL);
#endif

	{
		LOGi(FMT_CHAR_ADD, STR_CHAR_SESSION_DATA);
		cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
		addSessionDataCharacteristic(readBuf.data, readBuf.len);
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
		cs_result_t result;
		CommandHandlerTypes type = CTRL_CMD_UNKNOWN;
		uint8_t protocol = CS_CONNECTION_PROTOCOL_VERSION;
		CharacteristicWriteBuffer& writeBuffer = CharacteristicWriteBuffer::getInstance();
		LOGd("controlCharacteristic onWrite buf=%p size=%u", value, length);
		// At this point it is too late to check if writeBuffer was locked, because the softdevice doesn't care if the writeBuffer was locked,
		// it writes to the buffer in any case.
		if (!writeBuffer.isLocked()) {
			LOGi(MSG_CHAR_VALUE_WRITE);
			protocol = _controlPacketAccessor->getProtocolVersion();
			type = (CommandHandlerTypes) _controlPacketAccessor->getType();
			cs_data_t payload = _controlPacketAccessor->getPayload();
//			if (_resultPacketAccessor != NULL) {
			assert(_resultPacketAccessor != NULL, "_resultPacketAccessor is null");
			result.buf.data = _resultPacketAccessor->getPayloadBuffer();
			result.buf.len = _resultPacketAccessor->getMaxPayloadSize();
//			}

			CommandHandler::getInstance().handleCommand(protocol, type, payload, cmd_source_with_counter_t(CS_CMD_SOURCE_CONNECTION), accessLevel, result);
			writeBuffer.unlock();
		}
		else {
			LOGe(MSG_BUFFER_IS_LOCKED);
			result.returnCode = ERR_BUFFER_LOCKED;
		}

		LOGd("addControlCharacteristic returnCode=%u dataSize=%u", result.returnCode, result.dataSize);
//		[[maybe_unused]] uint8_t* buf = _resultPacketAccessor->getSerializedBuffer().data;
//		for(auto i = 0; i < 50; i+=10){
//			LOGd("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
//			buf[i+0],buf[i+1],buf[i+2],buf[i+3],buf[i+4],
//			buf[i+5],buf[i+6],buf[i+7],buf[i+8],buf[i+9]);
//		}

		writeResult(protocol, type, result);
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

void CrownstoneService::addSessionDataCharacteristic(buffer_ptr_t buffer, cs_buffer_size_t size, EncryptionAccessLevel minimumAccessLevel) {
	if (_sessionDataCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_SESSION_DATA);
		return;
	}
	_sessionDataCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_sessionDataCharacteristic);
	_sessionDataCharacteristic->setUUID(UUID(getUUID(), SESSION_DATA_UUID));
	_sessionDataCharacteristic->setName(BLE_CHAR_SESSION_DATA);
	_sessionDataCharacteristic->setWritable(false);
	_sessionDataCharacteristic->setNotifies(true);
	_sessionDataCharacteristic->setValue(buffer);
	_sessionDataCharacteristic->setSharedEncryptionBuffer(false);
	_sessionDataCharacteristic->setMinAccessLevel(minimumAccessLevel);
	_sessionDataCharacteristic->setMaxGattValueLength(size);
	_sessionDataCharacteristic->setValueLength(0);
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

void CrownstoneService::writeResult(uint8_t protocol, CommandHandlerTypes type, cs_result_t & result) {
	if (_resultCharacteristic == NULL) {
		return;
	}
	assert(_resultPacketAccessor != NULL, "_resultPacketAccessor is null");
	// Payload has already been set by command handler.
	// But the size hasn't been set yet.
//	cs_ret_code_t retVal = _resultPacketAccessor->setPayload(result.data.data, result.data.len);
	assert(result.dataSize == 0 || result.buf.data == _resultPacketAccessor->getPayloadBuffer(), "Wrong buffer");
	cs_ret_code_t retVal = _resultPacketAccessor->setPayloadSize(result.dataSize);
	if (!SUCCESS(retVal)) {
		LOGe("Unable to set result: %u", retVal);
		result.returnCode = retVal;
		_resultPacketAccessor->setPayloadSize(0);
	}
	LOGd("Result: protocol=%u type=%u code=%u size=%u", protocol, type, result.returnCode, result.dataSize);
	_resultPacketAccessor->setProtocolVersion(protocol);
	_resultPacketAccessor->setType(type);
	_resultPacketAccessor->setResult(result.returnCode);
	_resultCharacteristic->setValueLength(_resultPacketAccessor->getSerializedSize());
	_resultCharacteristic->updateValue();
}

void CrownstoneService::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::EVT_SESSION_DATA_SET: {
		// Check if this characteristic exists first. In case of setup mode it does not for instance.
		if (_sessionDataCharacteristic != NULL) {
			_sessionDataCharacteristic->setValueLength(event.size);
			_sessionDataCharacteristic->setValue((uint8_t*)event.data);
			_sessionDataCharacteristic->updateValue(ECB_GUEST_CAFEBABE);
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
		if (_sessionDataCharacteristic != NULL) {
			_sessionDataCharacteristic->setValueLength(0);
		}
		break;
	}
	case CS_TYPE::EVT_SETUP_DONE: {
		cs_result_t result(ERR_SUCCESS);
		writeResult(CS_CONNECTION_PROTOCOL_VERSION, CTRL_CMD_SETUP, result);
//		writeResult(CTRL_CMD_SETUP, cs_result_t(ERR_SUCCESS));
		break;
	}
	default: {}
	}
}
