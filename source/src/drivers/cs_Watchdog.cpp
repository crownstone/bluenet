/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 14, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Config.h>
#include <drivers/cs_GpRegRet.h>
#include <drivers/cs_Watchdog.h>
#include <logging/cs_Logger.h>
#include <util/cs_BleError.h>

// ============== Static members ==============
nrfx_wdt_channel_id Watchdog::_channelId;

void Watchdog::init() {
	nrfx_wdt_config_t config;
	config.behaviour    = (nrf_wdt_behaviour_t)NRFX_WDT_CONFIG_BEHAVIOUR;
	config.reload_value = CS_WATCHDOG_TIMEOUT_MS;
	//	config.interrupt_priority = CS_WATCHDOG_PRIORITY;

	LOGi("Init watchdog with %u ms timeout", CS_WATCHDOG_TIMEOUT_MS);

	uint32_t err_code;

	//	err_code = nrfx_wdt_init(&config, &handleTimeout);
	err_code = nrfx_wdt_init(&config, NULL);
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
