/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 7, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventDispatcher.h>
#include <processing/cs_EncryptionHandler.h>
#include <tracking/cs_TrackedDevices.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

#define LOGTrackedDevicesDebug LOGnone
#define LOGTrackedDevicesVerbose LOGnone

TrackedDevices::TrackedDevices() {
}

void TrackedDevices::init() {
	EventDispatcher::getInstance().addListener(this);
}

cs_ret_code_t TrackedDevices::handleRegister(internal_register_tracked_device_packet_t& packet) {
	LOGTrackedDevicesDebug("handleRegister id=%u", packet.data.deviceId);
	TrackedDevice* device = findOrAdd(packet.data.deviceId);
	if (device == nullptr) {
		return ERR_NO_SPACE;
	}
	if (!hasAccess(*device, packet.accessLevel)) {
		LOGw("No access id=%u oldLevel=%u newLevel=%u", packet.data.deviceId, device->data.accessLevel, packet.accessLevel);
		return ERR_NO_ACCESS;
	}
	if (!isTokenOkToSet(*device, packet.data.deviceToken, sizeof(packet.data.deviceToken))) {
		LOGw("Token already exists");
		return ERR_ALREADY_EXISTS;
	}
	setAccessLevel(*device, packet.accessLevel);
	setLocation(   *device, packet.data.locationId);
	setProfile(    *device, packet.data.profileId);
	setRssiOffset( *device, packet.data.rssiOffset);
	setFlags(      *device, packet.data.flags.asInt);
	setDevicetoken(*device, packet.data.deviceToken, sizeof(packet.data.deviceToken));
	setTTL(        *device, packet.data.timeToLiveMinutes);
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
	setLocation(*device, packet.locationId);
	setProfile(*device, packet.profileId);
	setRssiOffset(*device, packet.rssiOffset);
	setFlags(*device, packet.flags);
	setAccessLevel(*device, packet.accessLevel);
	print(*device);
}


void TrackedDevices::handleMeshToken(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)& packet) {
	LOGTrackedDevicesDebug("handleMeshToken id=%u", packet.deviceId);
	TrackedDevice* device = findOrAdd(packet.deviceId);
	if (device == nullptr) {
		return;
	}
	// Access has been checked by sending crownstone.
	if (!isTokenOkToSet(*device, packet.deviceToken, sizeof(packet.deviceToken))) {
		LOGw("Token already exists");
		return;
	}
	setDevicetoken(*device, packet.deviceToken, sizeof(packet.deviceToken));
	setTTL(*device, packet.ttlMinutes);
	print(*device);
}

void TrackedDevices::handleScannedDevice(adv_background_parsed_v1_t& packet) {
	TrackedDevice* device = findToken(packet.deviceToken, sizeof(packet.deviceToken));
	if (device == nullptr) {
		return;
	}
	// Maybe only check some fields to be set?
	if (!allFieldsSet(*device)) {
		LOGTrackedDevicesVerbose("not all fields set id=%u", device->data.data.deviceId);
		return;
	}
	if (!isValidTTL(*device)) {
		LOGTrackedDevicesVerbose("token expired id=%u", device->data.data.deviceId);
		return;
	}
	device->locationIdTimeout = LOCATION_ID_TIMEOUT_MINUTES;
//	if (!device->data.data.flags.flags.ignoreForBehaviour) {
//		sendLocation(*device);
//	}
	sendBackgroundAdv(*device, packet.macAddress, packet.rssi);
}

void TrackedDevices::sendDeviceList() {
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		sendRegisterToMesh(*iter);
		sendTokenToMesh(*iter);
	}
}




TrackedDevices::TrackedDevice* TrackedDevices::findOrAdd(device_id_t deviceId) {
	TrackedDevice* device = find(deviceId);
	if (device == nullptr) {
		device = add();
		device->data.data.deviceId = deviceId;
	}
	return device;
}

TrackedDevices::TrackedDevice* TrackedDevices::find(device_id_t deviceId) {
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if (iter->data.data.deviceId == deviceId) {
			LOGTrackedDevicesVerbose("found device");
			return &(*iter);
		}
	}
	return nullptr;
}

