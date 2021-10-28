/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 7, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_RNG.h>
#include <encryption/cs_KeysAndAccess.h>
#include <events/cs_EventDispatcher.h>
#include <tracking/cs_TrackedDevices.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

#define LOGTrackedDevicesDebug LOGvv
#define LOGTrackedDevicesVerbose LOGvv

TrackedDevices::TrackedDevices() {
}

void TrackedDevices::init() {
	LOGi("Init. Using %u bytes of RAM.", sizeof(_devices));
	EventDispatcher::getInstance().addListener(this);
}

cs_ret_code_t TrackedDevices::handleRegister(internal_register_tracked_device_packet_t& packet) {
	LOGTrackedDevicesDebug("handleRegister id=%u", packet.data.deviceId);
	TrackedDevice* device = findOrAdd(packet.data.deviceId);
	if (device == nullptr) {
		return ERR_NO_SPACE;
	}

	// When device token timed out, anyone is allowed to set a token for the device.
	if (!hasAccess(*device, packet.accessLevel)) {
		LOGw("No access id=%u oldLevel=%u newLevel=%u", packet.data.deviceId, device->data.accessLevel, packet.accessLevel);
		return ERR_NO_ACCESS;
	}
	if (!isTokenOkToSet(*device, packet.data.deviceToken, sizeof(packet.data.deviceToken))) {
		return ERR_ALREADY_EXISTS;
	}
	device->setAccessLevel(packet.accessLevel);
	device->setLocation(packet.data.locationId, LOCATION_ID_TTL_MINUTES);
	device->setProfile(packet.data.profileId);
	device->setRssiOffset(packet.data.rssiOffset);
	device->setFlags(packet.data.flags.asInt);
	device->setDevicetoken(packet.data.deviceToken, sizeof(packet.data.deviceToken));
	device->setTTL(packet.data.timeToLiveMinutes);
	sendRegisterToMesh(*device);
	sendTokenToMesh(*device);
	print(*device);
	return ERR_SUCCESS;
}

cs_ret_code_t TrackedDevices::handleUpdate(internal_update_tracked_device_packet_t& packet) {
	// For now, this packet is exactly the same as the register packet.
	return handleRegister(packet);
}

void TrackedDevices::handleMeshRegister(TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)& packet) {
	LOGTrackedDevicesDebug("handleMeshRegister id=%u", packet.deviceId);
	TrackedDevice* device = findOrAdd(packet.deviceId);
	if (device == nullptr) {
		return;
	}
	// Access has been checked by sending crownstone.
	device->setLocation(packet.locationId, LOCATION_ID_TTL_MINUTES);
	device->setProfile(packet.profileId);
	device->setRssiOffset(packet.rssiOffset);
	device->setFlags(packet.flags);
	device->setAccessLevel(packet.accessLevel);
	print(*device);
	checkSynced();
}


void TrackedDevices::handleMeshToken(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)& packet) {
	LOGTrackedDevicesDebug("handleMeshToken id=%u", packet.deviceId);
	TrackedDevice* device = findOrAdd(packet.deviceId);
	if (device == nullptr) {
		return;
	}
	// Access has been checked by sending crownstone.
	if (!isTokenOkToSet(*device, packet.deviceToken, sizeof(packet.deviceToken))) {
		return;
	}
	device->setDevicetoken(packet.deviceToken, sizeof(packet.deviceToken));
	device->setTTL(packet.ttlMinutes);
	print(*device);
	checkSynced();
}

void TrackedDevices::handleMeshListSize(TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)& packet) {
	LOGTrackedDevicesDebug("handleMeshListSize size=%u", packet.listSize);
	_expectedDeviceListSize = packet.listSize;
	checkSynced();
}

void TrackedDevices::handleScannedDevice(adv_background_parsed_v1_t& packet) {
	TrackedDevice* device = findToken(packet.deviceToken, sizeof(packet.deviceToken));
	if (device == nullptr) {
		return;
	}
	// Maybe only check some fields to be set?
	if (!device->allFieldsSet()) {
		LOGTrackedDevicesVerbose("not all fields set id=%u", device->data.data.deviceId);
		return;
	}
	device->locationIdTTLMinutes = LOCATION_ID_TTL_MINUTES;

	sendBackgroundAdv(*device, packet.macAddress, packet.rssi);
}

