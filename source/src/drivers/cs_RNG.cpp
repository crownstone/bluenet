/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 5 Jan., 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_RNG.h>

#include <ble/cs_Nordic.h>
#include <util/cs_BleError.h>

RNG::RNG() {};

void RNG::fillBuffer(uint8_t* buffer, uint8_t length) {
	uint8_t bytes_available = 0;
	uint32_t err_code;
	while (bytes_available < length) {
		err_code = sd_rand_application_bytes_available_get(&bytes_available);
		APP_ERROR_CHECK(err_code);
	}
	err_code = sd_rand_application_vector_get(buffer, length);
	APP_ERROR_CHECK(err_code);
};

uint32_t RNG::getRandom32() {
	uint8_t bytes_available = 0;
	uint32_t err_code;
	while (bytes_available < 4) {
		err_code = sd_rand_application_bytes_available_get(&bytes_available);
		APP_ERROR_CHECK(err_code);
	}


	err_code = sd_rand_application_vector_get(_randomBytes, 4);
	APP_ERROR_CHECK(err_code);

	conv8_32 converter;
	converter.a[0] = _randomBytes[0];
	converter.a[1] = _randomBytes[1];
	converter.a[2] = _randomBytes[2];
	converter.a[3] = _randomBytes[3];

	return converter.b;
};

uint16_t RNG::getRandom16() {
	uint8_t bytes_available = 0;
	uint32_t err_code;
	while (bytes_available < 2) {
		err_code = sd_rand_application_bytes_available_get(&bytes_available);
		APP_ERROR_CHECK(err_code);
	}


	err_code = sd_rand_application_vector_get(_randomBytes, 2);
	APP_ERROR_CHECK(err_code);

	conv8_16 converter;

	converter.a[0] = _randomBytes[0];
	converter.a[1] = _randomBytes[1];

	return converter.b;
};

uint8_t RNG::getRandom8() {
	uint8_t bytes_available = 0;
	uint32_t err_code;
	while (bytes_available < 1) {
		err_code = sd_rand_application_bytes_available_get(&bytes_available);
		APP_ERROR_CHECK(err_code);
	}


	err_code = sd_rand_application_vector_get(_randomBytes, 1);
	APP_ERROR_CHECK(err_code);
	return _randomBytes[0];
};
