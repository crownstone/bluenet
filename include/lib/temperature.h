/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 23, 2014
 * License: LGPLv3+
 */

#ifndef TEMPERATURE_HPP_
#define TEMPERATURE_HPP_

#include "nrf_soc.h"
#include "ble_error.h"

#include "log.h"

/*
 * get temperature from softdevice
 */
inline int32_t getTemperature() {
	int32_t temperature;
	uint32_t err_code;

	err_code = sd_temp_get(&temperature);
	APP_ERROR_CHECK(err_code);

//	LOG_DEBUG("raw temp: %d", temperature);
//	LOG_INFO("temp: %d", temperature / 4);

	temperature = (temperature / 4);

	return temperature;
}

#endif /* TEMPERATURE_HPP_ */
