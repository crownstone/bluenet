/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 7, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventDispatcher.h>
#include <processing/cs_EncryptionHandler.h>
#include <tracking/cs_TrackedDevices.h>
#include <util/cs_Utils.h>

cs_ret_code_t TrackedDevices::handleRegister(internal_register_tracked_device_packet_t& packet) {
	TrackedDevice* device = findOrAdd(packet.data.deviceId);
	if (device == nullptr) {
		return ERR_NO_SPACE;
	}
	if (!hasAccess(*device), packet.accessLevel) {
		return ERR_NO_ACCESS;
	}
	setAccessLevel(*device, packet.accessLevel);
	setLocation(   *device, packet.data.locationId);
	setProfile(    *device, packet.data.profileId);
	setRssiOffset( *device, packet.data.rssiOffset);
	setFlags(      *device, packet.data.flags);
	setDevicetoken(*device, packet.data.deviceToken);
	setTTL(        *device, packet.data.timeToLiveMinutes);
	sendLocation(*device);
	return ERR_SUCCESS;
}

cs_ret_code_t TrackedDevices::handleUpdate(internal_update_tracked_device_packet_t& packet) {
	TrackedDevice* device = findOrAdd(packet.data.deviceId);
	if (device == nullptr) {
		return ERR_NO_SPACE;
	}
	if (!hasAccess(*device), packet.accessLevel) {
		return ERR_NO_ACCESS;
	}
	setAccessLevel(*device, packet.accessLevel);
	setLocation(   *device, packet.data.locationId);
	setProfile(    *device, packet.data.profileId);
	setRssiOffset( *device, packet.data.rssiOffset);
	setFlags(      *device, packet.data.flags);
	setDevicetoken(*device, packet.data.deviceToken);
	setTTL(        *device, packet.data.timeToLiveMinutes);
	sendLocation(*device);
	return ERR_SUCCESS;
}

void TrackedDevices::handleMeshRegister(TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)& packet) {
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
	sendLocation(*device);
}


void TrackedDevices::handleMeshToken(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)& packet) {
	TrackedDevice* device = findOrAdd(packet.deviceId);
	if (device == nullptr) {
		return;
	}
	// Access has been checked by sending crownstone.
	setDevicetoken(*device, packet.deviceToken);
	setTTL(*device, packet.ttlMinutes);
}






TrackedDevices::TrackedDevice* TrackedDevices::findOrAdd(device_id_t deviceId) {
	TrackedDevice* device = find(deviceId);
	if (device == nullptr) {
		device = add();
	}
	return device;
}


TrackedDevices::TrackedDevice* TrackedDevices::find(device_id_t deviceId) {
	for (auto iter = devices.begin(); iter != devices.end();) {
		if (iter->data.data.deviceId == deviceId) {
			return iter;
		}
	}
	return nullptr;
}

TrackedDevices::TrackedDevice* TrackedDevices::add() {
	// Check number of devices.
	if (devices.size() >= MAX_TRACKED_DEVICES) {
		cs_ret_code_t retCode = removeDevice();
		if (retCode != ERR_SUCCESS) {
			return nullptr;
		}
	}
	TrackedDevice device;
	devices.push_front(device);
	return &(devices.front());
}


cs_ret_code_t TrackedDevices::removeDevice() {
	uint16_t lowestTTL = 0xFFFF;
	auto iterToRemove = devices.end();
	for (auto iter = devices.begin(); iter != devices.end();) {
		if (iter->fieldsSet != ALL_FIELDS_SET) {
			iterToRemove = iter;
			break;
		}
		if (iter->data.data.timeToLiveMinutes <= lowestTTL) {
			lowestTTL = iter->data.data.timeToLiveMinutes;
			iterToRemove = iter;
		}
	}
//	if (lowestTTL == 0xFFFF) {
//		return ERR_NOT_AVAILABLE;
//	}
	devices.erase(iterToRemove);
	return ERR_SUCCESS;
}


bool TrackedDevices::hasAccess(TrackedDevice& device, uint8_t accessLevel) {
	return (!BLEutil::isBitSet(device.fieldsSet, BIT_POS_ACCESS_LEVEL)) || (EncryptionHandler::allowAccess(device.data.accessLevel, accessLevel));
}

void TrackedDevices::setAccessLevel(TrackedDevice& device, uint8_t accessLevel) {
	device.data.accessLevel = accessLevel;
	device.fieldsSet |= (1 << BIT_POS_ACCESS_LEVEL);
}

void TrackedDevices::setLocation(TrackedDevice& device, uint8_t locationId) {
	device.data.data.locationId = locationId;
	device.fieldsSet |= (1 << BIT_POS_LOCATION);
}

void TrackedDevices::setProfile(TrackedDevice& device, uint8_t profileId) {
	device.data.data.profileId = profileId;
	device.fieldsSet |= (1 << BIT_POS_PROFILE);
}

void TrackedDevices::setRssiOffset(TrackedDevice& device, int8_t rssiOffset) {
	device.data.data.rssiOffset = rssiOffset;
	device.fieldsSet |= (1 << BIT_POS_RSSI_OFFSET);
}

void TrackedDevices::setFlags(TrackedDevice& device, uint8_t flags) {
	device.data.data.flags = flags;
	device.fieldsSet |= (1 << BIT_POS_FLAGS);
}

void TrackedDevices::setDevicetoken(TrackedDevice& device, uint8_t* deviceToken) {
	memcpy(device.data.data.deviceToken, deviceToken, sizeof(device.data.data.deviceToken));
	device.fieldsSet |= (1 << BIT_POS_DEVICE_TOKEN);
}

void TrackedDevices::setTTL(TrackedDevice& device, uint16_t ttlMinutes) {
	device.data.data.timeToLiveMinutes = ttlMinutes;
	device.fieldsSet |= (1 << BIT_POS_TTL);
}

void TrackedDevices::sendLocation(TrackedDevice& device) {
	TYPIFY(EVT_PROFILE_LOCATION) eventData;
	eventData.profileId = profileId;
	eventData.locationId = locationId;
	eventData.fromMesh = false;
	event_t event(CS_TYPE::EVT_PROFILE_LOCATION, eventData, sizeof(eventData));
	EventDispatcher::getInstance().dispatch(event);
}

void TrackedDevices::handleEvent(event_t& evt) {
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

}
