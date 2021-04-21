/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 16, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_CrownstoneCentral.h>
#include <ble/cs_BleCentral.h>
#include <cfg/cs_UuidConfig.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_KeysAndAccess.h>

#include <structs/cs_ControlPacketAccessor.h>
#include <structs/buffer/cs_CharacteristicBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <structs/buffer/cs_EncryptionBuffer.h>

#define LOGCsCentralInfo LOGi
#define LOGCsCentralDebug LOGd

cs_ret_code_t CrownstoneCentral::init() {
	_uuids[0].fromFullUuid(CROWNSTONE_UUID);
	_uuids[1].fromFullUuid(SETUP_UUID);
	_uuids[2].fromShortUuid(BLE_UUID_DEVICE_INFORMATION_SERVICE);
	_uuids[3].fromShortUuid(0xFE59); // DFU service

	listen();

	return ERR_SUCCESS;
}

cs_ret_code_t CrownstoneCentral::connect(const device_address_t& address, uint16_t timeoutMs) {
	cs_ret_code_t retCode = BleCentral::getInstance().connect(address, timeoutMs);
	if (retCode == ERR_WAIT_FOR_SUCCESS) {
		_currentOperation = Operation::CONNECT;
		setStep(ConnectSteps::CONNECT);
	}
	return retCode;
}

cs_ret_code_t CrownstoneCentral::write(cs_control_cmd_t commandType, uint8_t* data, uint16_t size) {
	cs_ret_code_t retCode;

	cs_data_t encryptionBuffer;
	if (!EncryptionBuffer::getInstance().getBuffer(encryptionBuffer.data, encryptionBuffer.len)) {
		return ERR_BUFFER_UNASSIGNED;
	}

	cs_data_t writeBuf = CharacteristicWriteBuffer::getInstance().getBuffer();
	ControlPacketAccessor<> controlPacketAccessor;
	retCode = controlPacketAccessor.assign(writeBuf.data, writeBuf.len);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	controlPacketAccessor.setProtocolVersion(CS_CONNECTION_PROTOCOL_VERSION);
	controlPacketAccessor.setType(commandType);
	retCode = controlPacketAccessor.setPayload(data, size);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	cs_data_t controlPacket = controlPacketAccessor.getSerializedBuffer();

	uint16_t encryptedSize = ConnectionEncryption::getEncryptedBufferSize(controlPacket.len, ConnectionEncryptionType::CTR);
	if (_opMode == OperationMode::OPERATION_MODE_SETUP) {
		ConnectionEncryption::getInstance().encrypt(controlPacket, encryptionBuffer, SETUP, ConnectionEncryptionType::CTR);
	}
	else {
		ConnectionEncryption::getInstance().encrypt(controlPacket, encryptionBuffer, ADMIN, ConnectionEncryptionType::CTR);
	}

	retCode = BleCentral::getInstance().write(_controlHandle, encryptionBuffer.data, encryptedSize);
	if (retCode != ERR_WAIT_FOR_SUCCESS) {
		return retCode;
	}

	_currentOperation = Operation::WRITE;
	return ERR_WAIT_FOR_SUCCESS;
}




void CrownstoneCentral::setStep(ConnectSteps step) {
	_currentStep = static_cast<uint8_t>(step);
}

bool CrownstoneCentral::finalizeStep(ConnectSteps step, cs_ret_code_t retCode) {
	if (_currentOperation != Operation::CONNECT) {
		LOGCsCentralInfo("Wrong operation: _currentOperation=%u", _currentOperation);
		finalizeOperation(Operation::CONNECT, ERR_WRONG_OPERATION);
		return false;
	}
	return finalizeStep(static_cast<uint8_t>(step), retCode);
}

bool CrownstoneCentral::finalizeStep(uint8_t step, cs_ret_code_t retCode) {
	LOGCsCentralDebug("finalizeStep _currentOperation=%u _currentStep=%u step=%u retCode=%u", _currentOperation, _currentStep, step, retCode);
	if (retCode != ERR_SUCCESS) {
		LOGCsCentralInfo("No success: retCode=%u", retCode);
		finalizeOperation(_currentOperation, retCode);
		return false;
	}
	if (_currentStep != step) {
		LOGCsCentralInfo("Wrong step: _currentStep=%u step=%u", _currentStep, step);
		finalizeOperation(_currentOperation, ERR_WRONG_OPERATION);
		return false;
	}
	return true;
}




void CrownstoneCentral::finalizeOperation(Operation operation, cs_ret_code_t retCode) {
	switch (operation) {
		case Operation::NONE:
		case Operation::CONNECT: {
			finalizeOperation(operation, reinterpret_cast<uint8_t*>(&retCode), sizeof(retCode));
			break;
		}
		case Operation::WRITE: {
			cs_central_read_result_t result = {
					.retCode = retCode,
					.data = cs_data_t()
			};
			finalizeOperation(operation, reinterpret_cast<uint8_t*>(&result), sizeof(result));
			break;
		}
	}
}

