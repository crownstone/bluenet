/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#pragma once

#include <cstdint>

#include "common/cs_Types.h"
#include "cfg/cs_Config.h"
#include "events/cs_EventListener.h"
#include "events/cs_EventDispatcher.h"

#include <ble/cs_Nordic.h>

struct __attribute__((__packed__)) setup_data_t {
	uint8_t        type;
	stone_id_t     id;
	uint8_t        adminKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        memberKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        guestKey[ENCRYPTION_KEY_LENGTH];
	uint32_t       meshAccessAddress;
	ble_uuid128_t  ibeaconUuid;
	uint16_t       ibeaconMajor;
	uint16_t       ibeaconMinor;
};

class Setup : EventListener {
public:
	// Gets a static singleton (no dynamic memory allocation)
	static Setup& getInstance() {
		static Setup instance;
		return instance;
	}

	ERR_CODE handleCommand(uint8_t* data, uint16_t size);

	// Handle events as EventListener
	void handleEvent(event_t & event);

private:
	Setup();

	bool _setupDone;
};
