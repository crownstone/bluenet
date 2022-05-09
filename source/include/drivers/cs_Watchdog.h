/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 14, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <ipc/cs_IpcRamData.h>

/**
 * Class that implements the watchdog timer.
 *
 * - Once started, keep kicking it to prevent a timeout.
 * - Reboot on timeout.
 * - When rebooted by timeout, RESETREAS will have the DOG bit set.
 */
class Watchdog {
public:
	/**
	 * Init the watchdog timer.
	 */
	static void init();

	/**
	 * Start the watchdog timer.
	 */
	static void start();

	/**
	 * Restart the watchdog timer.
	 */
	static void kick();

	/**
	 * We write an "operating state" towards the watchdog that can write this to some part of memory that is preserved
	 * across reboots. This only happens when the watchdog is triggered.
	 * It is written to index IPC_INDEX_WATCHDOG_INFO in IPC RAM.
	 */
	static void setOperatingStateToWriteOnTimeout(uint8_t* data, uint8_t dataSize);

	/**
	 * Clear the "operating state".
	 */
	static void clearOperatingStateToWriteOnTimeout();

	/**
	 * Get operating state of previous timeout.
	 */
	static void getOperatingStateOfPreviousTimeout(uint8_t* data, uint8_t& dataSize);

	static uint8_t* getData();

	static uint8_t getDataSize();

	static bool hasDataToWrite();

private:
	static nrfx_wdt_channel_id _channelId;

	static uint8_t _data[BLUENET_IPC_RAM_DATA_ITEM_SIZE];

	static uint8_t _dataSize;

	static bool _dataToWrite;
};
