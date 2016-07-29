/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Sep 23, 2015
 * License: LGPLv3+
 */
#include <ble/cs_DoBotsManufac.h>

void DoBotsManufac::toArray(uint8_t* array) {
	*array = _params.deviceType;
}

void DoBotsManufac::parse(uint8_t* array, uint16_t len) {

	_params.deviceType = *array;

}