void CrownstoneCentral::finalizeOperation(Operation operation, uint8_t* data, uint8_t dataSize) {
	LOGCsCentralDebug("finalizeOperation operation=%u _currentOperation=%u dataSize=%u", operation, _currentOperation, dataSize);
	event_t event(CS_TYPE::CONFIG_DO_NOT_USE);

	// Handle error first.
	if (_currentOperation != operation) {
		switch (_currentOperation) {
			case Operation::NONE: {
				break;
			}
			case Operation::CONNECT: {
				// Ignore data, finalize with error instead.
				TYPIFY(EVT_CS_CENTRAL_CONNECT_RESULT) result = ERR_WRONG_STATE;
				event_t errEvent(CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT, reinterpret_cast<uint8_t*>(&result), sizeof(result));
				sendOperationResult(errEvent);
				return;
			}
			case Operation::WRITE: {
				// Ignore data, finalize with error instead.
				TYPIFY(EVT_CS_CENTRAL_WRITE_RESULT) result = {
						.retCode = ERR_WRONG_STATE,
						.data = cs_data_t()
				};
				event_t errEvent(CS_TYPE::EVT_CS_CENTRAL_WRITE_RESULT, reinterpret_cast<uint8_t*>(&result), sizeof(result));
				sendOperationResult(errEvent);
				return;
			}
		}
	}

	event.data = data;
	event.size = dataSize;
	switch (_currentOperation) {
		case Operation::NONE: {
			LOGCsCentralDebug("No operation was in progress");
			break;
		}
		case Operation::CONNECT: {
			event.type = CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT;
			break;
		}
		case Operation::WRITE: {
			event.type = CS_TYPE::EVT_CS_CENTRAL_WRITE_RESULT;
			break;
		}
	}
	sendOperationResult(event);
}

void CrownstoneCentral::sendOperationResult(event_t& event) {
	LOGCsCentralDebug("sendOperationResult type=%u size=%u", event.type, event.size);
	// Set current operation before dispatching the event, so that a new command can be issued on event.
	_currentOperation = Operation::NONE;

	if (event.type != CS_TYPE::CONFIG_DO_NOT_USE) {
		event.dispatch();
	}
}







void CrownstoneCentral::onConnect(cs_ret_code_t retCode) {
	if (!finalizeStep(ConnectSteps::CONNECT, retCode)) {
		return;
	}

	retCode = BleCentral::getInstance().discoverServices(_uuids, sizeof(_uuids) / sizeof(_uuids[0]));
	if (retCode != ERR_WAIT_FOR_SUCCESS) {
		finalizeOperation(Operation::CONNECT, retCode);
		return;
	}
	setStep(ConnectSteps::DISCOVER);
	_sessionDataHandle = BLE_GATT_HANDLE_INVALID;
	_controlHandle = BLE_GATT_HANDLE_INVALID;
	_resultHandle = BLE_GATT_HANDLE_INVALID;
}

void CrownstoneCentral::onDiscovery(ble_central_discovery_t& result) {

//	// Quicker / shorter method?
//	ble_uuid_t csUuid = _uuids[0].getUuid();
//	ble_uuid_t uuid = result.uuid.getUuid();
//
//	if (uuid.type == csUuid.type && uuid.uuid == CONTROL_UUID) {
//		_controlHandle = result.valueHandle;
//		LOGi("Found control handle: %u", _controlHandle);
//	}

	UUID uuid;

	// Normal mode
	if (result.uuid == _uuids[0]) {
		_opMode = OperationMode::OPERATION_MODE_NORMAL;
		LOGCsCentralInfo("Crownstone service found");
	}
	uuid.fromBaseUuid(_uuids[0], SESSION_DATA_UNENCRYPTED_UUID);
	if (result.uuid == uuid) {
		_sessionDataHandle = result.valueHandle;
		LOGCsCentralDebug("Found session data handle: %u", _sessionDataHandle);
	}
	uuid.fromBaseUuid(_uuids[0], CONTROL_UUID);
	if (result.uuid == uuid) {
		_controlHandle = result.valueHandle;
		LOGCsCentralDebug("Found control handle: %u", _controlHandle);
	}
	uuid.fromBaseUuid(_uuids[0], RESULT_UUID);
	if (result.uuid == uuid) {
		_resultHandle = result.valueHandle;
		LOGCsCentralDebug("Found result handle: %u", _resultHandle);
	}

	// Setup mode
	if (result.uuid == _uuids[1]) {
		_opMode = OperationMode::OPERATION_MODE_SETUP;
		LOGCsCentralInfo("Setup service found");
	}
	uuid.fromBaseUuid(_uuids[1], SETUP_KEY_UUID);
	if (result.uuid == uuid) {
		_sessionKeyHandle = result.valueHandle;
		LOGCsCentralDebug("Found session key handle: %u", _sessionKeyHandle);
	}
	uuid.fromBaseUuid(_uuids[1], SESSION_DATA_UNENCRYPTED_UUID);
	if (result.uuid == uuid) {
		_sessionDataHandle = result.valueHandle;
		LOGCsCentralDebug("Found session data handle: %u", _sessionDataHandle);
	}
	uuid.fromBaseUuid(_uuids[1], SETUP_CONTROL_UUID);
	if (result.uuid == uuid) {
		_controlHandle = result.valueHandle;
		LOGCsCentralDebug("Found control handle: %u", _controlHandle);
	}
	uuid.fromBaseUuid(_uuids[1], SETUP_RESULT_UUID);
	if (result.uuid == uuid) {
		_resultHandle = result.valueHandle;
		LOGCsCentralDebug("Found result handle: %u", _resultHandle);
	}

}

