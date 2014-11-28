/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 26, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef CS_CHARAC_CONFIG_H_
#define CS_CHARAC_CONFIG_H_

enum {
	PWM_UUID                                 = 0x1,
	VOLTAGE_CURVE_UUID                       = 0x2,
	POWER_CONSUMPTION_UUID                   = 0x3,
	CURRENT_LIMIT_UUID                       = 0x4,
};

enum {
	TEMPERATURE_UUID                         = 0x1,
	CHANGE_NAME_UUID                         = 0x2,
};

struct CharacteristicStatus {
	uint8_t UUID;
	bool enabled;
};

typedef struct CharacteristicStatus CharacteristicStatusT;

#endif // CS_CHARAC_CONFIG_H_

