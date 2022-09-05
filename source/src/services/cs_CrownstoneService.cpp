/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <events/cs_Event.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <protocol/cs_ErrorCodes.h>
#include <services/cs_CrownstoneService.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <structs/cs_PacketsInternal.h>

#define LogLevelCsServiceDebug SERIAL_VERY_VERBOSE

CrownstoneService::CrownstoneService() : EventListener() {
	EventDispatcher::getInstance().addListener(this);
	LOGi("Add crownstone service");

	setUUID(UUID(CROWNSTONE_UUID));
}

void CrownstoneService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT BLE_SERVICE_CROWNSTONE);

	cs_data_t writeBuf     = CharacteristicWriteBuffer::getInstance().getBuffer();
	_controlPacketAccessor = new ControlPacketAccessor<>();
	_controlPacketAccessor->assign(writeBuf.data, writeBuf.len);
	addControlCharacteristic(writeBuf.data, writeBuf.len, CONTROL_UUID, BASIC);

	cs_data_t readBuf     = CharacteristicReadBuffer::getInstance().getBuffer();
	_resultPacketAccessor = new ResultPacketAccessor<>();
	_resultPacketAccessor->assign(readBuf.data, readBuf.len);
	addResultCharacteristic(readBuf.data, readBuf.len, RESULT_UUID, BASIC);

	{
		LOGi(FMT_CHAR_ADD STR_CHAR_SESSION_DATA);
		cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
		addSessionDataCharacteristic(readBuf.data, readBuf.len);
	}

	LOGi(FMT_CHAR_ADD STR_CHAR_FACTORY_RESET);
	addFactoryResetCharacteristic();
}

void CrownstoneService::addControlCharacteristic(
		buffer_ptr_t buffer, cs_buffer_size_t size, uint16_t charUuid, EncryptionAccessLevel minimumAccessLevel) {
	if (_controlCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS STR_CHAR_CONTROL);
		return;
	}

	characteristic_config_t config = {
			.read                   = false,
			.write                  = true,
			.notify                 = false,
			.encrypted              = State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED),
			.sharedEncryptionBuffer = true,
			.minAccessLevel         = minimumAccessLevel,
	};

	_controlCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_controlCharacteristic);
	_controlCharacteristic->setName(BLE_CHAR_CONTROL);
	_controlCharacteristic->setUuid(charUuid);
	_controlCharacteristic->setConfig(config);
	_controlCharacteristic->setValueBuffer(buffer, size);
	_controlCharacteristic->setEventHandler(
			[&](CharacteristicEventType eventType,
				CharacteristicBase* characteristic,
				const EncryptionAccessLevel accessLevel) -> void {
				switch (eventType) {
					case CHARACTERISTIC_EVENT_WRITE: {

						// Encryption in the write stage verifies if the key is at the lowest level, command specific
						// permissions are handled in the CommandHandler.
						cs_result_t result;
						CommandHandlerTypes type = CTRL_CMD_UNKNOWN;
						uint8_t protocol         = CS_CONNECTION_PROTOCOL_VERSION;
						LOGd("controlCharacteristic onWrite buf=%p size=%u",
							 characteristic->getValue().data,
							 characteristic->getValue().len);
						_log(LogLevelCsServiceDebug, false, "data=");
						_logArray(
								LogLevelCsServiceDebug,
								true,
								characteristic->getValue().data,
								characteristic->getValue().len);

						protocol          = _controlPacketAccessor->getProtocolVersion();
						type              = (CommandHandlerTypes)_controlPacketAccessor->getType();
						cs_data_t payload = _controlPacketAccessor->getPayload();

						assert(_resultPacketAccessor != NULL, "_resultPacketAccessor is null");
						result.buf.data = _resultPacketAccessor->getPayloadBuffer();
						result.buf.len  = _resultPacketAccessor->getMaxPayloadSize();

						CommandHandler::getInstance().handleCommand(
								protocol,
								type,
								payload,
								cmd_source_with_counter_t(CS_CMD_SOURCE_CONNECTION),
								accessLevel,
								result);

						_log(LogLevelCsServiceDebug,
							 false,
							 "controlcharacteristic onWrite returnCode=0x%x dataSize=%u data=",
							 result.returnCode,
							 result.dataSize);
						_logArray(LogLevelCsServiceDebug, true, result.buf.data, result.dataSize);

						writeResult(protocol, type, result);
						break;
					}
					default: {
						break;
					}
				}
			});
}

void CrownstoneService::addResultCharacteristic(
		buffer_ptr_t buffer, cs_buffer_size_t size, uint16_t charUuid, EncryptionAccessLevel minimumAccessLevel) {
	if (_resultCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS STR_CHAR_RESULT);
		return;
	}

	characteristic_config_t config = {
			.read                   = true,
			.write                  = false,
			.notify                 = true,
			.autoNotify             = true,
			.notificationChunker    = true,
			.encrypted              = State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED),
			.sharedEncryptionBuffer = true,
			.minAccessLevel         = minimumAccessLevel,
	};

	_resultCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_resultCharacteristic);
	_resultCharacteristic->setName(STR_CHAR_RESULT);
	_resultCharacteristic->setUuid(charUuid);
	_resultCharacteristic->setConfig(config);
	_resultCharacteristic->setValueBuffer(buffer, size);
}