void CrownstoneCentral::onDiscoveryDone(cs_ret_code_t retCode) {
	if (!finalizeStep(ConnectSteps::DISCOVER, retCode)) {
		return;
	}

	switch (_opMode) {
		case OperationMode::OPERATION_MODE_NORMAL: {
			// Read session data.
			retCode = BleCentral::getInstance().read(_sessionDataHandle);
			if (retCode != ERR_WAIT_FOR_SUCCESS) {
				finalizeOperation(Operation::CONNECT, retCode);
				return;
			}
			setStep(ConnectSteps::SESSION_DATA);
			break;
		}
		case OperationMode::OPERATION_MODE_SETUP: {
			// Read session key.
			retCode = BleCentral::getInstance().read(_sessionKeyHandle);
			if (retCode != ERR_WAIT_FOR_SUCCESS) {
				finalizeOperation(Operation::CONNECT, retCode);
				return;
			}
			setStep(ConnectSteps::SESSION_KEY);
			break;
		}
		default: {
			// TODO: event
			return;
		}
	}
}

void CrownstoneCentral::onRead(ble_central_read_result_t& result) {
	cs_ret_code_t retCode = result.retCode;
	if (retCode != ERR_SUCCESS) {
		finalizeOperation(_currentOperation, retCode);
		return;
	}

	switch (_currentOperation) {
		case Operation::CONNECT: {
			ConnectSteps currentStep = static_cast<ConnectSteps>(_currentStep);
			switch (currentStep) {
				case ConnectSteps::SESSION_DATA: {
					if (!finalizeStep(_currentStep, retCode)) {
						return;
					}
					// Parse data that was read.
					if (result.data.len != sizeof(session_data_t)) {
						finalizeOperation(_currentOperation, ERR_WRONG_PAYLOAD_LENGTH);
						return;
					}
					// Store the session data.
					retCode = ConnectionEncryption::getInstance().setSessionData(*reinterpret_cast<session_data_t*>(result.data.data));
					if (retCode != ERR_SUCCESS) {
						finalizeOperation(_currentOperation, retCode);
						return;
					}
					// Done.
					finalizeOperation(_currentOperation, retCode);
					break;
				}
				case ConnectSteps::SESSION_KEY: {
					if (!finalizeStep(_currentStep, retCode)) {
						return;
					}
					// Parse and store the session key.
					retCode = KeysAndAccess::getInstance().setSetupKey(result.data);
					if (retCode != ERR_SUCCESS) {
						finalizeOperation(_currentOperation, retCode);
						return;
					}
					// Read session data.
					retCode = BleCentral::getInstance().read(_sessionDataHandle);
					if (retCode != ERR_WAIT_FOR_SUCCESS) {
						finalizeOperation(_currentOperation, retCode);
						return;
					}
					setStep(ConnectSteps::SESSION_DATA);
					return;
				}
				default:
					finalizeOperation(_currentOperation, ERR_WRONG_OPERATION);
					return;
			}
			break;
		}
		default: {
			finalizeOperation(_currentOperation, ERR_WRONG_OPERATION);
			return;
		}
	}

}

void CrownstoneCentral::onWrite(cs_ret_code_t retCode) {
	if (retCode != ERR_SUCCESS) {
		finalizeOperation(_currentOperation, retCode);
		return;
	}
	switch (_currentOperation) {
		case Operation::WRITE: {
			// Done.
			finalizeOperation(_currentOperation, retCode);
			break;
		}
		default: {
			finalizeOperation(_currentOperation, ERR_WRONG_OPERATION);
			return;
		}
	}
}



void CrownstoneCentral::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::CMD_CS_CENTRAL_CONNECT: {
			auto packet = CS_TYPE_CAST(CMD_CS_CENTRAL_CONNECT, event.data);
			event.result.returnCode = connect(packet->address, packet->timeoutMs);
			break;
		}
		case CS_TYPE::CMD_CS_CENTRAL_WRITE: {
			auto packet = CS_TYPE_CAST(CMD_CS_CENTRAL_WRITE, event.data);
			event.result.returnCode = write(packet->commandType, packet->data.data, packet->data.len);
			break;
		}

		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);
			onConnect(*result);
			break;
		}

		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_DISCOVERY, event.data);
			onDiscovery(*result);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_DISCOVERY_RESULT, event.data);
			onDiscoveryDone(*result);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_READ_RESULT: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_READ_RESULT, event.data);
			onRead(*result);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_WRITE_RESULT, event.data);
			onWrite(*result);
			break;
		}
		default: {
			break;
		}
	}
}

