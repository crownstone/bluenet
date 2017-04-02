/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Sep 23, 2015
 * License: LGPLv3+
 */
#include <ble/cs_CrownstoneManufacturer.h>

void CrownstoneManufacturer::toArray(uint8_t* array) {
	*array = _params.deviceType;
}

void CrownstoneManufacturer::parse(uint8_t* array, uint16_t len) {

	_params.deviceType = *array;

}
