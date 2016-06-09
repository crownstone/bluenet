/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2016
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>

#include <cstring>

class ServiceData : EventListener {

public:
	ServiceData();

//	void updatePowerUsage(uint32_t powerUsage) {
//		_params.powerUsage = powerUsage;
//	}

//	void updateAccumulatedEnergy(uint32_t accumulatedEnergy) {
//		_params.accumulatedEnergy = accumulatedEnergy;
//	}

	void updateCrownstoneId(uint16_t crownstoneId) {
		_params.crownstoneId = crownstoneId;
	}

	void updateCrownstoneStateId(uint16_t crownstoneStateId) {
		_params.crownstoneStateId = crownstoneStateId;
	}

	void updateSwitchState(uint8_t switchState) {
		_params.switchState = switchState;
	}

	void updateEventBitmask(uint8_t bitmask) {
		_params.eventBitmask = bitmask;
	}

	void updateEventBitmask(uint8_t bit, bool set) {
		if (set) {
			_params.eventBitmask |= 1 < bit;
		} else {
			_params.eventBitmask &= 0 < bit;
		}
	}

	uint8_t* getArray() {
		return _array;
	}

	uint16_t getArraySize() {
		return sizeof(_params);
	}

private:

	union {
		struct __attribute__((packed)) {
			uint16_t crownstoneId;
			uint16_t crownstoneStateId;
			uint8_t switchState;
			uint8_t eventBitmask;
			uint16_t reserved;
//			uint32_t powerUsage;
//			uint32_t accumulatedEnergy;
		} _params;
		uint8_t _array[sizeof(_params)] = {};
	};

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);


};