cs_ret_code_t TrackedDevices::handleHeartbeat(internal_tracked_device_heartbeat_packet_t& packet) {
	LOGTrackedDevicesVerbose("handleHeartbeat deviceId=%u", packet.data.deviceId);

	TrackedDevice* device = find(packet.data.deviceId);
	if (device == nullptr) {
		return ERR_NOT_FOUND;
	}
	if (!device->allFieldsSet()) {
		LOGTrackedDevicesVerbose("Not all fields set id=%u", device->data.data.deviceId);
		return ERR_NOT_FOUND;
	}
	if (!hasAccess(*device, packet.accessLevel)) {
		LOGw("No access id=%u oldLevel=%u newLevel=%u", packet.data.deviceId, device->data.accessLevel, packet.accessLevel);
		return ERR_NO_ACCESS;
	}
	if (memcmp(device->data.data.deviceToken, packet.data.deviceToken, sizeof(packet.data.deviceToken)) != 0) {
		LOGw("Invalid token for id=%u stored=%u %u .. given=%u %u .. ", device->data.data.deviceId, device->data.data.deviceToken[0], device->data.data.deviceToken[1], packet.data.deviceToken[0], packet.data.deviceToken[1]);
		return ERR_NO_ACCESS;
	}

	cs_ret_code_t retCode = handleHeartbeat(*device, packet.data.locationId, packet.data.timeToLiveMinutes, false);
	if (retCode == ERR_SUCCESS) {
		sendHeartbeatToMesh(*device);
	}
	return retCode;
}

void TrackedDevices::handleMeshHeartbeat(TYPIFY(EVT_MESH_TRACKED_DEVICE_HEARTBEAT)& packet) {
	LOGTrackedDevicesVerbose("handleMeshHeartbeat deviceId=%u", packet.deviceId);
	TrackedDevice* device = find(packet.deviceId);
	if (device == nullptr) {
		return;
	}
	handleHeartbeat(*device, packet.locationId, packet.ttlMinutes, true);
}

cs_ret_code_t TrackedDevices::handleHeartbeat(TrackedDevice& device, uint8_t locationId, uint8_t ttlMinutes, bool fromMesh) {
	LOGTrackedDevicesDebug("handleHeartbeat deviceId=%u locationId=%u ttlMinutes=%u fromMesh=%u", device.data.data.deviceId, locationId, ttlMinutes, fromMesh);
	if (ttlMinutes > HEARTBEAT_TTL_MINUTES_MAX) {
		LOGd("Invalid heartbeat TTL %u", ttlMinutes);
		return ERR_WRONG_PARAMETER;
	}
	device.data.data.locationId = locationId;
	device.heartbeatTTLMinutes = ttlMinutes;

	// Make sure the location ID doesn't timeout before the heartbeat times out.
	device.locationIdTTLMinutes = std::max(ttlMinutes, LOCATION_ID_TTL_MINUTES);

	sendHeartbeatLocation(device, fromMesh, false);
	return ERR_SUCCESS;
}

TrackedDevice* TrackedDevices::findOrAdd(device_id_t deviceId) {
	TrackedDevice* device = find(deviceId);
	if (device == nullptr) {
		device = add();
		if (device != nullptr) {
			device->data.data.deviceId = deviceId;
		}
	}
	return device;
}

TrackedDevice* TrackedDevices::find(device_id_t deviceId) {
	LOGTrackedDevicesVerbose("find id=%u listSize=%u", deviceId, _deviceListSize);
	for (uint8_t i = 0; i < _deviceListSize; ++i) {
		if (_devices[i].isValid() && _devices[i].data.data.deviceId == deviceId) {
			LOGTrackedDevicesVerbose("found device at index=%u", i);
			return &(_devices[i]);
		}
	}
	return nullptr;
}

