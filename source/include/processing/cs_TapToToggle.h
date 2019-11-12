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
#define T2T_SCORE_INC 4
#define T2T_SCORE_THRESHOLD 6
#define T2T_SCORE_MAX 10
#define T2T_TIMEOUT_MS 1500

class TapToToggle : public EventListener {
public:
	static TapToToggle& getInstance() {
		static TapToToggle instance;
		return instance;
	}
	void handleEvent(event_t & event);

private:
	t2t_entry_t list[T2T_LIST_COUNT];
	uint8_t timeoutCounter = 0; // Used to make sure toggle doesn't happen too quickly after each other.

	int8_t rssiThreshold = CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_DEFAULT;
	uint8_t scoreIncrement = T2T_SCORE_INC; // Score is increased with this value when rssi is above rssi threshold.
	uint8_t scoreThreshold = T2T_SCORE_THRESHOLD; // Threshold above which the toggle is triggered.
	uint8_t scoreMax = T2T_SCORE_MAX;      // Score can't be higher than this value.
	uint8_t timeoutTicks = (T2T_TIMEOUT_MS / TICK_INTERVAL_MS);   // Minimal time between 2 tap to toggle events.
	TapToToggle();
	void handleBackgroundAdvertisement(adv_background_parsed_t* adv);
	void tick();
};
