/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 7, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <forward_list>

class TrackedDevices: public EventListener {
public:



	/**
	 * Handle events.
	 */
	void handleEvent(event_t& evt) override;

private:
	static const uint8_t MAX_TRACKED_DEVICES = 20;
	static const uint16_t TICKS_PER_MINUTES = 1000 / TICK_INTERVAL_MS * 60;

	enum TrackedDeviceFields {
		BIT_POS_ACCESS_LEVEL  = 0,
		BIT_POS_LOCATION      = 1,
		BIT_POS_PROFILE       = 2,
		BIT_POS_RSSI_OFFSET   = 3,
		BIT_POS_FLAGS         = 4,
		BIT_POS_DEVICE_TOKEN  = 5,
		BIT_POS_TTL           = 6,
	};
	static const uint8_t ALL_FIELDS_SET = 0x7F;

	struct TrackedDevice {
		uint8_t fieldsSet = 0;
		internal_register_tracked_device_packet_t data;
	};

	uint16_t ticksLeft = TICKS_PER_MINUTES;

	/**
	 * List of all tracked devices.
	 *
	 * Device ID should be unique.
	 */
	std::forward_list<TrackedDevice> devices;

	/**
	 * Find device with given ID, else add a new device with given ID.
	 *
	 * returns null if couldn't be added.
	 */
	TrackedDevice* findOrAdd(device_id_t deviceId);

	/**
	 * Find device with given id.
	 *
	 * returns null if it couldn't be found.
	 */
	TrackedDevice* find(device_id_t deviceId);

	/**
	 * Find device token.
	 *
	 * returns null if it couldn't be found.
	 */
	TrackedDevice* findToken(uint8_t* deviceToken, uint8_t size);

	/**
	 * Add device to list.
	 *
	 * returns null if couldn't be added.
	 */
	TrackedDevice* add();

	/**
	 * Remove a device from the list.
	 *
	 * Either a record with incomplete data, otherwise the oldest.
	 */
	cs_ret_code_t removeDevice();

	/**
	 *
	 */
	bool isValidTTL(TrackedDevice& device);


	cs_ret_code_t handleRegister(internal_register_tracked_device_packet_t& packet);
	cs_ret_code_t handleUpdate(internal_update_tracked_device_packet_t& packet);
	void handleMeshRegister(TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)& packet);
	void handleMeshToken(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)& packet);
	void handleScannedDevice(adv_background_parsed_v1_t& packet);

	bool hasAccess(TrackedDevice& device, uint8_t accessLevel);

	/**
	 * Returns false if another device already has this token.
	 */
	bool isTokenOkToSet(TrackedDevice& device, uint8_t* deviceToken, uint8_t size);

	bool allFieldsSet(TrackedDevice& device);
	void setAccessLevel(TrackedDevice& device, uint8_t accessLevel);
	void setLocation(TrackedDevice& device, uint8_t locationId);
	void setProfile(TrackedDevice& device, uint8_t profileId);
	void setRssiOffset(TrackedDevice& device, int8_t rssiOffset);
	void setFlags(TrackedDevice& device, uint8_t flags);
	void setDevicetoken(TrackedDevice& device, uint8_t* deviceToken, uint8_t size);
	void setTTL(TrackedDevice& device, uint16_t ttlMinutes);

	void decreaseTTL();

	/**
	 * Send profile location.
	 *
	 * Checks if all fields are set.
	 */
	void sendLocation(TrackedDevice& device);

	void sendRegisterToMesh(TrackedDevice& device);

	void sendTokenToMesh(TrackedDevice& device);

	/**
	 * Send the location of given device to mesh.
	 *
	 * Does not check if fields are set.
	 */
	void sendLocationToMesh(TrackedDevice& device);
};