TrackedDevice* TrackedDevices::findToken(uint8_t* deviceToken, uint8_t size) {
	assert(size == TRACKED_DEVICE_TOKEN_SIZE, "Wrong device token size");
	LOGTrackedDevicesVerbose("find token=%02X:%02X:%02X listSize=%u", deviceToken[0], deviceToken[1], deviceToken[2], _deviceListSize);
	for (uint8_t i = 0; i < _deviceListSize; ++i) {
		if (_devices[i].isValid() && memcmp(_devices[i].data.data.deviceToken, deviceToken, size) == 0) {
			LOGTrackedDevicesVerbose("found token at index=%u id=%u", i, _devices[i].data.data.deviceId);
			return &(_devices[i]);
		}
	}
	return nullptr;
}

TrackedDevice* TrackedDevices::add() {
	LOGTrackedDevicesDebug("add");

	// Use empty spot.
	uint8_t incompleteIndex = 0xFF;
	uint16_t lowestTtl = 0xFFFF;
	uint8_t lowestTtlIndex = 0xFF;
	for (uint8_t i = 0; i < _deviceListSize; ++i) {
		if (!_devices[i].isValid()) {
			LOGTrackedDevicesDebug("Use empty spot: index=%u", i);
			return &(_devices[i]);
		}
		if (!_devices[i].allFieldsSet()) {
			incompleteIndex = i;
		}
		if (_devices[i].data.data.timeToLiveMinutes < lowestTtl) {
			lowestTtlIndex = i;
		}
	}

	// Increase list size.
	if (_deviceListSize < MAX_TRACKED_DEVICES) {
		uint8_t index = _deviceListSize++;
		LOGTrackedDevicesDebug("Increase list size: index=%u listSize=%u", index, _deviceListSize);
		_devices[index].invalidate();
		return &(_devices[index]);
	}

	// Use spot with incomplete data.
	if (incompleteIndex != 0xFF) {
		LOGTrackedDevicesDebug("Use spot with incomplete data: index=%u", incompleteIndex);
		_devices[incompleteIndex].invalidate();
		return &(_devices[incompleteIndex]);
	}

	// Use spot with lowest TTL.
	if (lowestTtlIndex != 0xFF) {
		LOGTrackedDevicesDebug("Use spot with lowest TTL: index=%u", lowestTtlIndex);
		_devices[lowestTtlIndex].invalidate();
		return &(_devices[lowestTtlIndex]);
	}

	// Shouldn't happen.
	LOGw("No space");
	return nullptr;
}

bool TrackedDevices::hasAccess(TrackedDevice& device, uint8_t accessLevel) {
	if (CsUtils::isBitSet(device.fieldsSet, BIT_POS_ACCESS_LEVEL) &&
			(!KeysAndAccess::getInstance().allowAccess((EncryptionAccessLevel)device.data.accessLevel, (EncryptionAccessLevel)accessLevel))) {
		return false;
	}
	return true;
}

bool TrackedDevices::isTokenOkToSet(TrackedDevice& device, uint8_t* deviceToken, uint8_t size) {
	TrackedDevice* otherDevice = findToken(deviceToken, size);
	if (otherDevice == nullptr) {
		return true;
	}
	if (otherDevice->data.data.deviceId == device.data.data.deviceId) {
		return true;
	}
	LOGw("Token already exists (id=%u)", otherDevice->data.data.deviceId);
	return false;
}

void TrackedDevices::checkSynced() {
	if (_deviceListIsSynced) {
		return;
	}
	if (_deviceListSize < _expectedDeviceListSize) {
		LOGTrackedDevicesDebug("Expecting more devices, current=%u expected=%u", _deviceListSize, _expectedDeviceListSize);
		return;
	}
	for (uint8_t i = 0; i < _deviceListSize; ++i) {
		if (!_devices[i].allFieldsSet()) {
			LOGTrackedDevicesDebug("Not all fields set for id=%u", _devices[i].data.data.deviceId);
			return;
		}
	}
	LOGi("Synced");
	_deviceListIsSynced = true;
}





void TrackedDevices::print(TrackedDevice& device) {
#if CS_SERIAL_NRF_LOG_ENABLED == 0
	LOGTrackedDevicesDebug("id=%u fieldsSet=%u accessLvl=%u profile=%u location=%u rssiOffset=%i flags=%u TTL=%u token=%02X:%02X:%02X",
			device.data.data.deviceId,
			device.fieldsSet,
			device.data.accessLevel,
			device.data.data.profileId,
			device.data.data.locationId,
			device.data.data.rssiOffset,
			device.data.data.flags.asInt,
			device.data.data.timeToLiveMinutes,
			device.data.data.deviceToken[0],
			device.data.data.deviceToken[1],
			device.data.data.deviceToken[2]
	);
#endif
}

