/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <dfu/cs_MeshDfuTransport.h>
#include <dfu/cs_MeshDfuConstants.h>
#include <util/cs_Utils.h>


#define LOGMeshDfuTransportDebug LOGd
#define LOGMeshDfuTransportInfo LOGi
#define LOGMeshDfuTransportWarn LOGw

cs_ret_code_t MeshDfuTransport::init() {
	if (!_firstInit) {
		LOGMeshDfuTransportDebug("mesh dfu transport already initialized");
		return ERR_SUCCESS;
	}

	_bleCentral = &BleCentral::getInstance();
	if(_bleCentral == nullptr){
		LOGMeshDfuTransportDebug("could't find ble central");
		return ERR_NOT_AVAILABLE;
	}

	// Make use of the fact ERR_SUCCESS = 0, to avoid many if statements.
	cs_ret_code_t retCode = ERR_SUCCESS;
	retCode |= _dfuServiceUuid.fromShortUuid(MeshDfuConstants::DFUAdapter::dfuServiceShortUuid);

	retCode |= _uuids[Index::ControlPoint].fromFullUuid(MeshDfuConstants::DFUAdapter::controlPointString);
	retCode |= _uuids[Index::DataPoint].fromFullUuid(MeshDfuConstants::DFUAdapter::dataPointString);

	clearConnectionData();

	if (retCode != ERR_SUCCESS) {
		LOGMeshDfuTransportDebug("failed to register required uuids");
		return retCode;
	}

	listen();

	LOGMeshDfuTransportDebug("MeshDfuTransport init successful");

	_firstInit = false;
	return ERR_SUCCESS;
}

cs_ret_code_t MeshDfuTransport::enableNotifications(bool on) {
	LOGMeshDfuTransportDebug("MeshDfuTransport: activate notifications");
	return _bleCentral->writeNotificationConfig(_cccdHandles[Index::ControlPoint], true);
}

bool MeshDfuTransport::isTargetInDfuMode() {
	[[maybe_unused]] bool characteristicsFound = _uuidHandles[Index::ControlPoint] != BLE_GATT_HANDLE_INVALID
			   && _uuidHandles[Index::DataPoint] != BLE_GATT_HANDLE_INVALID;

	LOGMeshDfuTransportDebug("charackteristicsFound==%u, _dfuServiceFound==%u",
			characteristicsFound, _dfuServiceFound);

	return _dfuServiceFound && characteristicsFound;
}

UUID* MeshDfuTransport::getServiceUuids(){
	return &_dfuServiceUuid;
}

uint8_t MeshDfuTransport::getServiceUuidCount() {
	return 1;
}

void MeshDfuTransport::clearConnectionData() {
	_uuidHandles[Index::ControlPoint] = BLE_GATT_HANDLE_INVALID;
	_uuidHandles[Index::DataPoint]    = BLE_GATT_HANDLE_INVALID;
	_dfuServiceFound = false;
	_discoveryComplete = false;
}

bool MeshDfuTransport::isBusy() {
	return _onExpectedEvent != nullptr || _onTimeout != nullptr;
}




void MeshDfuTransport::prepare() {
	_setPrn();
}

void MeshDfuTransport::dispatchResult(cs_result_t res) {
	event_t eventOut(CS_TYPE::EVT_MESH_DFU_TRANSPORT_RESULT, &res, sizeof(res));
	eventOut.dispatch();
}
void MeshDfuTransport::dispatchResponse(MeshDfuTransportResponse res) {
	event_t eventOut(CS_TYPE::EVT_MESH_DFU_TRANSPORT_RESPONSE, &res, sizeof(res));
	eventOut.dispatch();
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++++++++++++++++++++++++ async flowcontrol ++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool MeshDfuTransport::setEventCallback(CS_TYPE evtToWaitOn, ExpectedEventCallback callback) {
	bool overridden = _onExpectedEvent != nullptr;

	_expectedEvent         = evtToWaitOn;
	_onExpectedEvent = callback;

	return overridden;
}

bool MeshDfuTransport::setTimeoutCallback(TimeoutCallback onTimeout, uint32_t timeoutMs) {
	bool overriden = _onTimeout != nullptr;

	_onTimeout = onTimeout;
	_timeOutRoutine.startSingleMs(timeoutMs);

	return overriden;
}

void MeshDfuTransport::clearEventCallback() {
	_onExpectedEvent = nullptr;
}

void MeshDfuTransport::clearTimeoutCallback() {
	_onTimeout = nullptr;
}

void MeshDfuTransport::onEventCallbackTimeOut() {
	LOGMeshDfuTransportInfo("+++ event callback timed out, calling _onTimeout");
	if(_onTimeout != nullptr) {
		(this->*_onTimeout)();
	}
}


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++++++++++++++++++++++++ nordic protocol ++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ------------- the adapter layer for crownstone_ble -------------

void MeshDfuTransport::write_control_point(cs_data_t buff) {
	LOGMeshDfuTransportDebug("MeshDfuTransport: write control point 0x%04X", _uuidHandles[Index::ControlPoint]);
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_NOTIFICATION, &MeshDfuTransport::onNotificationReceived);
	_bleCentral->write(_uuidHandles[Index::ControlPoint], buff.data, buff.len);

}

