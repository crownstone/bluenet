/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jun 8, 2016
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventTypes.h>

/** State variable types
 *
 * Use in the characteristic to read and write state variables in <CommonService>.
 */
enum StateTypes {
	STATE_RESET_COUNTER = State_Base,     // 0x80 - 128
	STATE_SWITCH_STATE,                   // 0x81 - 129
	STATE_ACCUMULATED_ENERGY,             // 0x82 - 130
	STATE_POWER_USAGE,		              // 0x83 - 131
	STATE_TRACKED_DEVICES,                // 0x84 - 132
	STATE_SCHEDULE,                       // 0x85 - 133
	STATE_OPERATION_MODE,                 // 0x86 - 134
	STATE_TEMPERATURE,                    // 0x87 - 135
	STATE_TIME,                           // 0x88 - 136
	STATE_FACTORY_RESET,                  // 0x89 - 137
	STATE_LEARNED_SWITCHES,               // 0x8A - 138
	STATE_ERRORS,                         // 0x8B - 139
	STATE_TYPES
};