TrackedDevices::TrackedDevice* TrackedDevices::findToken(uint8_t* deviceToken, uint8_t size) {
	assert(size == TRACKED_DEVICE_TOKEN_SIZE, "Wrong device token size");
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if (memcmp(iter->data.data.deviceToken, deviceToken, size) == 0) {
			LOGTrackedDevicesVerbose("found token id=%u", iter->data.data.deviceId);
			return &(*iter);
		}
	}
	return nullptr;
}

TrackedDevices::TrackedDevice* TrackedDevices::add() {
	// Check number of devices.
	if (deviceListSize >= MAX_TRACKED_DEVICES) {
		cs_ret_code_t retCode = removeDevice();
		if (retCode != ERR_SUCCESS) {
			LOGTrackedDevicesDebug("no space to add");
			return nullptr;
		}
	}
	LOGTrackedDevicesDebug("add device");
	TrackedDevice device;
	devices.push_front(device);
	++deviceListSize;
	return &(devices.front());
}

cs_ret_code_t TrackedDevices::removeDevice() {
	uint16_t lowestTTL = 0xFFFF;
	auto iterToRemove = devices.end();
	auto prevIter = devices.before_begin();
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if (iter->fieldsSet != ALL_FIELDS_SET) {
			iterToRemove = prevIter;
			break;
		}
		if (iter->data.data.timeToLiveMinutes <= lowestTTL) {
			lowestTTL = iter->data.data.timeToLiveMinutes;
			iterToRemove = prevIter;
		}
		prevIter = iter;
	}
	[[maybe_unused]] auto removedIter = iterToRemove;
	LOGTrackedDevicesDebug("remove device id=%u", (++removedIter)->data.data.deviceId);
//	if (lowestTTL == 0xFFFF) {
//		return ERR_NOT_AVAILABLE;
//	}
	devices.erase_after(iterToRemove);
	--deviceListSize;
	return ERR_SUCCESS;
}