void MeshDfuTransport::write_data_point(cs_data_t buff) {
	LOGMeshDfuTransportDebug("MeshDfuTransport: write data point 0x%04X", _uuidHandles[Index::DataPoint]);
	_bleCentral->write(_uuidHandles[Index::DataPoint], buff.data, buff.len);
}

// ----------------------- recovery methods -----------------------

// ------------------- nordic protocol commands -------------------


void MeshDfuTransport::_setPrn() {
	LOGMeshDfuTransportDebug("set PRN");
	cs_data_t buff = _bleCentral->requestWriteBuffer();

	constexpr uint8_t len = sizeof(OP_CODE) + sizeof(_prn);
	if(buff.data == nullptr || buff.len < len) {
		return;
	}

	_lastOperation = OP_CODE::SetPRN;
	buff.data[0] = static_cast<uint8_t>(OP_CODE::SetPRN);
	memcpy(	&buff.data[1], &_prn, sizeof(_prn)); // little endian unsigned uint16_t

	buff.len = len;

	write_control_point(buff);
}

void  MeshDfuTransport::_calculateChecksum() {
	LOGMeshDfuTransportDebug("Calculate checksum");
	cs_data_t buff = _bleCentral->requestWriteBuffer();

	constexpr uint8_t len = sizeof(OP_CODE);
	if(buff.data == nullptr || buff.len < len) {
		return;
	}

	_lastOperation = OP_CODE::CalcChecSum;
	buff.data[0] = static_cast<uint8_t>(OP_CODE::CalcChecSum);

	buff.len = len;

	write_control_point(buff);
}

void  MeshDfuTransport::_execute() {
	LOGMeshDfuTransportDebug("Execute");
	cs_data_t buff = _bleCentral->requestWriteBuffer();

	constexpr uint8_t len = sizeof(OP_CODE);
	if(buff.data == nullptr || buff.len < len) {
		return;
	}

	_lastOperation = OP_CODE::Execute;
	buff.data[0] = static_cast<uint8_t>(OP_CODE::Execute);

	buff.len = len;

	write_control_point(buff);
}

void  MeshDfuTransport::_createObject(uint8_t objectType, uint32_t size) {
	LOGMeshDfuTransportDebug("Create object id:%u, size:%u", objectType, size);
	cs_data_t buff = _bleCentral->requestWriteBuffer();

	constexpr uint8_t len = sizeof(OP_CODE) + sizeof(objectType) + sizeof(size);
	if(buff.data == nullptr || buff.len < len) {
		return;
	}

	_lastOperation = OP_CODE::CreateObject;
	buff.data[0] = static_cast<uint8_t>(OP_CODE::CreateObject);
	buff.data[1] = objectType;

	memcpy(&buff.data[2], &size, sizeof(size));

	buff.len = len;

	write_control_point(buff);
}

void MeshDfuTransport::_createCommand(uint32_t size) {
	LOGMeshDfuTransportDebug("_createCommand %u", size);
	_createObject(0x01, size);
}

void MeshDfuTransport::_createData(uint32_t size) {
	_createObject(0x02, size);
}

void  MeshDfuTransport::_selectObject(uint8_t objectType) {
	LOGMeshDfuTransportDebug("Select object id:%u", objectType);
	cs_data_t buff = _bleCentral->requestWriteBuffer();

	constexpr uint8_t len = sizeof(OP_CODE) + sizeof(objectType);
	if(buff.data == nullptr || buff.len < len) {
		return;
	}

	_lastOperation = OP_CODE::ReadObject;
	buff.data[0] = static_cast<uint8_t>(OP_CODE::ReadObject);
	buff.data[1] = objectType;

	buff.len = len;

	write_control_point(buff);
}


void MeshDfuTransport::_selectCommand(){
	_selectObject(0x01);
}

void MeshDfuTransport::_selectData() {
	_selectObject(0x02);
}

// -------------------- raw data communication --------------------

