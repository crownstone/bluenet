/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <protocol/cs_ErrorCodes.h>
#include <services/cs_CrownstoneService.h>
#include <storage/cs_State.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <structs/cs_PacketsInternal.h>

CrownstoneService::CrownstoneService() : EventListener()
{
	EventDispatcher::getInstance().addListener(this);
	LOGi("Add crownstone service");

	setUUID(UUID(CROWNSTONE_UUID));
	setName(BLE_SERVICE_CROWNSTONE);
}

void CrownstoneService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_CROWNSTONE);

	cs_data_t writeBuf = CharacteristicWriteBuffer::getInstance().getBuffer();
	_controlPacketAccessor = new ControlPacketAccessor<>();
	_controlPacketAccessor->assign(writeBuf.data, writeBuf.len);
	addControlCharacteristic(writeBuf.data, writeBuf.len, CONTROL_UUID, BASIC);

	cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
	_resultPacketAccessor = new ResultPacketAccessor<>();
	_resultPacketAccessor->assign(readBuf.data, readBuf.len);
	addResultCharacteristic(readBuf.data, readBuf.len, RESULT_UUID, BASIC);

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
			protocol = _controlPacketAccessor->getProtocolVersion();
			type = (CommandHandlerTypes) _controlPacketAccessor->getType();
			LOGi(MSG_CHAR_VALUE_WRITE " (command 0x%x=%i)", type, type);
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

		_log(SERIAL_DEBUG, false, "addControlCharacteristic returnCode=%u dataSize=%u ", result.returnCode, result.dataSize);
		_logArray(SERIAL_DEBUG, true, _resultPacketAccessor->getSerializedBuffer().data, _resultPacketAccessor->getSerializedBuffer().len);

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

	if (_sessionDataUnencryptedCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_SESSION_DATA);
		return;
	}
	_sessionDataUnencryptedCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_sessionDataUnencryptedCharacteristic);
	_sessionDataUnencryptedCharacteristic->setUUID(UUID(getUUID(), SESSION_DATA_UNENCRYPTED_UUID));
	_sessionDataUnencryptedCharacteristic->setName(BLE_CHAR_SESSION_DATA);
	_sessionDataUnencryptedCharacteristic->setWritable(false);
//	_sessionDataUnencryptedCharacteristic->setNotifies(false);
	_sessionDataUnencryptedCharacteristic->setValue(_keySessionDataBuffer);
//	_sessionDataUnencryptedCharacteristic->setSharedEncryptionBuffer(true);
	_sessionDataUnencryptedCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_sessionDataUnencryptedCharacteristic->setMaxGattValueLength(sizeof(_keySessionDataBuffer));
	_sessionDataUnencryptedCharacteristic->setValueLength(0);
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

	// 2020-11-18 Bart: maybe we can remove this check now.
	assert(result.dataSize == 0 || result.buf.data == _resultPacketAccessor->getPayloadBuffer(), "Wrong buffer");

	writeResult(protocol, type, result.returnCode, cs_data_t(result.buf.data, result.dataSize));
}

void CrownstoneService::writeResult(uint8_t protocol, CommandHandlerTypes type, cs_ret_code_t retCode, cs_data_t data) {
	if (_resultCharacteristic == NULL) {
		return;
	}
	assert(_resultPacketAccessor != NULL, "_resultPacketAccessor is null");

	cs_ret_code_t retVal;
	if (data.len != 0 && data.data != _resultPacketAccessor->getPayloadBuffer()) {
		retVal = _resultPacketAccessor->setPayload(data.data, data.len);
	}
	else {
		// No need to copy data, it's already at the right buffer.
		retVal = _resultPacketAccessor->setPayloadSize(data.len);
	}
	if (!SUCCESS(retVal)) {
		LOGe("Unable to set result: %u", retVal);
		retCode = retVal;
		_resultPacketAccessor->setPayloadSize(0);
	}

	LOGd("Result: protocol=%u type=%u code=%u size=%u", protocol, type, retCode, data.len);
	_resultPacketAccessor->setProtocolVersion(protocol);
	_resultPacketAccessor->setType(type);
	_resultPacketAccessor->setResult(retCode);
	_resultCharacteristic->setValueLength(_resultPacketAccessor->getSerializedSize());
	_resultCharacteristic->updateValue();
}

void CrownstoneService::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_SESSION_DATA_SET: {
			// Check if this characteristic exists first. In case of setup mode it does not for instance.
			if (_sessionDataCharacteristic != NULL) {
				// This code assumes the session data that is pointed to, will remain valid!
				// In setup mode, this requires the setup key to be generated.
				_sessionDataCharacteristic->setValueLength(event.size);
				_sessionDataCharacteristic->setValue((uint8_t*)event.data);
				_sessionDataCharacteristic->updateValue(ConnectionEncryptionType::ECB);

				_sessionDataUnencryptedCharacteristic->setValueLength(event.size);
				_sessionDataUnencryptedCharacteristic->setValue(reinterpret_cast<uint8_t*>(event.data));
				_sessionDataUnencryptedCharacteristic->updateValue();
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
//			writeResult(CTRL_CMD_SETUP, cs_result_t(ERR_SUCCESS));
			break;
		}
		case CS_TYPE::EVT_MICROAPP_UPLOAD_RESULT: {
			TYPIFY(EVT_MICROAPP_UPLOAD_RESULT)* retCode = CS_TYPE_CAST(EVT_MICROAPP_UPLOAD_RESULT, event.data);
			cs_result_t result(*retCode);
			writeResult(CS_CONNECTION_PROTOCOL_VERSION, CTRL_CMD_MICROAPP_UPLOAD, result);
			break;
		}
		case CS_TYPE::EVT_MICROAPP_ERASE_RESULT: {
			TYPIFY(EVT_MICROAPP_ERASE_RESULT)* retCode = CS_TYPE_CAST(EVT_MICROAPP_ERASE_RESULT, event.data);
			cs_result_t result(*retCode);
			writeResult(CS_CONNECTION_PROTOCOL_VERSION, CTRL_CMD_MICROAPP_REMOVE, result);
			break;
		}
		case CS_TYPE::EVT_HUB_DATA_REPLY: {
			TYPIFY(EVT_HUB_DATA_REPLY)* reply = reinterpret_cast<TYPIFY(EVT_HUB_DATA_REPLY)*>(event.data);
			writeResult(CS_CONNECTION_PROTOCOL_VERSION, CTRL_CMD_HUB_DATA, reply->retCode, reply->data);
			break;
	}


	default: {}
	}
}
