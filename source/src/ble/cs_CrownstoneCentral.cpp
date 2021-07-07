/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 16, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_BleCentral.h>
#include <ble/cs_CrownstoneCentral.h>
#include <ble/cs_UUID.h>
#include <cfg/cs_UuidConfig.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_KeysAndAccess.h>
#include <structs/buffer/cs_CharacteristicBuffer.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <structs/buffer/cs_EncryptionBuffer.h>
#include <structs/cs_ControlPacketAccessor.h>
#include <structs/cs_ResultPacketAccessor.h>
#include <util/cs_Utils.h>

#define LOGCsCentralInfo LOGi
#define LOGCsCentralDebug LOGvv
#define LogLevelCsCentralDebug SERIAL_VERY_VERBOSE

cs_ret_code_t CrownstoneCentral::init() {
	// Make use of the fact ERR_SUCCESS = 0, to avoid many if statements.
	cs_ret_code_t retCode = ERR_SUCCESS;
	retCode |= _serviceUuids[ServiceIndex::SERVICE_INDEX_CROWNSTONE].fromFullUuid(CROWNSTONE_UUID);
	retCode |= _serviceUuids[ServiceIndex::SERVICE_INDEX_SETUP].fromFullUuid(SETUP_UUID);
	retCode |= _serviceUuids[ServiceIndex::SERVICE_INDEX_DEVICE_INFO].fromShortUuid(BLE_UUID_DEVICE_INFORMATION_SERVICE);
	retCode |= _serviceUuids[ServiceIndex::SERVICE_INDEX_DFU].fromShortUuid(0xFE59); // DFU service
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	reset();
	listen();
	return ERR_SUCCESS;
}

void CrownstoneCentral::reset() {
	_sessionKeyHandle =  BLE_GATT_HANDLE_INVALID;
	_sessionDataHandle = BLE_GATT_HANDLE_INVALID;
	_controlHandle =     BLE_GATT_HANDLE_INVALID;
	_resultHandle =      BLE_GATT_HANDLE_INVALID;
	_resultCccdHandle =  BLE_GATT_HANDLE_INVALID;
	_opMode =            OperationMode::OPERATION_MODE_UNINITIALIZED;
	_stoneId =           0;
	stopTimeoutTimer();
	resetNotifactionMergerState();
}

void CrownstoneCentral::resetNotifactionMergerState() {
	_notificationNextIndex = 0;
	_notificationMergedDataSize = 0;
}

cs_ret_code_t CrownstoneCentral::connect(stone_id_t stoneId, uint16_t timeoutMs) {
	if (isBusy()) {
		LOGCsCentralInfo("Busy");
		return ERR_BUSY;
	}

	TYPIFY(CMD_MESH_TOPO_GET_MAC) eventData = stoneId;
	event_t event(CS_TYPE::CMD_MESH_TOPO_GET_MAC, &eventData, sizeof(eventData));
	event.dispatch();
	cs_ret_code_t retCode = event.result.returnCode;
	switch (retCode) {
		case ERR_SUCCESS: {
			// If address would be available immediately, we don't have to wait for it and can connect instead.
			LOGw("Not implemented");
			return ERR_NOT_IMPLEMENTED;
		}
		case ERR_WAIT_FOR_SUCCESS: {
			// Start with a clean state.
			reset();
			_stoneId = stoneId;
			_timeoutMs = timeoutMs;
			_currentOperation = Operation::CONNECT;
			setStep(ConnectSteps::GET_ADDRESS);

			startTimeoutTimer(timeoutMs);
			return ERR_WAIT_FOR_SUCCESS;
		}
		default: {
			return retCode;
		}
	}
}

cs_ret_code_t CrownstoneCentral::connect(const device_address_t& address, uint16_t timeoutMs) {
	if (isBusy()) {
		LOGCsCentralInfo("Busy");
		return ERR_BUSY;
	}

	cs_ret_code_t retCode = BleCentral::getInstance().connect(address, timeoutMs);
	if (retCode == ERR_WAIT_FOR_SUCCESS) {
		// Start with a clean state.
		reset();
		_timeoutMs = timeoutMs;
		_currentOperation = Operation::CONNECT;
		setStep(ConnectSteps::CONNECT);
	}
	return retCode;
}

