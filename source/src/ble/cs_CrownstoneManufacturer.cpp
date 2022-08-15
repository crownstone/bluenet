/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <ble/cs_CrownstoneManufacturer.h>

void CrownstoneManufacturer::toArray(uint8_t* array) {
	*array = _params.deviceType;
}

void CrownstoneManufacturer::parse(uint8_t* array, uint16_t len) {

	_params.deviceType = *array;
}
