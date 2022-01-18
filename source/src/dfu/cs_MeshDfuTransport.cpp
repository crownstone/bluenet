/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <dfu/cs_MeshDfuTransport.h>
#include <dfu/cs_MeshDfuConstants.h>


#define LOGMeshDfuTransportDebug LOGd
#define LOGMeshDfuTransportInfo LOGi
#define LOGMeshDfuTransportWarn LOGw

cs_ret_code_t MeshDfuTransport::init() {
	if (!_firstInit) {
		return ERR_SUCCESS;
	}

	_bleCentral = getComponent<BleCentral>();
	if(_bleCentral == nullptr){
		return ERR_NOT_AVAILABLE;
	}

	// Make use of the fact ERR_SUCCESS = 0, to avoid many if statements.
	cs_ret_code_t retCode = ERR_SUCCESS;
	retCode |= _dfuServiceUuid.fromShortUuid(MeshDfuConstants::DFUAdapter::dfuServiceShortUuid);

	retCode |= _uuids[Index::ControlPoint].fromFullUuid(MeshDfuConstants::DFUAdapter::controlPointString);
	retCode |= _uuids[Index::DataPoint].fromFullUuid(MeshDfuConstants::DFUAdapter::dataPointString);

	clearConnectionData();

	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	listen();

	_firstInit = false;
	return ERR_SUCCESS;
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


void MeshDfuTransport::onDisconnect() {
	clearConnectionData();
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
			return;
		}
	}
}

void MeshDfuTransport::onDiscoveryComplete() {
	_discoveryComplete = true;
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

//void MeshDfuTransport::writeCharacteristicWithoutResponse(uint16_t characteristicHandle, uint8_t* data, uint8_t len){
//	_bleCentral->write(characteristicHandle, data, len);
//}
//void MeshDfuTransport::writeCharacteristicForResponse(uint16_t characteristicHandle, uint8_t* data, uint8_t len) {
//	_bleCentral->write(characteristicHandle, data, len);
//	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_NOTIFICATION, &MeshDfuTransport::onNotificationReceived);
//}

void MeshDfuTransport::write_control_point(cs_data_t buff) {
	setEventCallback(CS_TYPE::EVT_BLE_CENTRAL_NOTIFICATION, &MeshDfuTransport::onNotificationReceived);
	_bleCentral->write(_uuidHandles[Index::ControlPoint], buff.data, buff.len);

}

void MeshDfuTransport::write_data_point(cs_data_t buff) {
	// data writes don't have to wait
	_bleCentral->write(_uuidHandles[Index::DataPoint], buff.data, buff.len);
}


// ----------------- utility forwardering methods -----------------

// ----------------------- recovery methods -----------------------

// ------------------- nordic protocol commands -------------------


void MeshDfuTransport::_setPrn() {
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

void  MeshDfuTransport::_createObject(uint8_t objectType, uint32_t size) {
	cs_data_t buff = _bleCentral->requestWriteBuffer();

	constexpr uint8_t len = sizeof(OP_CODE) + sizeof(objectType) + sizeof(size);
	if(buff.data == nullptr || buff.len < len) {
		return;
	}

	_lastOperation = OP_CODE::CreateObject;
	buff.data[0] = static_cast<uint8_t>(OP_CODE::CreateObject);
	buff.data[1] = objectType;
	memcpy(&buff.data[2], &size, sizeof(size)); // little endian unsigned uint32_t

	buff.len = len;

	write_control_point(buff);
}

void MeshDfuTransport::_createCommand(uint32_t size) {
	_createObject(0x01, size);
}

void MeshDfuTransport::_createData(uint32_t size) {
	_createObject(0x02, size);
}


void  MeshDfuTransport::_execute() {
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

void MeshDfuTransport::_selectCommand(){
	_selectObject(0x01);
}

void MeshDfuTransport::_selectData() {
	_selectObject(0x02);
}

//
//
//async def __calculate_checksum(self):
//    raw_response = await self.write_control_point([DfuTransportBle.OP_CODE['CalcChecSum']])
//    response = self.__parse_response(raw_response, DfuTransportBle.OP_CODE['CalcChecSum'])
//
//    (offset, crc) = struct.unpack('<II', bytearray(response))
//    return {'offset': offset, 'crc': crc}
//


//async def __select_object(self, object_type):
//    logger.debug("BLE: Selecting Object: type:{}".format(object_type))
//    raw_response = await self.write_control_point([DfuTransportBle.OP_CODE['ReadObject'], object_type])
//    response = self.__parse_response(raw_response, DfuTransportBle.OP_CODE['ReadObject'])
//
//    (max_size, offset, crc)= struct.unpack('<III', bytearray(response))
//    logger.debug("BLE: Object selected: max_size:{} offset:{} crc:{}".format(max_size, offset, crc))
//    return {'max_size': max_size, 'offset': offset, 'crc': crc}


// -------------------- raw data communication --------------------

cs_ret_code_t MeshDfuTransport::_parseResponse(OP_CODE lastOperation, cs_const_data_t evtData) {
	if (evtData.data == nullptr || evtData.len < 2) {
		return ERR_BUFFER_UNASSIGNED;
	}

	if (static_cast<OP_CODE>(evtData.data[0]) != OP_CODE::Response) {
		return ERR_UNKNOWN_OP_CODE;
	}

	if(static_cast<OP_CODE>(evtData.data[1]) != lastOperation) {
		return ERR_WRONG_STATE;
	}

	switch(static_cast<RES_CODE>(evtData.data[2])) {
		case RES_CODE::Success : {
			return ERR_SUCCESS; // return result is  data[3:]
		}
		case RES_CODE::ExtendedError: {
			uint8_t errcode = evtData.len >=3 ? evtData.data[3] : 0;

			LOGMeshDfuTransportWarn("Dfu Transport extended error: %s",
						MeshDfuConstants::DfuTransportBle::EXT_ERROR_CODE(errcode));
			return ERR_INVALID_MESSAGE;
		}
		default: {
			LOGMeshDfuTransportWarn("Dfu Transport unspecified error occured");
			return ERR_UNSPECIFIED;

		}
	}
}

// ---------------------------- event handling ----------------------------

void MeshDfuTransport::onNotificationReceived(event_t& event) {
	TYPIFY(EVT_BLE_CENTRAL_NOTIFICATION)* packet = CS_TYPE_CAST(EVT_BLE_CENTRAL_NOTIFICATION, event.data);

	cs_ret_code_t result = _parseResponse(_lastOperation, packet->data);

	if(result != ERR_SUCCESS) {
		LOGMeshDfuTransportWarn("error during dfu transport. Result: %u");
		return;
	}


	// do something specific after operation finished (?)
	switch(_lastOperation) {
		case OP_CODE::SetPRN: { break; }
		case OP_CODE::CreateObject: { break;}
		case OP_CODE::Execute: { break; }
		case OP_CODE::Response: { break; }
		case OP_CODE::ReadObject: {
			onFinishOperationRead();
			break;
		}
		case OP_CODE::CalcChecSum: {
			onFinishOperationCheckSum();
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
