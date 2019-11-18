/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 2, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "events/cs_EventListener.h"
#include "ble/cs_Nordic.h"

struct __attribute__((__packed__)) t2t_entry_t {
	uint8_t address[BLE_GAP_ADDR_LEN];
	uint16_t score; // Score that decreases every tick, and increases when rssi is above threshold.
};

#define T2T_LIST_COUNT 3
#define T2T_SCORE_INC (2000 / TICK_INTERVAL_MS)
#define T2T_SCORE_THRESHOLD (3000 / TICK_INTERVAL_MS)
#define T2T_SCORE_MAX (5000 / TICK_INTERVAL_MS)
#define T2T_TIMEOUT_MS 1500

/**
 * Class that toggles the switch when a device, like a phone, is held close to the Crownstone.
 *
 * Receives data from event EVT_ADV_BACKGROUND_PARSED.
 * Checks if tap to toggle is enabled in that data.
 * Determines whether a device is considered to be close. Implemented as a leaking bucket:
 * - A score per MAC address is kept up.
 * - Each received background advertisement with an RSSI above threshold, adds to the score.
 * - Each tick the score is decreased.
 * - When going from below score threshold to above, a toggle is sent.
 * Makes sure there is some time between two toggles.
 * - Each time a toggle is sent, score additions will be blocked for a certain time.
 * Sends out CMD_SWITCH_TOGGLE to toggle switch.
 */
class TapToToggle : public EventListener {
public:
	static TapToToggle& getInstance() {
		static TapToToggle instance;
		return instance;
	}
	void handleEvent(event_t & event);

private:
	/**
	 * Whether tap to toggle is enabled on this crownstone.
	 */
	TYPIFY(CONFIG_TAP_TO_TOGGLE_ENABLED) enabled = CONFIG_TAP_TO_TOGGLE_ENABLED_DEFAULT;

	t2t_entry_t list[T2T_LIST_COUNT];
	/**
	 * Used to count down the timeout.
	 */
	uint16_t timeoutCounter = 0;

	/**
	 * RSSI threshold, above which score will be added.
	 */
	int8_t rssiThreshold = CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_DEFAULT;

	/**
	 * Score is increased with this value when rssi is above rssi threshold.
	 */
	uint8_t scoreIncrement = T2T_SCORE_INC;

	/**
	 * Threshold above which the toggle is triggered.
	 */
	uint8_t scoreThreshold = T2T_SCORE_THRESHOLD;

	/**
	 * Score can't be higher than this value.
	 */
	uint8_t scoreMax = T2T_SCORE_MAX;

	/**
	 * Minimal time in ticks between 2 tap to toggle events.
	 */
	uint8_t timeoutTicks = (T2T_TIMEOUT_MS / TICK_INTERVAL_MS);

	TapToToggle();

	/**
	 * Handle background advertisement.
	 *
	 * Checks if RSSI is above threshold.
	 * Checks if tap to toggle is enabled in the background advertisement.
	 * Checks if timeout is active.
	 * Adds to score.
	 * Triggers toggle.
	 */
	void handleBackgroundAdvertisement(adv_background_parsed_t* adv);

	/**
	 * Decrease score of each MAC address.
	 */
	void tick();
};
