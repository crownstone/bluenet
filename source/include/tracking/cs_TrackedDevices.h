/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 7, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <cstdint>

/**
 * Class that keeps up devices to be tracked.
 *
 * Alternative to dynamic background broadcasts (no heartbeat required),
 * or to no background broadcasts at all (heartbeats required).
 *
 * - Register devices: each device has a unique ID, and registers a unique token.
 * - Make sure device tokens expire.
 * - Cache the data of the devices (profile, location, flags, etc).
 * - Handle scans with device token only as data, and dispatch background broadcast event, by adding cached data.
 * - Handle device heartbeats, which are treated as if the device was scanned, by dispatching profile location event.
 */
class TrackedDevices: public EventListener {
public:
	TrackedDevices();

	/**
	 * Init class
	 *
	 * Register as event listener.
	 */
	void init();

	/**
	 * Handle events.
	 */
	void handleEvent(event_t& evt) override;

	/**
	 * Maximum number of registered tracked devices.
	 */
	static const uint8_t MAX_TRACKED_DEVICES = 20;

	/**
	 * After N minutes not hearing anything from the device, the location ID will be set to 0 (in sphere).
	 * This prevents sending out old locations.
	 */
	static const uint8_t LOCATION_ID_TTL_MINUTES = 5;

	/**
	 * Max heartbeat TTL in minutes.
	 * Make sure that it's not larger than what fits in location id timeout.
	 */
	static const uint16_t HEARTBEAT_TTL_MINUTES_MAX = 60;

private:
	static const uint16_t TICKS_PER_SECOND = (1000 / TICK_INTERVAL_MS);
	static const uint16_t TICKS_PER_MINUTES = (60 * 1000 / TICK_INTERVAL_MS);

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

	struct __attribute__((packed)) TrackedDevice {
		uint8_t fieldsSet = 0;
		uint8_t locationIdTTLMinutes = LOCATION_ID_TTL_MINUTES;
		uint8_t heartbeatTTLMinutes = 0;
		internal_register_tracked_device_packet_t data;
	};

	uint16_t ticksLeftSecond = TICKS_PER_SECOND;
	uint16_t ticksLeftMinute = TICKS_PER_MINUTES;

	/**
	 * List of all tracked devices.
	 *
	 * Device ID should be unique.
	 */
	TrackedDevice _devices[MAX_TRACKED_DEVICES];

	uint8_t _deviceListSize = 0;

	/**
	 * Whether there has been a successful sync of tracked devices.
	 *
	 * For now, this means just getting a list of devices from another crownstone, after boot.
	 */
	bool _deviceListIsSynced = false;

	/**
	 * When syncing, the remote crownstone will tell how many devices there are.
	 * This number is cached in this variable, so we know when we're synced.
	 */
	uint8_t _expectedDeviceListSize = 0xFF;

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

	cs_ret_code_t handleRegister(internal_register_tracked_device_packet_t& packet);
	cs_ret_code_t handleUpdate(internal_update_tracked_device_packet_t& packet);
	void handleMeshRegister(TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)& packet);
	void handleMeshToken(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)& packet);
	void handleMeshListSize(TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)& packet);
	void handleScannedDevice(adv_background_parsed_v1_t& packet);
	cs_ret_code_t handleHeartbeat(internal_tracked_device_heartbeat_packet_t& packet);
	cs_ret_code_t handleHeartbeat(TrackedDevice& device, uint8_t locationId, uint8_t ttlMinutes, bool fromMesh);
	void handleMeshHeartbeat(TYPIFY(EVT_MESH_TRACKED_DEVICE_HEARTBEAT)& packet);

	/**
	 * Return true when given access level is equal or higher than device access level.
	 */
	bool hasAccess(TrackedDevice& device, uint8_t accessLevel);

	/**
	 * Returns true when device has a valid TTL, that didn't expire yet.
	 */
	bool isValidTTL(TrackedDevice& device);

	/**
	 * Returns true when no other device has this token.
	 */
	bool isTokenOkToSet(TrackedDevice& device, uint8_t* deviceToken, uint8_t size);

	void print(TrackedDevice& device);

	/**
	 * A minute has passed.
	 *
	 * Decrease TTL of all devices by 1.
	 * Decrease location timeout of all device by 1.
	 * Decrease heartbeat TTL.
	 */
	void tickMinute();

	/**
	 * Send location of devices with non-timed out heartbeat.
	 */
	void tickSecond();

	/**
	 * Returns true when all fields are set.
	 */
	bool allFieldsSet(TrackedDevice& device);

	/**
	 * Check if tracked device list is synced yet.
	 */
	void checkSynced();


	void setAccessLevel(TrackedDevice& device, uint8_t accessLevel);
	void setLocation(TrackedDevice& device, uint8_t locationId);
	void setProfile(TrackedDevice& device, uint8_t profileId);
	void setRssiOffset(TrackedDevice& device, int8_t rssiOffset);
	void setFlags(TrackedDevice& device, uint8_t flags);
	void setDevicetoken(TrackedDevice& device, uint8_t* deviceToken, uint8_t size);
	void setTTL(TrackedDevice& device, uint16_t ttlMinutes);

	/**
	 * Send background adv event.
	 *
	 * Checks if all fields are set.
	 */
	void sendBackgroundAdv(TrackedDevice& device, uint8_t* macAddress, int8_t rssi);

	/**
	 * Send profile location to event dispatcher.
	 *
	 * Checks if all fields are set.
	 * Set fromMesh to true when location is based on a mesh message.
	 * Set simulated to true when the location is from the heartbeat TTL, not from a command or broadcast.
	 */
	void sendHeartbeatLocation(TrackedDevice& device, bool fromMesh, bool simulated);

	/**
	 * Send the heartbeat msg to the mesh.
	 */
	void sendHeartbeatToMesh(TrackedDevice& device);

	/**
	 * Send tracked device register msg to mesh.
	 */
	void sendRegisterToMesh(TrackedDevice& device);

	/**
	 * Send tracked device token msg to mesh.
	 */
	void sendTokenToMesh(TrackedDevice& device);

	/**
	 * Send tracked devices list size to mesh.
	 */
	void sendListSizeToMesh(uint8_t deviceCount);

	/**
	 * Send all tracked devices to mesh.
	 */
	void sendDeviceList();
};


