/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

enum EventType {
	Configuration = 0x000,
	General       = 0x100
};

// for Configuration event type see cs_ConfigHelper.h

enum GeneralEventType {
	EVT_POWER_ON = General,
	EVT_POWER_OFF,
	EVT_ALL
};