bool TrackedDevices::hasAccess(TrackedDevice& device, uint8_t accessLevel) {
	if (!isValidTTL(device)) {
		// Device token timed out, so anyone is allowed to set a token for the device.
		return true;
	}
	if (BLEutil::isBitSet(device.fieldsSet, BIT_POS_ACCESS_LEVEL) &&
			(!EncryptionHandler::getInstance().allowAccess((EncryptionAccessLevel)device.data.accessLevel, (EncryptionAccessLevel)accessLevel))) {
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
	return false;
}


bool TrackedDevices::allFieldsSet(TrackedDevice& device) {
	return device.fieldsSet == ALL_FIELDS_SET;
}

void TrackedDevices::setAccessLevel(TrackedDevice& device, uint8_t accessLevel) {
	device.data.accessLevel = accessLevel;
	BLEutil::setBit(device.fieldsSet, BIT_POS_ACCESS_LEVEL);
}

void TrackedDevices::setLocation(TrackedDevice& device, uint8_t locationId) {
	device.data.data.locationId = locationId;
	BLEutil::setBit(device.fieldsSet, BIT_POS_LOCATION);
	device.locationIdTimeout = LOCATION_ID_TIMEOUT_MINUTES;
}

void TrackedDevices::setProfile(TrackedDevice& device, uint8_t profileId) {
	device.data.data.profileId = profileId;
	BLEutil::setBit(device.fieldsSet, BIT_POS_PROFILE);
}

void TrackedDevices::setRssiOffset(TrackedDevice& device, int8_t rssiOffset) {
	device.data.data.rssiOffset = rssiOffset;
	BLEutil::setBit(device.fieldsSet, BIT_POS_RSSI_OFFSET);
}

void TrackedDevices::setFlags(TrackedDevice& device, uint8_t flags) {
	device.data.data.flags.asInt = flags;
	BLEutil::setBit(device.fieldsSet, BIT_POS_FLAGS);
}

void TrackedDevices::setDevicetoken(TrackedDevice& device, uint8_t* deviceToken, uint8_t size) {
	assert(size == TRACKED_DEVICE_TOKEN_SIZE, "Wrong device token size");
	memcpy(device.data.data.deviceToken, deviceToken, sizeof(device.data.data.deviceToken));
	BLEutil::setBit(device.fieldsSet, BIT_POS_DEVICE_TOKEN);
}

void TrackedDevices::setTTL(TrackedDevice& device, uint16_t ttlMinutes) {
	device.data.data.timeToLiveMinutes = ttlMinutes;
	BLEutil::setBit(device.fieldsSet, BIT_POS_TTL);
}

bool TrackedDevices::isValidTTL(TrackedDevice& device) {
	return (BLEutil::isBitSet(device.fieldsSet, BIT_POS_TTL)) && (device.data.data.timeToLiveMinutes != 0);
}

void TrackedDevices::print(TrackedDevice& device) {
	LOGTrackedDevicesDebug("id=%u fieldsSet=%u accessLvl=%u profile=%u location=%u rssiOffset=%i flags=%u TTL=%u token=[%u %u %u]",
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
}



void TrackedDevices::tickMinute() {
	LOGTrackedDevicesDebug("tickMinute");
	for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
		if (iter->locationIdTimeout != 0) {
			iter->locationIdTimeout--;
			if (iter->locationIdTimeout == 0) {
				iter->data.data.locationId = 0;
			}
		}
		if ((iter->fieldsSet & BIT_POS_TTL) && (iter->data.data.timeToLiveMinutes != 0)) {
			iter->data.data.timeToLiveMinutes--;
		}
		print(*iter);
	}
}



void TrackedDevices::sendBackgroundAdv(TrackedDevice& device, uint8_t* macAddress, int8_t rssi) {
	LOGTrackedDevicesVerbose("sendBackgroundAdv id=%u", device.data.data.deviceId);
	if (!allFieldsSet(device)) {
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

void TrackedDevices::sendLocation(TrackedDevice& device) {
	LOGTrackedDevicesVerbose("sendLocation id=%u", device.data.data.deviceId);
	if (!allFieldsSet(device)) {
		return;
	}
	TYPIFY(EVT_PROFILE_LOCATION) eventData;
	eventData.fromMesh = false;
	eventData.profileId = device.data.data.profileId;
	eventData.locationId = device.data.data.locationId;
	event_t event(CS_TYPE::EVT_PROFILE_LOCATION, &eventData, sizeof(eventData));
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
	eventData.deviceId   = device.data.data.deviceId;
	memcpy(eventData.deviceToken, device.data.data.deviceToken, sizeof(device.data.data.deviceToken));
	eventData.ttlMinutes = device.data.data.timeToLiveMinutes;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN, &eventData, sizeof(eventData));
	event.dispatch();
}

void TrackedDevices::handleEvent(event_t& evt) {
	switch(evt.type) {
		case CS_TYPE::CMD_REGISTER_TRACKED_DEVICE: {
			internal_register_tracked_device_packet_t* data = reinterpret_cast<TYPIFY(CMD_REGISTER_TRACKED_DEVICE)*>(evt.data);
			evt.result.returnCode = handleRegister(*data);
			break;
		}
		case CS_TYPE::CMD_UPDATE_TRACKED_DEVICE: {
			internal_update_tracked_device_packet_t* data = reinterpret_cast<TYPIFY(CMD_UPDATE_TRACKED_DEVICE)*>(evt.data);
			evt.result.returnCode = handleUpdate(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_REGISTER: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)*>(evt.data);
			handleMeshRegister(*data);
			break;
		}
		case CS_TYPE::EVT_MESH_TRACKED_DEVICE_TOKEN: {
			TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)* data = reinterpret_cast<TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)*>(evt.data);
			handleMeshToken(*data);
			break;
		}
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED_V1: {
			TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1)* data = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1)*>(evt.data);
			handleScannedDevice(*data);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			if (--ticksLeft == 0) {
				ticksLeft = TICKS_PER_MINUTES;
				tickMinute();
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING: {
			if (!deviceListIsSynced) {
				auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING)*>(evt.data);
				req->bits.trackedDevices = true;
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING: {
			auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)*>(evt.data);
			if (req->bits.time && deviceListIsSynced) {
				// Device list is requested by a crownstone in the mesh.
				// If we are synced, send it.
				// But only with a 1/10 chance, to prevent flooding the mesh.
				uint8_t rand8;
				RNG::fillBuffer(&rand8, 1);
				if (rand8 < (255 / 10 + 1)) {
					sendDeviceList();
				}
			}
			break;
		}
		default:
			break;
	}
}
