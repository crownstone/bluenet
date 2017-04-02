/**
 * @file
 * Manufacturing data.
 *
 * @authors Crownstone Team
 * @copyright Crownstone B.V.
 * @date Sep 23, 2015
 * @license LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>

#include <cfg/cs_DeviceTypes.h>

/** CrownstoneManufacturer defines the different types of developed devices for within the advertisement packets.
 */
class CrownstoneManufacturer {

private:
	union {
		struct {
			uint8_t deviceType;
		} _params;
		uint8_t _buffer[sizeof(_params)];
	};

public:
	CrownstoneManufacturer() {
		_params.deviceType = DEVICE_UNDEF;
	};

	CrownstoneManufacturer(uint8_t deviceType) {
		_params.deviceType = deviceType;
	};

	virtual ~CrownstoneManufacturer() {};

	uint8_t size() {
		return 1;
	}

	uint8_t* getArray() {
		return _buffer;
	}

	uint8_t getDeviceType() {
		return _params.deviceType;
	}

	/** Serializes the object to a byte array
	 *
	 * @array pointer to the preallocated byte array where the
	 *   data should be copied into. Use <size> to get the required
	 *   length of the array
	 *
	 */
	void toArray(uint8_t* array);

	void parse(uint8_t* array, uint16_t len);
};

