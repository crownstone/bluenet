/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2016
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_Settings.h>
#include <storage/cs_State.h>

#include <cstring>

#define SERVICE_DATA_PROTOCOL_VERSION 1

class ServiceData : EventListener {

public:
	ServiceData();

	void updatePowerUsage(int32_t powerUsage) {
		_params.powerUsage = powerUsage;
	}

	void updateAccumulatedEnergy(int32_t accumulatedEnergy) {
		_params.accumulatedEnergy = accumulatedEnergy;
	}

	void updateCrownstoneId(uint16_t crownstoneId) {
		_params.crownstoneId = crownstoneId;
	}

	void updateCrownstoneStateId(uint16_t crownstoneStateId) {
//		_params.crownstoneStateId = crownstoneStateId;
	}

	void updateSwitchState(uint8_t switchState) {
		_params.switchState = switchState;
	}

	void updateEventBitmask(uint8_t bitmask) {
		_params.eventBitmask = bitmask;
	}

	void updateTemperature(int8_t temperature) {
		_params.temperature = temperature;
	}

	void updateEventBitmask(uint8_t bit, bool set) {
		if (set) {
			_params.eventBitmask |= 1 < bit;
		} else {
			_params.eventBitmask &= 0 < bit;
		}
	}

	uint8_t* getArray() {
		uint8_t opMode;
		State::getInstance().get(STATE_OPERATION_MODE, opMode);
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED) && !(opMode == OPERATION_MODE_SETUP)) {
			return _encryptedArray;
		}
		else {
			return _array;
		}

	}

	uint16_t getArraySize() {
		return sizeof(_params);
	}

private:

	union {
		struct __attribute__((packed)) {
			uint8_t protocolVersion;
			uint16_t crownstoneId;
			uint8_t switchState;
			uint8_t eventBitmask;
//			uint8_t reserved;
			int8_t temperature;
			int32_t powerUsage;
			int32_t accumulatedEnergy;
			uint8_t rand[3];
		} _params;
		uint8_t _array[sizeof(_params)] = {};
	};

	uint8_t _encryptedArray[sizeof(_params)];

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);


};

