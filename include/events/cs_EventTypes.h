/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

enum EventType {
	Configuration_Base = 0x000,
	General_Base       = 0x100
};

//! for Configuration event type see cs_ConfigHelper.h

enum GeneralEventType {
	EVT_POWER_OFF = General_Base,
	EVT_POWER_ON,
	EVT_ENV_TEMP_LOW,
	EVT_ENV_TEMP_HIGH,
	EVT_ADVERTISEMENT_PAUSE,
	EVT_ADVERTISEMENT_RESUME,
	EVT_SCANNER_START,
	EVT_SCANNER_STOP,
	EVT_ALL = 0xFFFF
};

