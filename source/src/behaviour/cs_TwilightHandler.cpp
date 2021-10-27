/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourConflictResolution.h>
#include <behaviour/cs_BehaviourStore.h>
#include <behaviour/cs_TwilightBehaviour.h>
#include <behaviour/cs_TwilightHandler.h>
#include <logging/cs_Logger.h>
#include <storage/cs_State.h>
#include <test/cs_Test.h>
#include <time/cs_SystemTime.h>

#define LOGTwilightHandlerDebug LOGnone

void TwilightHandler::init() {
	TYPIFY(STATE_BEHAVIOUR_SETTINGS) settings;
	State::getInstance().get(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, &settings, sizeof(settings));
	isActive = settings.flags.enabled;

	LOGi("Init: isActive=%u", isActive);

	listen();
}

void TwilightHandler::handleEvent(event_t& evt) {
	switch (evt.type) {
		case CS_TYPE::EVT_PRESENCE_MUTATION: {
			update();
			break;
		}
		case CS_TYPE::EVT_BEHAVIOURSTORE_MUTATION: {
			update();
			break;
		}
		case CS_TYPE::STATE_BEHAVIOUR_SETTINGS: {
			behaviour_settings_t* settings = reinterpret_cast<TYPIFY(STATE_BEHAVIOUR_SETTINGS)*>(evt.data);
			isActive = settings->flags.enabled;
			LOGTwilightHandlerDebug("TwilightHandler.isActive=%u", isActive);
			TEST_PUSH_B(this, isActive);
			update();
			break;
		}
		default: {
			// ignore other events
			break;
		}
	}
}

bool TwilightHandler::update() {
	Time time = SystemTime::now();
	auto nextIntendedState = computeIntendedState(time);

	bool valuechanged = currentIntendedState != nextIntendedState;
	currentIntendedState = nextIntendedState;

	TEST_PUSH_EXPR_D(this, "currentIntendedState", (currentIntendedState ? (int)currentIntendedState.value() : -1));
	return valuechanged;
}

std::optional<uint8_t> TwilightHandler::computeIntendedState(Time currentTime) {
	if (!isActive) {
		return {};
	}

	if (!currentTime.isValid()) {
		return {};
	}

	uint32_t nowTimeOfDay = currentTime.timeOfDay();
	uint32_t winningFromTimeOfDay = 0;
	uint32_t winningUntilTimeOfDay = 0;
	uint8_t winningValue = 0xFF;

	// loop through all twilight behaviours searching for valid ones.
	for (auto& b : BehaviourStore::getActiveBehaviours()) {
		if (TwilightBehaviour * behaviour = dynamic_cast<TwilightBehaviour*>(b)) {
			// cast to twilight behaviour succesful.
			if (behaviour->isValid(currentTime)) {
				// cache some values.
				uint32_t candidateFromTimeOfDay = behaviour->from();
				uint32_t candidateUntilTimeOfDay = behaviour->until();

				bool okToOverwriteWinningValue;

				if (winningValue == 0xFF) {
					// no previous values, anything will do
					okToOverwriteWinningValue = true;
				}
				else {
					// previous value found, conflict resolution
					okToOverwriteWinningValue = FromUntilIntervalIsMoreRelevantOrEqual(
							candidateFromTimeOfDay, candidateUntilTimeOfDay,
							winningFromTimeOfDay, winningUntilTimeOfDay,
							nowTimeOfDay
					);
				}

				if (okToOverwriteWinningValue) {
					// what a silly edge case, there are two (or more) twilights with the exact same
					// boundary times. Let's use the minimum value in that case.
					if (candidateFromTimeOfDay == winningFromTimeOfDay && candidateUntilTimeOfDay == winningUntilTimeOfDay) {
						winningValue = CsMath::min(behaviour->value(), winningValue);
					}
					else {
						winningValue = behaviour->value();
					}

					// update winning value and winning from/until boundaries
					winningFromTimeOfDay = candidateFromTimeOfDay;
					winningUntilTimeOfDay = candidateUntilTimeOfDay;
				}
			}
		}
	}

	// if no winning_value is found at all, return 100.
	return winningValue == 0xFF ? 100 : winningValue;
}

std::optional<uint8_t> TwilightHandler::getValue() {
	return currentIntendedState;
}