cs_ret_code_t MeshDfuTransport::_parseResult(cs_const_data_t evtData) {
	LOGMeshDfuTransportDebug("Parse dfu notification result, len %u", evtData.len);
	_logArray(SERIAL_DEBUG, true, evtData.data, CsMath::min(evtData.len, 32u), "%02x");

	if (evtData.data == nullptr || evtData.len < 2) {
		return ERR_BUFFER_UNASSIGNED;
	}

	if (static_cast<OP_CODE>(evtData.data[0]) != OP_CODE::Response) {
		return ERR_WRONG_MODE;
	}

	if(static_cast<OP_CODE>(evtData.data[1]) != _lastOperation) {
		LOGMeshDfuTransportWarn("Wrong operation: last sent %u != notification result %u",
				_lastOperation, static_cast<OP_CODE>(evtData.data[1]));
		return ERR_WRONG_STATE;
	}

	RES_CODE resCode = static_cast<RES_CODE>(evtData.data[2]);

	if(resCode == RES_CODE::Success) {
		LOGMeshDfuTransportDebug("notification result: success");
		return ERR_SUCCESS; // return result is  data[3:]
	}

	LOGMeshDfuTransportDebug("notification result: error %u", static_cast<uint8_t>(resCode));

	switch(resCode) {
		case RES_CODE::InvalidCode:           return ERR_UNKNOWN_OP_CODE;
		case RES_CODE::NotSupported:          return ERR_PROTOCOL_UNSUPPORTED;
		case RES_CODE::InvalidParameter:      return ERR_WRONG_PARAMETER;
		case RES_CODE::InsufficientResources: return ERR_NO_SPACE;
		case RES_CODE::InvalidObject:         return ERR_INVALID_MESSAGE;
		case RES_CODE::InvalidSignature:      return ERR_MISMATCH;
		case RES_CODE::UnsupportedType:       return ERR_UNKNOWN_TYPE;
		case RES_CODE::OperationNotPermitted: return ERR_WRONG_OPERATION;
		case RES_CODE::OperationFailed:       return ERR_OPERATION_FAILED;

		case RES_CODE::ExtendedError: {
			uint8_t errcode = evtData.len >=3 ? evtData.data[3] : 0;

			LOGMeshDfuTransportWarn("Dfu Transport extended error: %u",
						MeshDfuConstants::DfuTransportBle::EXT_ERROR_CODE(errcode));
			return ERR_UNHANDLED;
		}
		default: {
			LOGMeshDfuTransportWarn("Dfu Transport unspecified error occured");
			return ERR_UNSPECIFIED;
		}
	}
}

MeshDfuTransportResponse MeshDfuTransport::_parseResponseReadObject(cs_const_data_t evtData) {
	LOGMeshDfuTransportDebug("Parse dfu notification read object result");
	MeshDfuTransportResponse response;
	response.result = _parseResult(evtData);

	if(response.result == ERR_SUCCESS) {
		// incoming data starts at index 3, in little endian (native byte order).
		const uint32_t len = sizeof(uint32_t);
		const uint32_t start = 3;
		const uint32_t num_items = 3;

		if(evtData.len < start + num_items*len) {
			LOGMeshDfuTransportWarn("Dfu Transport error: packet too small for current operation");
			response.result = ERR_BUFFER_TOO_SMALL;
		} else {
			memcpy(&response.max_size, &evtData.data[start + 0 * len], len);
			memcpy(&response.offset,   &evtData.data[start + 1 * len], len);
			memcpy(&response.crc,      &evtData.data[start + 2 * len], len);
		}
	}

	return response;
}


MeshDfuTransportResponse MeshDfuTransport::_parseResponseCalcChecksum(cs_const_data_t evtData) {
	LOGMeshDfuTransportDebug("Parse dfu notification calck checksum result");

	MeshDfuTransportResponse response;
	response.result = _parseResult(evtData);

	if(response.result == ERR_SUCCESS) {
		// incoming data starts at index 3, in little endian (native byte order).
		const uint32_t len = sizeof(uint32_t);
		const uint32_t start = 3;
		const uint32_t num_items = 2;

		if(evtData.len < start + num_items*len) {
			LOGMeshDfuTransportWarn("Dfu Transport error: packet too small for current operation");
			response.result = ERR_BUFFER_TOO_SMALL;
		} else {
			memcpy(&response.offset,   &evtData.data[start + 0 * len], len);
			memcpy(&response.crc,      &evtData.data[start + 1 * len], len);
		}
	}

	return response;
}

// ---------------------------- event handling ----------------------------


