/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 15, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cs_GpRegRetConfig.h>

#include <cstdint>

/**
 * Class to write and read the GPREGRET.
 *
 * Only to be used when SoftDevice is enabled.
 *
 * See cs_GpRegRetConfig.h for more info.
 */
class GpRegRet {
public:
	enum GpRegRetFlag {
		FLAG_BROWNOUT          = CS_GPREGRET_FLAG_BROWNOUT,
		FLAG_DFU               = CS_GPREGRET_FLAG_DFU_RESET,
		FLAG_STORAGE_RECOVERED = CS_GPREGRET_FLAG_STORAGE_RECOVERED
	};

	enum GpRegRetId { GPREGRET = 0, GPREGRET2 = 1 };

	/**
	 * Get the GPREGRET value.
	 *
	 * @param[in] registerId
	 */
	static uint32_t getValue(GpRegRetId registerId = GpRegRetId::GPREGRET);

	/**
	 * Clear all GPREGRET bits.
	 */
	static void clearAll();

	/**
	 * Clear counter.
	 */
	static void clearCounter();

	/**
	 * Get the current counter value.
	 */
	static uint32_t getCounter();

	/**
	 * Set counter to a given value.
	 */
	static void setCounter(uint32_t value);

	/**
	 * Clear all flags.
	 */
	static void clearFlags();

	/**
	 * Set a flag.
	 */
	static void setFlag(GpRegRetFlag flag);

	/**
	 * Clear a flag.
	 */
	static void clearFlag(GpRegRetFlag flag);

	/**
	 * Returns true when flag is set.
	 */
	static bool isFlagSet(GpRegRetFlag flag);

private:
	static void printRegRet();
};
