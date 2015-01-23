#include "nrf_soc.h"
#include "nrf_sdm.h"

#ifdef __cplusplus
extern "C" {
#endif
	#include "app_error.h"
#ifdef __cplusplus
}
#endif

#ifndef _RNG_H
#define _RNG_H

class RNG{
public:
	uint8_t _randomBytes[4] = {0,0,0,0};

	RNG();
	uint32_t getRandom32();
	uint16_t getRandom16();
	uint8_t  getRandom8();

};

#endif