void TrackedDevices::tickMinute() {
	LOGTrackedDevicesDebug("tickMinute");
	for (uint8_t i = 0; i < _deviceListSize; ++i) {
		if (!_devices[i].isValid()) {
			continue;
		}
		if (_devices[i].locationIdTTLMinutes != 0) {
			_devices[i].locationIdTTLMinutes--;
			if (_devices[i].locationIdTTLMinutes == 0) {
				// Location timed out.
				_devices[i].data.data.locationId = 0;
			}
		}
		if (_devices[i].data.data.timeToLiveMinutes != 0) {
			_devices[i].data.data.timeToLiveMinutes--;
		}
		if (_devices[i].data.data.timeToLiveMinutes == 0) {
			// Always check if device is timed out, as it might be that the TTL was never set.
			LOGTrackedDevicesDebug("Timed out id=%u", _devices[i].data.data.deviceId);
			_devices[i].invalidate();
		}

		if (_devices[i].heartbeatTTLMinutes != 0) {
			_devices[i].heartbeatTTLMinutes--;
		}
		print(_devices[i]);
	}

	// Remove trailing invalid devices.
	while (_deviceListSize > 0 && !_devices[_deviceListSize - 1].isValid()) {
		_deviceListSize--;
		LOGTrackedDevicesDebug("Shrinking list size to %u", _deviceListSize);
	}
}

void TrackedDevices::tickSecond() {
	for (uint8_t i = 0; i < _deviceListSize; ++i) {
		if (_devices[i].isValid() && _devices[i].allFieldsSet() && _devices[i].heartbeatTTLMinutes != 0) {
			sendHeartbeatLocation(_devices[i], false, true);
		}
	}
}

