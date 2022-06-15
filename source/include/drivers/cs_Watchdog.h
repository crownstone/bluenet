/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 14, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>

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

private:
	static nrfx_wdt_channel_id _channelId;
};
