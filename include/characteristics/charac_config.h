/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 26, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef CS_CHARAC_CONFIG_H_
#define CS_CHARAC_CONFIG_H_

// TODO Dominik: how about moving the Service UUIDs here too
//   or at least make a similar config file?

enum {
	PWM_UUID                                = 0x1,
	CURRENT_GET_UUID                        = 0x2,
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
};

enum {
	RSSI_UUID                               = 0x1,
	PERSONAL_THRESHOLD_UUID                 = 0x2,
	SCAN_DEVICE_UUID                        = 0x3,
	LIST_DEVICE_UUID                        = 0x4,
};

#endif // CS_CHARAC_CONFIG_H_

