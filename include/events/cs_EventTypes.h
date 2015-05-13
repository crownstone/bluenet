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

// for Configuration event type see cs_ConfigHelper.h

enum GeneralEventType {
	EVT_POWER_ON = General_Base,
	EVT_POWER_OFF,
	EVT_ALL
};

