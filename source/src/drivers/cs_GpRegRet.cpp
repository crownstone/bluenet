/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 15, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_GpRegRet.h>
#include <drivers/cs_Serial.h>
#include <util/cs_BleError.h>

uint32_t GpRegRet::getValue(GpRegRetId registerId) {
	uint32_t gpregret;
	sd_power_gpregret_get(registerId, &gpregret);
	return gpregret;
}

void GpRegRet::clearAll() {
	uint32_t retCode;
	uint32_t mask = 0xFFFFFFFF;
	retCode = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	APP_ERROR_CHECK(retCode);

	// This flag is stored in GPREGRET2.
	clearFlag(GpRegRetFlag::FLAG_STORAGE_RECOVERED);
}

void GpRegRet::clearCounter() {
	uint32_t retCode;
	uint32_t mask = CS_GPREGRET_COUNTER_MASK;
	retCode = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	APP_ERROR_CHECK(retCode);
}

void GpRegRet::setCounter(uint32_t value) {
	if (value > CS_GPREGRET_COUNTER_MAX) {
		LOGe("Value too high: %u", value);
		value = CS_GPREGRET_COUNTER_MAX;
	}
	uint32_t retCode;
	clearCounter();
	retCode = sd_power_gpregret_set(GpRegRetId::GPREGRET, value);
	APP_ERROR_CHECK(retCode);
}

void GpRegRet::clearFlags() {
	uint32_t retCode;
	uint32_t mask = CS_GPREGRET_COUNTER_MASK;
	mask = ~mask;
	retCode = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	APP_ERROR_CHECK(retCode);

	// This flag is stored in GPREGRET2.
	clearFlag(GpRegRetFlag::FLAG_STORAGE_RECOVERED);
}

void GpRegRet::setFlag(GpRegRetFlag flag) {
	uint32_t retCode;
	if (flag == FLAG_STORAGE_RECOVERED) {
		retCode = sd_power_gpregret_set(GpRegRetId::GPREGRET2, flag);
	}
	else {
		retCode = sd_power_gpregret_set(GpRegRetId::GPREGRET, flag);
	}
	APP_ERROR_CHECK(retCode);
}

void GpRegRet::clearFlag(GpRegRetFlag flag) {
	uint32_t retCode;
	uint32_t mask = flag;
	if (flag == FLAG_STORAGE_RECOVERED) {
		retCode = sd_power_gpregret_clr(GpRegRetId::GPREGRET2, mask);
	}
	else {
		retCode = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	}
	APP_ERROR_CHECK(retCode);
}

bool GpRegRet::isFlagSet(GpRegRetFlag flag) {
	if (flag == FLAG_STORAGE_RECOVERED) {
		uint32_t gpregret = getValue(GpRegRetId::GPREGRET2);
		return (gpregret & flag);
	}
	else {
		uint32_t gpregret = getValue(GpRegRetId::GPREGRET);
		return (gpregret & flag);
	}
}
