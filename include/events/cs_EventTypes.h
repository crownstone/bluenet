/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

/**
 * Event types.
 */
enum EventType {
	Configuration_Base = 0x000,
	StateVar_Base      = 0x080,
	General_Base       = 0x100,
};

//! for Configuration event type see cs_ConfigHelper.h

/**
 * General event types.
 */
enum GeneralEventType {
	EVT_POWER_OFF = General_Base,
	EVT_POWER_ON,
	EVT_ENV_TEMP_LOW,
	EVT_ENV_TEMP_HIGH,
	EVT_ADVERTISEMENT_PAUSE,
	EVT_ADVERTISEMENT_RESUME,
	EVT_SCANNER_START,
	EVT_SCANNER_STOP,
	EVT_SCANNED_DEVICES,
	EVT_POWER_CURVE,
	EVT_POWER_CONSUMPTION,
	EVT_ENABLED_MESH,
	EVT_ENABLED_ENCRYPTION,
	EVT_ENABLED_IBEACON,
	EVT_CHARACTERISTICS_UPDATED,
	EVT_TRACKED_DEVICES,
	EVT_TIME_UPDATED,
	EVT_SCHEDULE_ENTRIES_UPDATED,
	EVT_BLE_EVENT,
	EVT_STATE_VARS_NOTIFICATION,
	EVT_ALL = 0xFFFF
};

