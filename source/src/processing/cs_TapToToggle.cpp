/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 2, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_TapToToggle.h"
#include "events/cs_EventDispatcher.h"
#include "drivers/cs_RTC.h"
#include "drivers/cs_Serial.h"
#include "processing/cs_CommandHandler.h"
#include "util/cs_Utils.h"
#include "storage/cs_State.h"

#define LOGT2Td LOGnone
#define LOGT2Tv LOGnone

TapToToggle::TapToToggle() {
	State::getInstance().get(CS_TYPE::CONFIG_TAP_TO_TOGGLE_ENABLED, &enabled, sizeof(enabled));
	State::getInstance().get(CS_TYPE::CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD, &rssiThreshold, sizeof(rssiThreshold));
	EventDispatcher::getInstance().addListener(this);
}

void TapToToggle::handleBackgroundAdvertisement(adv_background_parsed_t* adv) {
	LOGT2Td("addr=%x:%x:%x:%x:%x:%x rssi=%i flags=%u", adv->macAddress[0], adv->macAddress[1], adv->macAddress[2], adv->macAddress[3], adv->macAddress[4], adv->macAddress[5], adv->adjustedRssi, adv->flags);
	if (adv->adjustedRssi < rssiThreshold) {
		LOGT2Td("rssi too low");
		return;
	}
	// 2019-11-12 TODO: don't use magic number 2.
	if (!BLEutil::isBitSet(adv->flags, 2)) {
		LOGT2Td("t2t flag not set");
		return;
	}
	// Use index of entry with matching address, or else with lowest score.
	uint8_t index = -1; // Index to use
	bool foundAddress = false;
	uint8_t lowestScore = 255;
	for (uint8_t i=0; i<T2T_LIST_COUNT; ++i) {
		if (memcmp(list[i].address, adv->macAddress, BLE_GAP_ADDR_LEN) == 0) {
			index = i;
			foundAddress = true;
			break;
		}
		if (list[i].score < lowestScore) {
			lowestScore = list[i].score;
			index = i;
		}
	}
	if (!foundAddress) {
		memcpy(list[index].address, adv->macAddress, BLE_GAP_ADDR_LEN);
		list[index].score = 0;
	}

	// By placing this check here, the score will drop, which will make it more likely to trigger again when phone is kept close.
	if (timeoutCounter != 0) {
		LOGT2Td("wait with t2t");
		return;
	}

	uint8_t prevScore = list[index].score;
	list[index].score += scoreIncrement;
	if (list[index].score > scoreMax) {
		list[index].score = scoreMax;
	}

	LOGT2Td("rssi=%i ind=%u prevScore=%u score=%u", adv->adjustedRssi, index, prevScore, list[index].score);
	if (prevScore <= scoreThreshold && list[index].score > scoreThreshold) {
		LOGi("Tap to toggle triggered");
		timeoutCounter = timeoutTicks;
		event_t event(CS_TYPE::CMD_SWITCH_TOGGLE);
		EventDispatcher::getInstance().dispatch(event);
	}
}

void TapToToggle::tick() {
	for (uint8_t i=0; i<T2T_LIST_COUNT; ++i) {
		if (list[i].score) {
			list[i].score--;
		}
	}
	if (timeoutCounter) {
		timeoutCounter--;
	}
	LOGT2Tv("scores=%u %u %u", list[0].score, list[1].score, list[2].score)
}

void TapToToggle::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::EVT_TICK: {
		tick();
		break;
	}
	case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
		TYPIFY(EVT_ADV_BACKGROUND_PARSED)* backgroundAdv = (TYPIFY(EVT_ADV_BACKGROUND_PARSED)*)event.data;
		handleBackgroundAdvertisement(backgroundAdv);
		break;
	}
	case CS_TYPE::CONFIG_TAP_TO_TOGGLE_ENABLED: {
		enabled = *((TYPIFY(CONFIG_TAP_TO_TOGGLE_ENABLED)*)event.data);
		break;
	}
	case CS_TYPE::CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD: {
		rssiThreshold = *((TYPIFY(CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD)*)event.data);
		break;
	}
	default:
		break;
	}
}

