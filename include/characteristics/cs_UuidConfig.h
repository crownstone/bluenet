/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 26, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

// TODO Dominik: how about moving the Service UUIDs here too
//   or at least make a similar config file?

enum GeneralCharacteristics {
	PWM_UUID                                = 0x1,
	SAMPLE_CURRENT_UUID                     = 0x2,
	CURRENT_CURVE_UUID                      = 0x3,
	CURRENT_CONSUMPTION_UUID                = 0x4,
	CURRENT_LIMIT_UUID                      = 0x5,
};

enum {
	TEMPERATURE_UUID                        = 0x1,
	CHANGE_NAME_UUID                        = 0x2,
	DEVICE_TYPE_UUID                        = 0x3,
	ROOM_UUID                               = 0x4,
	FIRMWARE_UUID                           = 0x5,
	MESH_UUID                               = 0x6,
	SET_CONFIGURATION_UUID                  = 0x7,
	SELECT_CONFIGURATION_UUID               = 0x8,
	GET_CONFIGURATION_UUID                  = 0x9,
};

enum {
	RSSI_UUID                               = 0x1,
	TRACKED_DEVICE_UUID                     = 0x2,
	SCAN_DEVICE_UUID                        = 0x3,
	LIST_DEVICE_UUID                        = 0x4,
	TRACKED_DEVICE_LIST_UUID                = 0x5,
};

/* Configuration types
 *
 */
enum {
	CONFIG_NAME_UUID                        = 0x0,
	CONFIG_DEVICE_TYPE_UUID                 = 0x1,
	CONFIG_ROOM_UUID                        = 0x2,
	CONFIG_FLOOR_UUID                       = 0x3,
	CONFIG_TYPES
};