cs_ret_code_t CrownstoneCentral::disconnect() {
	if (isBusy()) {
		LOGCsCentralInfo("Cancel current operation");
		finalizeOperation(_currentOperation, ERR_CANCELED);
	}
	// No need to set current operation: BleCentral will block any other operations for us.
	// Also, the user waits for the BleCentral event, and might get it before we get it.
	// In which case the user wouldn't be able to connect() again, because we wouldn't have finalized the disconnect operation yet.
	return BleCentral::getInstance().disconnect();
}

cs_ret_code_t CrownstoneCentral::write(cs_control_cmd_t commandType, uint8_t* data, uint16_t size) {
	if (isBusy()) {
		LOGCsCentralInfo("Busy");
		return ERR_BUSY;
	}

	cs_ret_code_t retCode;

	cs_data_t encryptedBuffer = BleCentral::getInstance().requestWriteBuffer();
	if (encryptedBuffer.data == nullptr) {
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
		ConnectionEncryption::getInstance().encrypt(controlPacket, encryptedBuffer, SETUP, ConnectionEncryptionType::CTR);
	}
	else {
		ConnectionEncryption::getInstance().encrypt(controlPacket, encryptedBuffer, ADMIN, ConnectionEncryptionType::CTR);
	}

	retCode = BleCentral::getInstance().write(_controlHandle, encryptedBuffer.data, encryptedSize);
	if (retCode != ERR_WAIT_FOR_SUCCESS) {
		return retCode;
	}

	_currentOperation = Operation::WRITE;
	setStep(WriteControlSteps::WRITE);
	return ERR_WAIT_FOR_SUCCESS;
}

cs_data_t CrownstoneCentral::requestWriteBuffer() {
	if (isBusy()) {
		return cs_data_t();
	}
	cs_data_t writeBuf = CharacteristicWriteBuffer::getInstance().getBuffer();
	ControlPacketAccessor<> controlPacketAccessor;
	cs_ret_code_t retCode = controlPacketAccessor.assign(writeBuf.data, writeBuf.len);
	if (retCode != ERR_SUCCESS) {
		return cs_data_t();
	}
	uint16_t maxWriteSize = 224; // 256 - some bytes for encryption overhead..
	if (writeBuf.len > maxWriteSize) {
		return cs_data_t(controlPacketAccessor.getPayload().data, controlPacketAccessor.getMaxPayloadSize() - (writeBuf.len - maxWriteSize));
	}
	return cs_data_t(controlPacketAccessor.getPayload().data, controlPacketAccessor.getMaxPayloadSize());
}

//////////////////// Helpers ////////////////////

void CrownstoneCentral::readSessionData() {
	cs_ret_code_t retCode;
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
			finalizeOperation(Operation::CONNECT, ERR_WRONG_MODE);
			return;
		}
	}
}


//////////////////// Keeping up operation and step ////////////////////

bool CrownstoneCentral::isBusy() {
	return _currentOperation != Operation::NONE;
}

void CrownstoneCentral::startTimeoutTimer(uint16_t timeoutMs) {
	_timeoutCountDown = timeoutMs / TICK_INTERVAL_MS;
}

void CrownstoneCentral::stopTimeoutTimer() {
	_timeoutCountDown = 0;
}

void CrownstoneCentral::setStep(ConnectSteps step) {
	setStep(static_cast<uint8_t>(step));
}

void CrownstoneCentral::setStep(WriteControlSteps step) {
	setStep(static_cast<uint8_t>(step));
}

void CrownstoneCentral::setStep(uint8_t step) {
	if (step != _currentStep + 1) {
		LOGCsCentralDebug("Skipping steps: going from %u to %u", _currentStep, step);
	}
	_currentStep = step;
}

bool CrownstoneCentral::finalizeStep(ConnectSteps step, cs_ret_code_t retCode) {
	if (_currentOperation != Operation::CONNECT) {
		LOGCsCentralInfo("Wrong operation: _currentOperation=%u", _currentOperation);
		finalizeOperation(Operation::CONNECT, ERR_WRONG_OPERATION);
		return false;
	}
	return finalizeStep(static_cast<uint8_t>(step), retCode);
}