void MeshDfuTransport::onDisconnect() {
	if(isBusy()) {
		LOGMeshDfuTransportWarn("disconnected during dfu operation");
	}

	clearConnectionData();
	clearEventCallback();
	clearTimeoutCallback();
}

void MeshDfuTransport::onTimeout() {
	if(!isBusy()) {
		LOGMeshDfuTransportDebug("timeout callback invoked, but not currently busy. Forgot to cancel timeout?");
	}

	clearEventCallback();

	MeshDfuTransportResponse response;
	response.result = ERR_TIMEOUT;

	switch(_lastOperation){
		case OP_CODE::SetPRN:
		case OP_CODE::CreateObject:
		case OP_CODE::Execute:
			dispatchResult(ERR_TIMEOUT);
			break;

		case OP_CODE::ReadObject:
		case OP_CODE::CalcChecSum:
			dispatchResponse(response);
			break;

		case OP_CODE::Response:
		case OP_CODE::None:
			LOGMeshDfuTransportDebug("timeout callback invoked, but current operation is Response or None");
	}
}

void MeshDfuTransport::onDiscover(ble_central_discovery_t& result) {
	if(_discoveryComplete) {
		LOGMeshDfuTransportDebug("Discovery was already completed. Continuing after clearing old data.");
		clearConnectionData();
	}

	if(result.uuid == _dfuServiceUuid) {
		LOGMeshDfuTransportInfo("Found dfu service handle");
		_dfuServiceFound = true;
		return;
	}

	LOGMeshDfuTransportDebug("result uuid: 0x04%x", result.uuid.getUuid().uuid);

	for (auto index : {Index::ControlPoint, Index::DataPoint}) {
		if (result.uuid == _uuids[index]) {
			LOGMeshDfuTransportDebug("Found dfu characteristic handle for Index %u", index);
			_uuidHandles[index] = result.valueHandle;
			_cccdHandles[index] = result.cccdHandle;
			return;
		}
	}
}

void MeshDfuTransport::onDiscoveryComplete() {
	_discoveryComplete = true;
}

void MeshDfuTransport::onNotificationReceived(event_t& event) {
	LOGMeshDfuTransportDebug("On notification received");
	TYPIFY(EVT_BLE_CENTRAL_NOTIFICATION)* packet = CS_TYPE_CAST(EVT_BLE_CENTRAL_NOTIFICATION, event.data);

	// do something specific after operation finished (?)
	switch(_lastOperation) {
		case OP_CODE::SetPRN: {
			cs_ret_code_t result = _parseResult(packet->data);
			dispatchResult(result);
			break;
		}
		case OP_CODE::CreateObject: {
			cs_ret_code_t result = _parseResult(packet->data);
			dispatchResult(result);
			break;
		}
		case OP_CODE::Execute: {
			cs_ret_code_t result = _parseResult(packet->data);
			dispatchResult(result);
			break;
		}
		case OP_CODE::ReadObject: {
			MeshDfuTransportResponse response = _parseResponseReadObject(packet->data);
			dispatchResponse(response);
			break;
		}
		case OP_CODE::CalcChecSum: {
			MeshDfuTransportResponse response = _parseResponseCalcChecksum(packet->data);
			dispatchResponse(response);
			break;
		}
		case OP_CODE::Response: {
			// _lastOperation should never be Response. It is the target that responds, not us.
			dispatchResult(ERR_WRONG_STATE);
			break;
		}
		case OP_CODE::None: {
			// forgot to set _lastOperation somewhere?
			dispatchResult(ERR_WRONG_STATE);
			break;
		}
	}
}

void MeshDfuTransport::handleEvent(event_t& event) {
	switch(event.type) {
		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY: {
			auto result = CS_TYPE_CAST(EVT_BLE_CENTRAL_DISCOVERY, event.data);
			onDiscover(*result);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT: {
			onDiscoveryComplete();
			break;
		}

		case CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED: {
			onDisconnect();
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: break;
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_CLEARANCE_REQUEST: break;

		case CS_TYPE::EVT_BLE_CENTRAL_NOTIFICATION: break;
		case CS_TYPE::EVT_BLE_CENTRAL_READ_RESULT: break;
		case CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT: break;
		default: break;
	}

	// if a callback is waiting on this event call it back.
	if (_onExpectedEvent != nullptr && event.type == _expectedEvent) {
		// create local copy
		auto eventCallback = _onExpectedEvent;

		// clear expected event
		_onExpectedEvent = nullptr;
		(this->*eventCallback)(event);
	}

	_timeOutRoutine.handleEvent(event);
}
