/**
 * Author: Alex de Mulder
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 5 Jan., 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <drivers/cs_RNG.h>

#include <ble/cs_Nordic.h>
#include <util/cs_BleError.h>

//#include "nrf_soc.h"
//#include "nrf_sdm.h"
//
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//	#include "app_error.h"
//#ifdef __cplusplus
//}
//#endif

// convert uint8_t to uint32_t
typedef union {
	uint8_t a[4];
	uint32_t b;
} conv8_32;

// convert from uint8_t to uint16_t and back
typedef union {
	uint8_t a[2];
	uint16_t b;
} conv8_16;


RNG::RNG() {};

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
