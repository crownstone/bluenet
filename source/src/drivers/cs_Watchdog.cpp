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
bool Watchdog::_dataToWrite;
uint8_t Watchdog::_data[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
uint8_t Watchdog::_dataSize;

void handleTimeout() {
	if (Watchdog::hasDataToWrite()) {
		setRamData(IPC_INDEX_WATCHDOG_INFO, Watchdog::getData(), Watchdog::getDataSize());
	}
}

void Watchdog::init() {
	_dataToWrite = false;
	_dataSize = 0;

	nrfx_wdt_config_t config;
	config.behaviour = (nrf_wdt_behaviour_t)NRFX_WDT_CONFIG_BEHAVIOUR;
	config.reload_value = CS_WATCHDOG_TIMEOUT_MS;
#if NORDIC_SDK_VERSION >= 17
	config.interrupt_priority = CS_WATCHDOG_PRIORITY;
#endif
	LOGi("Init watchdog with %u ms timeout", CS_WATCHDOG_TIMEOUT_MS);

	uint32_t err_code;

	err_code = nrfx_wdt_init(&config, &handleTimeout);
//	err_code = nrfx_wdt_init(&config, NULL);
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
	_dataSize = dataSize;
	memcpy(_data, data, dataSize);
	_dataToWrite = true;
}

void Watchdog::clearOperatingStateToWriteOnTimeout() {
	_dataToWrite = false;
}

void Watchdog::clearOperatingStateOfPreviousTimeout() {
	clearRamData(IPC_INDEX_WATCHDOG_INFO);
}

void Watchdog::getOperatingStateOfPreviousTimeout(uint8_t *data, uint8_t & dataSize) {
	getRamData(IPC_INDEX_WATCHDOG_INFO, data, dataSize, &dataSize);
}

uint8_t* Watchdog::getData() {
	return _data;
}

uint8_t Watchdog::getDataSize() {
	return _dataSize;
}

bool Watchdog::hasDataToWrite() {
	return _dataToWrite;
}
