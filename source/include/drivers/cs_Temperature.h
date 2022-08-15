/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

//#include "nrf_soc.h"

#include <ble/cs_Nordic.h>

#include "util/cs_BleError.h"

/** Get temperature reading from the softdevice
 *
 * The received value from the softdevice is in
 * 0.25°C steps, so in order to get it in °C, the
 * value is divided by 4
 */
inline int32_t getTemperature() {
	int32_t temperature;
	uint32_t nrfCode = sd_temp_get(&temperature);
	// This function only has success as return code.
	APP_ERROR_CHECK(nrfCode);

	temperature = (temperature / 4);
	return temperature;
}
