/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2016
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>

class ServiceData : EventListener {

public:
	ServiceData() : EventListener(EVT_ALL) {
		EventDispatcher::getInstance().addListener(this);
		_params.crownstoneId = 1;
	};

	void updateSwitchState(uint8_t switchState) {
		_params.switchState = switchState;
	}

	void updateCurrentUsage(uint32_t powerConsumption) {
		_params.powerConsumption = powerConsumption;
	}

//	void updateAccumulatedPower(uint64_t accumulatedPower) {
//		_params.accumulatedPower = accumulatedPower;
//	}

	void updateCrownstoneId(uint32_t crownstoneId) {
		_params.crownstoneId = crownstoneId;
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
			uint8_t switchState : 8;
			uint32_t powerConsumption : 32;
//			uint64_t accumulatedPower : 64;
			uint32_t crownstoneId : 24;
		} _params;
		uint8_t _array[sizeof(_params)] = {};
	};

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);


};