void TrackedDevices::sendBackgroundAdv(TrackedDevice& device, uint8_t* macAddress, int8_t rssi) {
	LOGTrackedDevicesVerbose("sendBackgroundAdv id=%u", device.data.data.deviceId);
	if (!device.allFieldsSet()) {
		return;
	}
	TYPIFY(EVT_ADV_BACKGROUND_PARSED) eventData;
	eventData.macAddress = macAddress;
	eventData.adjustedRssi = rssi + device.data.data.rssiOffset;
	eventData.locationId = device.data.data.locationId;
	eventData.profileId = device.data.data.profileId;
	eventData.flags = device.data.data.flags.asInt;
	LOGTrackedDevicesVerbose("sendBackgroundAdv adjustedRssi=%i locationId=%u profileId=%u flags=%u", eventData.adjustedRssi, eventData.locationId, eventData.profileId, eventData.flags);
	event_t event(CS_TYPE::EVT_ADV_BACKGROUND_PARSED, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendHeartbeatLocation(TrackedDevice& device, bool fromMesh, bool simulated) {
	LOGTrackedDevicesVerbose("sendHeartbeatLocation id=%u", device.data.data.deviceId);
	if (!device.allFieldsSet()) {
		return;
	}
	if (device.data.data.flags.flags.ignoreForBehaviour) {
		return;
	}
	TYPIFY(EVT_RECEIVED_PROFILE_LOCATION) eventData;
	eventData.fromMesh = fromMesh;
	eventData.simulated = simulated;
	eventData.profileId = device.data.data.profileId;
	eventData.locationId = device.data.data.locationId;
	event_t event(CS_TYPE::EVT_RECEIVED_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendHeartbeatToMesh(TrackedDevice& device) {
	TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_HEARTBEAT) meshMsg;
	meshMsg.deviceId = device.data.data.deviceId;
	meshMsg.locationId = device.data.data.locationId;
	meshMsg.ttlMinutes = device.heartbeatTTLMinutes;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_HEARTBEAT, &meshMsg, sizeof(meshMsg));
	event.dispatch();
}

void TrackedDevices::sendRegisterToMesh(TrackedDevice& device) {
	LOGTrackedDevicesDebug("sendRegisterToMesh id=%u", device.data.data.deviceId);
	TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER) eventData;
	eventData.accessLevel = device.data.accessLevel;
	eventData.deviceId    = device.data.data.deviceId;
	eventData.flags       = device.data.data.flags.asInt;
	eventData.locationId  = device.data.data.locationId;
	eventData.profileId   = device.data.data.profileId;
	eventData.rssiOffset  = device.data.data.rssiOffset;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendTokenToMesh(TrackedDevice& device) {
	LOGTrackedDevicesDebug("sendTokenToMesh id=%u", device.data.data.deviceId);
	TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN) eventData;
	eventData.deviceId = device.data.data.deviceId;
	memcpy(eventData.deviceToken, device.data.data.deviceToken, sizeof(device.data.data.deviceToken));
	eventData.ttlMinutes = device.data.data.timeToLiveMinutes;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendListSizeToMesh(uint8_t deviceCount) {
	LOGTrackedDevicesDebug("sendListSizeToMesh size=%u", deviceCount);
	TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE) eventData;
	eventData.listSize = deviceCount;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::sendDeviceList() {
	LOGTrackedDevicesDebug("sendDeviceList");
	uint8_t sentDevicesCount = 0;
	for (uint8_t i = 0; i < _deviceListSize; ++i) {
		if (_devices[i].isValid() && _devices[i].allFieldsSet()) {
			sendRegisterToMesh(_devices[i]);
			sendTokenToMesh(_devices[i]);
			++sentDevicesCount;
		}
	}

	sendListSizeToMesh(sentDevicesCount);
}

void TrackedDevices::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::CMD_REGISTER_TRACKED_DEVICE: {
			internal_register_tracked_device_packet_t* data = reinterpret_cast<TYPIFY(CMD_REGISTER_TRACKED_DEVICE)*>(event.data);
			event.result.returnCode = handleRegister(*data);
			break;
		}
		case CS_TYPE::CMD_UPDATE_TRACKED_DEVICE: {
			internal_update_tracked_device_packet_t* data = reinterpret_cast<TYPIFY(CMD_UPDATE_TRACKED_DEVICE)*>(event.data);
			event.result.returnCode = handleUpdate(*data);
			break;
		}
		case CS_TYPE::CMD_TRACKED_DEVICE_HEARTBEAT: {
			internal_tracked_device_heartbeat_packet_t* data = reinterpret_cast<TYPIFY(CMD_TRACKED_DEVICE_HEARTBEAT)*>(event.data);
			event.result.returnCode = handleHeartbeat(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_REGISTER: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)*>(event.data);
			handleMeshRegister(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_TOKEN: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)*>(event.data);
			handleMeshToken(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_HEARTBEAT: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_HEARTBEAT)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_HEARTBEAT)*>(event.data);
			handleMeshHeartbeat(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_LIST_SIZE: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)*>(event.data);
			handleMeshListSize(*data);
			break;
		}
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED_V1: {
			TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1)* data = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1)*>(event.data);
			handleScannedDevice(*data);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			if (--ticksLeftMinute == 0) {
				ticksLeftMinute = TICKS_PER_MINUTES;
				tickMinute();
			}
			if (--ticksLeftSecond == 0) {
				ticksLeftSecond = TICKS_PER_SECOND;
				tickSecond();
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING: {
			if (!_deviceListIsSynced) {
				auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING)*>(event.data);
				req->bits.trackedDevices = true;
				_expectedDeviceListSize = 0xFF;
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING: {
			auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)*>(event.data);
			if (req->bits.trackedDevices && _deviceListIsSynced) {
				// Device list is requested by a crownstone in the mesh.
				// If we are synced, send it.
				// But only with a 0.15 chance (0.15 * 255 = 39), to prevent flooding the mesh.
				if (RNG::getInstance().getRandom8() < 39) {
					sendDeviceList();
				}
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_FAILED: {
			LOGTrackedDevicesDebug("Mesh sync failed consider synced in any case. Device list is synced=%u", _deviceListIsSynced);
			_deviceListIsSynced = true;
			break;
		}
		default:
			break;
	}
}
