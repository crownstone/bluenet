/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Sep 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstdint>

#include <cfg/cs_DeviceTypes.h>

namespace BLEpp {

class DoBotsManufac {

private:
	uint8_t _deviceType;

public:
	DoBotsManufac(uint8_t deviceType) {
		_deviceType = deviceType;
	};

	virtual ~DoBotsManufac() {};

	uint8_t size() {
		return 1;
	}

	/* Serializes the object to a byte array
	 *
	 * @array pointer to the preallocated byte array where the
	 *   data should be copied into. Use <size> to get the required
	 *   length of the array
	 *
	 */
	void toArray(uint8_t* array);
};

} /* namespace BLEpp */
