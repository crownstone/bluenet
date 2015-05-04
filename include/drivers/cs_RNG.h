/**
 * Author: Alex de Mulder
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 5 Jan., 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>

//#include "nrf_soc.h"
//#include "nrf_sdm.h"

//#ifdef __cplusplus
//extern "C" {
//#endif
//	#include "app_error.h"
//#ifdef __cplusplus
//}
//#endif

class RNG{
public:
	uint8_t _randomBytes[4] = {0,0,0,0};

	RNG();
	uint32_t getRandom32();
	uint16_t getRandom16();
	uint8_t  getRandom8();

};
