/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2016
 * License: LGPLv3+
 */
#include <ble/cs_ServiceData.h>

#include <protocol/cs_StateTypes.h>
#include <protocol/cs_ConfigTypes.h>

ServiceData::ServiceData() : EventListener(EVT_ALL)
{
	EventDispatcher::getInstance().addListener(this);
	memset(_array, 0, sizeof(_array));
};

void ServiceData::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
		switch(evt) {
		case CONFIG_CROWNSTONE_ID: {
			updateCrownstoneId(*(uint16_t*)p_data);
			break;
		}
		case STATE_SWITCH_STATE: {
			updateSwitchState(*(uint8_t*)p_data);
			break;
		}
//		case STATE_ACCUMULATED_ENERGY: {
//			updateAccumulatedEnergy(*(uint32_t*)p_data);
//			break;
//		}
//		case STATE_POWER_USAGE: {
//			updatePowerUsage(*(uint32_t*)p_data);
//			break;
//		}
		}
	}
