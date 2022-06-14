/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 14, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Config.h>
#include <drivers/cs_GpRegRet.h>
#include <logging/cs_Logger.h>
#include <drivers/cs_Watchdog.h>
#include <util/cs_BleError.h>


// ============== Static members ==============
nrfx_wdt_channel_id Watchdog::_channelId;

/*
 * Can be used to write information towards the bootloader or the app across a warm boot.
 */
void handleTimeout() {
}

void Watchdog::init() {
	nrfx_wdt_config_t config;
	config.behaviour = (nrf_wdt_behaviour_t)NRFX_WDT_CONFIG_BEHAVIOUR;
	config.reload_value = CS_WATCHDOG_TIMEOUT_MS;
	config.interrupt_priority = CS_WATCHDOG_PRIORITY;
	LOGi("Init watchdog with %u ms timeout", CS_WATCHDOG_TIMEOUT_MS);

	uint32_t err_code;

	err_code = nrfx_wdt_init(&config, &handleTimeout);
	APP_ERROR_CHECK(err_code);

	err_code = nrfx_wdt_channel_alloc(&_channelId);
	APP_ERROR_CHECK(err_code);
}

void Watchdog::start() {
	LOGi("Start watchdog");
	nrfx_wdt_enable();
}

void Watchdog::kick() {
	nrfx_wdt_channel_feed(_channelId);
}

void Watchdog::setOperatingStateToWriteOnTimeout(uint8_t* data, uint8_t dataSize) {
	LOGv("Set microapp as dirty");
	setRamData(IPC_INDEX_WATCHDOG_INFO, data, dataSize);
}

void Watchdog::clearOperatingStateToWriteOnTimeout() {
	LOGv("Set microapp as clean");
	clearRamData(IPC_INDEX_WATCHDOG_INFO);
}

void Watchdog::clearOperatingStateOfPreviousTimeout() {
	clearRamData(IPC_INDEX_WATCHDOG_INFO);
}

void Watchdog::getOperatingStateOfPreviousTimeout(uint8_t *data, uint8_t & dataSize) {
	LOGv("Get microapp state of previous timeout");
	getRamData(IPC_INDEX_WATCHDOG_INFO, data, dataSize, &dataSize);
}