bool CrownstoneCentral::finalizeStep(WriteControlSteps step, cs_ret_code_t retCode) {
	if (_currentOperation != Operation::WRITE) {
		LOGCsCentralInfo("Wrong operation: _currentOperation=%u", _currentOperation);
		finalizeOperation(Operation::WRITE, ERR_WRONG_OPERATION);
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
			cs_central_write_result_t result = {
					.writeRetCode = retCode,
					.result = ResultPacketAccessor<>()
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
						.writeRetCode = ERR_WRONG_STATE,
						.result = ResultPacketAccessor<>()
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
	_currentStep = 0;

	if (event.type != CS_TYPE::CONFIG_DO_NOT_USE) {
		event.dispatch();
	}
}

//////////////////// Handlers ////////////////////

void CrownstoneCentral::onTimeout() {
	LOGCsCentralInfo("onTimeout");
	finalizeOperation(_currentOperation, ERR_TIMEOUT);
}

void CrownstoneCentral::onMacAddress(mesh_topo_mac_result_t& result) {
	if (!finalizeStep(ConnectSteps::GET_ADDRESS, ERR_SUCCESS)) {
		return;
	}
	if (result.stoneId != _stoneId) {
		LOGw("Wrong stone ID: received=%u expected=%u", result.stoneId, _stoneId);
		return;
	}
	stopTimeoutTimer();

	device_address_t address;
	memcpy(address.address, result.macAddress, sizeof(address.address));

	cs_ret_code_t retCode = BleCentral::getInstance().connect(address, _timeoutMs);
	if (retCode != ERR_WAIT_FOR_SUCCESS) {
		finalizeOperation(Operation::CONNECT, retCode);
		return;
	}
	_currentOperation = Operation::CONNECT;
	setStep(ConnectSteps::CONNECT);
}

void CrownstoneCentral::onConnect(cs_ret_code_t retCode) {
	if (!finalizeStep(ConnectSteps::CONNECT, retCode)) {
		return;
	}

	retCode = BleCentral::getInstance().discoverServices(_serviceUuids, BLEutil::ArraySize(_serviceUuids));
	if (retCode != ERR_WAIT_FOR_SUCCESS) {
		finalizeOperation(Operation::CONNECT, retCode);
		return;
	}
	setStep(ConnectSteps::DISCOVER);
	_sessionDataHandle = BLE_GATT_HANDLE_INVALID;
	_controlHandle = BLE_GATT_HANDLE_INVALID;
	_resultCccdHandle = BLE_GATT_HANDLE_INVALID;
}

void CrownstoneCentral::onDisconnect() {
	reset();
	finalizeOperation(_currentOperation, ERR_CANCELED);
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
	if (result.uuid == _serviceUuids[ServiceIndex::SERVICE_INDEX_CROWNSTONE]) {
		_opMode = OperationMode::OPERATION_MODE_NORMAL;
		LOGCsCentralInfo("Crownstone service found");
	}
	uuid.fromBaseUuid(_serviceUuids[ServiceIndex::SERVICE_INDEX_CROWNSTONE], SESSION_DATA_UNENCRYPTED_UUID);
	if (result.uuid == uuid) {
		_sessionDataHandle = result.valueHandle;
		LOGCsCentralDebug("Found session data handle: %u", _sessionDataHandle);
	}
	uuid.fromBaseUuid(_serviceUuids[ServiceIndex::SERVICE_INDEX_CROWNSTONE], CONTROL_UUID);
	if (result.uuid == uuid) {
		_controlHandle = result.valueHandle;
		LOGCsCentralDebug("Found control handle: %u", _controlHandle);
	}
	uuid.fromBaseUuid(_serviceUuids[ServiceIndex::SERVICE_INDEX_CROWNSTONE], RESULT_UUID);
	if (result.uuid == uuid) {
		_resultHandle = result.valueHandle;
		_resultCccdHandle = result.cccdHandle;
		LOGCsCentralDebug("Found result handle: %u", _resultHandle);
	}

	// Setup mode
	if (result.uuid == _serviceUuids[ServiceIndex::SERVICE_INDEX_SETUP]) {
		_opMode = OperationMode::OPERATION_MODE_SETUP;
		LOGCsCentralInfo("Setup service found");
	}
	uuid.fromBaseUuid(_serviceUuids[ServiceIndex::SERVICE_INDEX_SETUP], SETUP_KEY_UUID);
	if (result.uuid == uuid) {
		_sessionKeyHandle = result.valueHandle;
		LOGCsCentralDebug("Found session key handle: %u", _sessionKeyHandle);
	}
	uuid.fromBaseUuid(_serviceUuids[ServiceIndex::SERVICE_INDEX_SETUP], SESSION_DATA_UNENCRYPTED_UUID);
	if (result.uuid == uuid) {
		_sessionDataHandle = result.valueHandle;
		LOGCsCentralDebug("Found session data handle: %u", _sessionDataHandle);
	}
	uuid.fromBaseUuid(_serviceUuids[ServiceIndex::SERVICE_INDEX_SETUP], SETUP_CONTROL_UUID);
	if (result.uuid == uuid) {
		_controlHandle = result.valueHandle;
		LOGCsCentralDebug("Found control handle: %u", _controlHandle);
	}
	uuid.fromBaseUuid(_serviceUuids[ServiceIndex::SERVICE_INDEX_SETUP], SETUP_RESULT_UUID);
	if (result.uuid == uuid) {
		_resultHandle = result.valueHandle;
		_resultCccdHandle = result.cccdHandle;
		LOGCsCentralDebug("Found result handle: %u", _resultHandle);
	}

}

void CrownstoneCentral::onDiscoveryDone(cs_ret_code_t retCode) {
	if (!finalizeStep(ConnectSteps::DISCOVER, retCode)) {
		return;
	}
	retCode = BleCentral::getInstance().writeNotificationConfig(_resultCccdHandle, true);
	if (retCode != ERR_WAIT_FOR_SUCCESS) {
		finalizeOperation(Operation::CONNECT, retCode);
		return;
	}
	setStep(ConnectSteps::ENABLE_NOTIFICATIONS);
}

void CrownstoneCentral::onRead(ble_central_read_result_t& result) {
	cs_ret_code_t retCode = result.retCode;
	if (retCode != ERR_SUCCESS) {
		finalizeOperation(_currentOperation, retCode);
		return;
	}

	switch (_currentOperation) {
		case Operation::CONNECT: {
			onReadDuringConnect(result);
			break;
		}
		default: {
			finalizeOperation(_currentOperation, ERR_WRONG_OPERATION);
			return;
		}
	}
}

void CrownstoneCentral::onReadDuringConnect(ble_central_read_result_t& result) {
	// The retCode has already been checked to be ERR_SUCCESS.
	cs_ret_code_t retCode = result.retCode;

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
}

void CrownstoneCentral::onWrite(cs_ret_code_t retCode) {
	if (retCode != ERR_SUCCESS) {
		finalizeOperation(_currentOperation, retCode);
		return;
	}
	switch (_currentOperation) {
		case Operation::CONNECT: {
			if (!finalizeStep(ConnectSteps::ENABLE_NOTIFICATIONS, retCode)) {
				return;
			}
			readSessionData();
			break;
		}
		case Operation::WRITE: {
			if (!finalizeStep(WriteControlSteps::WRITE, retCode)) {
				return;
			}
			// Wait for notifications.
			setStep(WriteControlSteps::RECEIVE_RESULT);
			break;
		}
		default: {
			finalizeOperation(_currentOperation, ERR_WRONG_OPERATION);
			return;
		}
	}
}

void CrownstoneCentral::onNotification(ble_central_notification_t& result) {
	if (result.handle != _resultHandle) {
		LOGw("Unexpected handle=%u", result.handle);
		return;
	}

	// In theory, we can receive notifications at any moment.
	// So only handle them when we expect them, and ignore them otherwise.
	if (_currentOperation != Operation::WRITE || static_cast<WriteControlSteps>(_currentStep) != WriteControlSteps::RECEIVE_RESULT) {
		return;
	}

	cs_data_t resultData;
	cs_ret_code_t retCode = mergeNotification(result.data, resultData);
	switch (retCode) {
		case ERR_SUCCESS: {
			if (resultData.data == nullptr) {
				finalizeOperation(_currentOperation, ERR_BUFFER_UNASSIGNED);
				return;
			}
			// Check the result data.
			ResultPacketAccessor<> resultPacketAccessor;
			retCode = resultPacketAccessor.assign(resultData.data, resultData.len);
			if (retCode != ERR_SUCCESS) {
				finalizeOperation(_currentOperation, retCode);
				return;
			}
			_log(LogLevelCsCentralDebug, false, "Result: protocol=%u type=%u result=%u data=",
					resultPacketAccessor.getProtocolVersion(),
					resultPacketAccessor.getType(),
					resultPacketAccessor.getResult());
			_logArray(LogLevelCsCentralDebug, true, resultPacketAccessor.getPayload().data, resultPacketAccessor.getPayload().len);

			cs_central_write_result_t result = {
					.writeRetCode = ERR_SUCCESS,
					.result = resultPacketAccessor
			};
			if (resultPacketAccessor.getResult() == ERR_WAIT_FOR_SUCCESS) {
				// Send event, but keep waiting for the next result.
				// No need to check the current operation or step, that has been earlier in this function.
				event_t resultEvent(CS_TYPE::EVT_CS_CENTRAL_WRITE_RESULT, reinterpret_cast<uint8_t*>(&result), sizeof(result));
				resultEvent.dispatch();
				return;
			}
			// Done.
			finalizeOperation(_currentOperation, reinterpret_cast<uint8_t*>(&result), sizeof(result));
			break;
		}
		case ERR_WAIT_FOR_SUCCESS: {
			// Keep waiting for more notifications.
			break;
		}
		default: {
			finalizeOperation(_currentOperation, retCode);
			break;
		}
	}
}

cs_ret_code_t CrownstoneCentral::mergeNotification(const cs_const_data_t& data, cs_data_t& resultData) {
	// Check data size.
	const uint16_t headerSize = 1;
	if (data.len < (headerSize + 1)) {
		LOGw("Invalid length=%u", data.len);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	const uint16_t dataSize = data.len - headerSize;

	// Check index.
	uint8_t index = data.data[0];
	if (index != _notificationNextIndex && index != 255) {
		LOGw("Expected index=%u, received index=%u", _notificationNextIndex, index);
		return ERR_WRONG_PARAMETER;
	}

	// Get the encryption buffer.
	cs_data_t encryptedBuffer;
	if (!EncryptionBuffer::getInstance().getBuffer(encryptedBuffer.data, encryptedBuffer.len)) {
		return ERR_BUFFER_UNASSIGNED;
	}

	if (encryptedBuffer.len < _notificationMergedDataSize + dataSize) {
		return ERR_BUFFER_TOO_SMALL;
	}

	// Copy data to encryption buffer.
	memcpy(encryptedBuffer.data + _notificationMergedDataSize, data.data + headerSize, dataSize);
	_notificationNextIndex++;
	_notificationMergedDataSize += dataSize;

	if (index == CS_CHARACTERISTIC_NOTIFICATION_PART_LAST) {
		// Last index.
		cs_data_t inputBuf(encryptedBuffer.data, _notificationMergedDataSize);
		_log(LogLevelCsCentralDebug, false, "Merged notification data=");
		_logArray(LogLevelCsCentralDebug, true, inputBuf.data, inputBuf.len);

		// Decrypt merged data to read buffer.
		cs_data_t readBuf = CharacteristicReadBuffer::getInstance().getBuffer();
		EncryptionAccessLevel accessLevel;
		cs_ret_code_t retCode = ConnectionEncryption::getInstance().decrypt(inputBuf, readBuf, accessLevel, ConnectionEncryptionType::CTR);
		if (retCode != ERR_SUCCESS) {
			return retCode;
		}
		resetNotifactionMergerState();
		resultData = readBuf;
		return ERR_SUCCESS;
	}
	return ERR_WAIT_FOR_SUCCESS;
}

void CrownstoneCentral::handleEvent(event_t& event) {
	switch (event.type) {
		// Commands
		case CS_TYPE::CMD_CS_CENTRAL_CONNECT: {
			auto packet = CS_TYPE_CAST(CMD_CS_CENTRAL_CONNECT, event.data);
			if (packet->stoneId != 0) {
				event.result.returnCode = connect(packet->stoneId, packet->timeoutMs);
			}
			else {
				event.result.returnCode = connect(packet->address, packet->timeoutMs);
			}
			break;
		}
		case CS_TYPE::CMD_CS_CENTRAL_DISCONNECT: {
			event.result.returnCode = disconnect();
			break;
		}
		case CS_TYPE::CMD_CS_CENTRAL_WRITE: {
			auto packet = CS_TYPE_CAST(CMD_CS_CENTRAL_WRITE, event.data);
			event.result.returnCode = write(packet->commandType, packet->data.data, packet->data.len);
			break;
		}
		case CS_TYPE::CMD_CS_CENTRAL_GET_WRITE_BUF: {
			event.result.buf = requestWriteBuffer();
			break;
		}

		// Events
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);
			onConnect(*result);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED: {
			onDisconnect();
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
		case CS_TYPE::EVT_BLE_CENTRAL_NOTIFICATION: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_NOTIFICATION, event.data);
			onNotification(*result);
			break;
		}
		case CS_TYPE::EVT_MESH_TOPO_MAC_RESULT: {
			auto result = CS_TYPE_CAST(EVT_MESH_TOPO_MAC_RESULT, event.data);
			onMacAddress(*result);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			if (_timeoutCountDown != 0) {
				if (--_timeoutCountDown == 0) {
					onTimeout();
				}
			}
			break;
		}
		default: {
			break;
		}
	}
}

