/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <cstdint>
#include <events/cs_EventDispatcher.h>
#include <events/cs_EventListener.h>
#include <storage/cs_State.h>

struct __attribute__((__packed__)) setup_data_t {
	stone_id_t     stoneId;
	uint8_t        sphereId;
	uint8_t        adminKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        memberKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        basicKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        serviceDataKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        localizationKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        meshDeviceKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        meshAppKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        meshNetKey[ENCRYPTION_KEY_LENGTH];
	ble_uuid128_t  ibeaconUuid;
	uint16_t       ibeaconMajor;
	uint16_t       ibeaconMinor;
};

// All the state variables that are expected to be written to flash.
enum SetupConfigBit {
	SETUP_CONFIG_BIT_STONE_ID = 0,
	SETUP_CONFIG_BIT_SPHERE_ID,
	SETUP_CONFIG_BIT_ADMIN_KEY,
	SETUP_CONFIG_BIT_MEMBER_KEY,
	SETUP_CONFIG_BIT_BASIC_KEY,
	SETUP_CONFIG_BIT_SERVICE_DATA_KEY,
	SETUP_CONFIG_BIT_LOCALIZATION_KEY,
	SETUP_CONFIG_BIT_MESH_DEVICE_KEY,
	SETUP_CONFIG_BIT_MESH_APP_KEY,
	SETUP_CONFIG_BIT_MESH_NET_KEY,
	SETUP_CONFIG_BIT_IBEACON_UUID,
	SETUP_CONFIG_BIT_IBEACON_MAJOR,
	SETUP_CONFIG_BIT_IBEACON_MINOR,
	SETUP_CONFIG_BIT_SWITCH,
	SETUP_CONFIG_BIT_OPERATION_MODE,
	SETUP_CONFIG_NUM_BITS
};
#define SETUP_CONFIG_MASK_ALL ((1 << SETUP_CONFIG_NUM_BITS) - 1)


class Setup : EventListener {
public:
	// Gets a static singleton (no dynamic memory allocation)
	static Setup& getInstance() {
		static Setup instance;
		return instance;
	}

	cs_ret_code_t handleCommand(uint8_t* data, uint16_t size);

	// Handle events as EventListener
	void handleEvent(event_t & event);

private:
	Setup();

	// Used to check if all state variables are written to flash.
	uint32_t _successfullyStoredBitmask = 0;

	void onStorageDone(const CS_TYPE& type);
	void finalize();
};
