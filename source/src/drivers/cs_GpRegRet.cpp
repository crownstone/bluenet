/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 15, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_GpRegRet.h>
#include <logging/cs_Logger.h>
#include <util/cs_BleError.h>

// Set true to enable debug logs.
#define CS_GPREGRET_DEBUG false

#if CS_GPREGRET_DEBUG == true
#define LOGGpRegRetDebug LOGd
#else
#define LOGGpRegRetDebug LOGnone
#endif

uint32_t GpRegRet::getValue(GpRegRetId registerId) {
	uint32_t gpregret;
	sd_power_gpregret_get(registerId, &gpregret);
	return gpregret;
}

void GpRegRet::clearAll() {
	LOGGpRegRetDebug("clearAll");
	uint32_t retCode;
	uint32_t mask = 0xFFFFFFFF;
	retCode       = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	APP_ERROR_CHECK(retCode);
	printRegRet();
}

void GpRegRet::clearCounter() {
	LOGGpRegRetDebug("clearCounter");
	uint32_t retCode;
	uint32_t mask = CS_GPREGRET_COUNTER_MASK;
	retCode       = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	APP_ERROR_CHECK(retCode);
	printRegRet();
}

void GpRegRet::setCounter(uint32_t value) {
	LOGGpRegRetDebug("setCounter %u", value);
	if (value > CS_GPREGRET_COUNTER_MAX) {
		LOGe("Value too high: %u", value);
		value = CS_GPREGRET_COUNTER_MAX;
	}
	uint32_t retCode;
	clearCounter();
	retCode = sd_power_gpregret_set(GpRegRetId::GPREGRET, value);
	APP_ERROR_CHECK(retCode);
	printRegRet();
}

void GpRegRet::clearFlags() {
	LOGGpRegRetDebug("clearFlags");
	uint32_t retCode;
	uint32_t mask = CS_GPREGRET_COUNTER_MASK;
	mask          = ~mask;
	retCode       = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	APP_ERROR_CHECK(retCode);
	printRegRet();
}

void GpRegRet::setFlag(GpRegRetFlag flag) {
	LOGGpRegRetDebug("setFlag %u", flag);
	uint32_t retCode;
	retCode = sd_power_gpregret_set(GpRegRetId::GPREGRET, flag);
	APP_ERROR_CHECK(retCode);
	printRegRet();
}

void GpRegRet::clearFlag(GpRegRetFlag flag) {
	LOGGpRegRetDebug("clearFlag %u", flag);
	uint32_t retCode;
	uint32_t mask = flag;
	retCode       = sd_power_gpregret_clr(GpRegRetId::GPREGRET, mask);
	APP_ERROR_CHECK(retCode);
	printRegRet();
}

bool GpRegRet::isFlagSet(GpRegRetFlag flag) {
	LOGGpRegRetDebug("isFlagSet %u", flag);
	uint32_t gpregret = getValue(GpRegRetId::GPREGRET);
	return (gpregret & flag);
}

void GpRegRet::printRegRet() {
#if CS_GPREGRET_DEBUG == true
	LOGGpRegRetDebug("GPRegRet: %u %u", getValue(GPREGRET), getValue(GPREGRET2));
#endif
}
