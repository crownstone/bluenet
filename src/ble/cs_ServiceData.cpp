/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2016
 * License: LGPLv3+
 */
#include <ble/cs_ServiceData.h>

#include <storage/cs_State.h>

void ServiceData::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
		switch(evt) {
		case STATE_SWITCH_STATE: {
			_params.switchState = *(uint8_t*)p_data;
			break;
		}
//		case SV_ACCUMULATED_POWER: {
//			_params.accumulatedPower = *(uint64_t*)p_data;
//			break;
//		}
		case STATE_POWER_USAGE: {
			_params.powerConsumption = *(uint32_t*)p_data;
			break;
		}
		}
	}