void CrownstoneService::addSessionDataCharacteristic(
		buffer_ptr_t buffer, cs_buffer_size_t size, EncryptionAccessLevel minimumAccessLevel) {
	if (_sessionDataCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS STR_CHAR_SESSION_DATA);
		return;
	}

	characteristic_config_t config = {
			.read                   = true,
			.write                  = false,
			.notify                 = true,
			.autoNotify             = true,
			.notificationChunker    = true,
			.encrypted              = State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED),
			.sharedEncryptionBuffer = true,
			.minAccessLevel         = minimumAccessLevel,
			.encryptionType         = ConnectionEncryptionType::ECB,
	};

	_sessionDataCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_sessionDataCharacteristic);
	_sessionDataCharacteristic->setName(BLE_CHAR_SESSION_DATA);
	_sessionDataCharacteristic->setUuid(SESSION_DATA_UUID);
	_sessionDataCharacteristic->setConfig(config);
	_sessionDataCharacteristic->setValueBuffer(buffer, size);

	if (_sessionDataUnencryptedCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS STR_CHAR_SESSION_DATA);
		return;
	}

	characteristic_config_t configUnencrypted = {
			.read                = true,
			.write               = false,
			.notify              = true,
			.autoNotify          = true,
			.notificationChunker = true,
			.encrypted           = false,
	};

	_sessionDataUnencryptedCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_sessionDataUnencryptedCharacteristic);
	_sessionDataUnencryptedCharacteristic->setName(BLE_CHAR_SESSION_DATA);
	_sessionDataUnencryptedCharacteristic->setUuid(SESSION_DATA_UNENCRYPTED_UUID);
	_sessionDataUnencryptedCharacteristic->setConfig(configUnencrypted);
	_sessionDataUnencryptedCharacteristic->setValueBuffer(_keySessionDataBuffer, sizeof(_keySessionDataBuffer));
}

void CrownstoneService::addFactoryResetCharacteristic() {
	characteristic_config_t config = {
			.read                = true,
			.write               = true,
			.notify              = true,
			.autoNotify          = true,
			.notificationChunker = false,
			.encrypted           = false,
	};

	_factoryResetCharacteristic = new Characteristic<uint32_t>();
	addCharacteristic(_factoryResetCharacteristic);
	_factoryResetCharacteristic->setName(BLE_CHAR_FACTORY_RESET);
	_factoryResetCharacteristic->setUuid(FACTORY_RESET_UUID);
	_factoryResetCharacteristic->setConfig(config);
	_factoryResetCharacteristic->setInitialValue(0);
	_factoryResetCharacteristic->setEventHandler(
			[&](CharacteristicEventType eventType,
				CharacteristicBase* characteristic,
				const EncryptionAccessLevel accessLevel) -> void {
				switch (eventType) {
					case CHARACTERISTIC_EVENT_WRITE: {
						uint32_t value = reinterpret_cast<Characteristic<uint32_t>*>(characteristic)->getValue();
						_log(LogLevelCsServiceDebug, true, "onWrite value=%u", value);

						// TODO if settings --> factory reset disabled, we set the value to 2 to indicate reset is not
						// possible.
						bool success         = FactoryReset::getInstance().recover(value);
						uint32_t resultValue = 0;
						if (success) {
							resultValue = 1;
						}
						_log(LogLevelCsServiceDebug, true, "  resultValue=%u", resultValue);
						reinterpret_cast<Characteristic<uint32_t>*>(characteristic)->setValue(resultValue);
						break;
					}
					default: {
						break;
					}
				}
			});
}

void CrownstoneService::writeResult(uint8_t protocol, CommandHandlerTypes type, cs_result_t& result) {

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

	_log(LogLevelCsServiceDebug, false, "Result buffer: size=%u data=", _resultPacketAccessor->getSerializedSize());
	_logArray(
			LogLevelCsServiceDebug,
			true,
			_resultPacketAccessor->getSerializedBuffer().data,
			_resultPacketAccessor->getSerializedBuffer().len);

	_resultCharacteristic->updateValue(_resultPacketAccessor->getSerializedSize());
}

void CrownstoneService::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_SESSION_DATA_SET: {
			if (_sessionDataCharacteristic != nullptr) {

				// For now we copy the session data.
				// We could avoid that if we get the session data buffer from ConnectionEncryption at init.
				// And as long as notifications don't influence the value buffer.
				memcpy(_sessionDataCharacteristic->getValue().data, event.data, event.size);
				_sessionDataCharacteristic->updateValue(event.size);

				memcpy(_sessionDataUnencryptedCharacteristic->getValue().data, event.data, event.size);
				_sessionDataUnencryptedCharacteristic->updateValue(event.size);
			}
			break;
		}
		case CS_TYPE::EVT_BLE_CONNECT: {
			if (_factoryResetCharacteristic != nullptr) {
				_factoryResetCharacteristic->setValue(0);
			}
			if (_resultCharacteristic != nullptr) {
				_resultCharacteristic->updateValue(0);
			}
			break;
		}
		case CS_TYPE::EVT_BLE_DISCONNECT: {
			if (_sessionDataCharacteristic != nullptr) {
				_sessionDataCharacteristic->updateValue(0);
			}
			break;
		}

		case CS_TYPE::CMD_SEND_ASYNC_RESULT_TO_BLE: {
			auto result = CS_TYPE_CAST(CMD_SEND_ASYNC_RESULT_TO_BLE, event.data);
			writeResult(CS_CONNECTION_PROTOCOL_VERSION, result->commandType, result->resultCode, result->resultData);
			break;
		}
		default: {
		}
	}
}
