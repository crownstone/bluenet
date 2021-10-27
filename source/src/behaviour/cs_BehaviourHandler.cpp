/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Okt 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <behaviour/cs_BehaviourHandler.h>

#include <behaviour/cs_BehaviourConflictResolution.h>
#include <behaviour/cs_BehaviourStore.h>
#include <behaviour/cs_SwitchBehaviour.h>
#include <common/cs_Types.h>
#include <presence/cs_PresenceDescription.h>
#include <presence/cs_PresenceHandler.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <time/cs_TimeOfDay.h>
#include <logging/cs_Logger.h>
#include <test/cs_Test.h>


#define LOGBehaviourHandlerDebug LOGnone
#define LOGBehaviourHandlerVerbose LOGnone

void BehaviourHandler::init() {
	TYPIFY(STATE_BEHAVIOUR_SETTINGS) settings;
	State::getInstance().get(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, &settings, sizeof(settings));
	isActive = settings.flags.enabled;

	LOGi("Init: isActive=%u", isActive);

	listen();
}

void BehaviourHandler::handleEvent(event_t& evt) {
	switch (evt.type) {
		case CS_TYPE::EVT_PRESENCE_MUTATION: {
			LOGBehaviourHandlerDebug("Presence mutation event in BehaviourHandler");
			update();
			break;
		}
		case CS_TYPE::EVT_BEHAVIOURSTORE_MUTATION: {
			update();
			break;
		}
		case CS_TYPE::STATE_BEHAVIOUR_SETTINGS: {
			behaviour_settings_t* settings = CS_TYPE_CAST(STATE_BEHAVIOUR_SETTINGS, evt.data);
			isActive = settings->flags.enabled;
			LOGi("settings isActive=%u", isActive);
			TEST_PUSH_B(this, isActive);
			update();
			break;
		}
		case CS_TYPE::CMD_GET_BEHAVIOUR_DEBUG: {
			handleGetBehaviourDebug(evt);
			break;
		}
		default: {
			// ignore other events
			break;
		}
	}
}

bool BehaviourHandler::update() {
	if (!isActive) {
		currentIntendedState = std::nullopt;
	}
	else {
		Time time = SystemTime::now();
		std::optional<PresenceStateDescription> presence = PresenceHandler::getCurrentPresenceDescription();

		if (!presence) {
			LOGBehaviourHandlerVerbose("Not updating, because presence data is missing");
		}
		else {
			currentIntendedState = computeIntendedState(time, presence.value());
		}
	}

	return true;
}

SwitchBehaviour* BehaviourHandler::ValidateSwitchBehaviour(Behaviour* behave, Time currentTime, PresenceStateDescription currentPresence) {
	if (SwitchBehaviour * switchbehave = dynamic_cast<SwitchBehaviour*>(behave)) {
		if (switchbehave->isValid(currentTime) &&
				switchbehave->isValid(currentTime, currentPresence)) {
			return switchbehave;
		}
	}

	return nullptr;
}

std::optional<uint8_t> BehaviourHandler::computeIntendedState(
		Time currentTime,
		PresenceStateDescription currentPresence) {
	if (!isActive) {
		LOGBehaviourHandlerDebug("Behaviour handler is inactive, computed intended state: empty");
		return {};
	}
	if (!currentTime.isValid()) {
		LOGBehaviourHandlerDebug("Current time invalid, computed intended state: empty");
		return {};
	}

	LOGBehaviourHandlerDebug("BehaviourHandler computeIntendedState resolves");

	// 'best' meaning most relevant considering from/until time window.
	SwitchBehaviour* currentBestSwitchBehaviour = nullptr;
	for (auto candidateBehaviour : BehaviourStore::getActiveBehaviours()) {
		SwitchBehaviour* candidateSwitchBehaviour = ValidateSwitchBehaviour(
				candidateBehaviour, currentTime, currentPresence);

		// check for failed transformation from right to left. If either
		// current or candidate is nullptr, we can continue to the next candidate.
		if (currentBestSwitchBehaviour == nullptr) {
			// candidate always wins when there is no current best.
			currentBestSwitchBehaviour = candidateSwitchBehaviour;
			continue;
		}
		if (candidateSwitchBehaviour == nullptr) {
			continue;
		}

		// conflict resolve:
		if (FromUntilIntervalIsEqual(
				currentBestSwitchBehaviour,
				candidateSwitchBehaviour)) {
			// when interval coincides, lowest intensity behaviour wins:
			if (candidateSwitchBehaviour->value() < currentBestSwitchBehaviour->value()) {
				currentBestSwitchBehaviour = candidateSwitchBehaviour;
			}
		}
		else if (FromUntilIntervalIsMoreRelevantOrEqual(
				candidateSwitchBehaviour,
				currentBestSwitchBehaviour,
				currentTime)) {
			// when interval is more relevant, that behaviour wins
			currentBestSwitchBehaviour = candidateSwitchBehaviour;
		}
	}


	if (currentBestSwitchBehaviour) {
		return currentBestSwitchBehaviour->value();
	}

	return 0;
}

void BehaviourHandler::handleGetBehaviourDebug(event_t& evt) {
	LOGd("handleGetBehaviourDebug");
	if (evt.result.buf.data == nullptr) {
		LOGd("ERR_BUFFER_UNASSIGNED");
		evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
		return;
	}
	if (evt.result.buf.len < sizeof(behaviour_debug_t)) {
		LOGd("ERR_BUFFER_TOO_SMALL");
		evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}
	behaviour_debug_t* behaviourDebug = (behaviour_debug_t*)(evt.result.buf.data);

	Time currentTime = SystemTime::now();
	std::optional<PresenceStateDescription> currentPresence = PresenceHandler::getCurrentPresenceDescription();

	// Set time.
	behaviourDebug->time = currentTime.isValid() ? currentTime.timestamp() : 0;

	// Set sunrise and sunset.
	TYPIFY(STATE_SUN_TIME) sunTime;
	State::getInstance().get(CS_TYPE::STATE_SUN_TIME, &sunTime, sizeof(sunTime));
	behaviourDebug->sunrise = sunTime.sunrise;
	behaviourDebug->sunset  = sunTime.sunset;

	// Set behaviour enabled.
	behaviourDebug->behaviourEnabled = isActive;

	// Set presence.
	for (uint8_t i = 0; i < 8; ++i) {
		behaviourDebug->presence[i] = 0;
	}
	behaviourDebug->presence[0] = currentPresence ? currentPresence.value().getBitmask() : 0;

	// Set active behaviours.
	behaviourDebug->storedBehaviours = 0;
	behaviourDebug->activeBehaviours = 0;
	behaviourDebug->extensionActive = 0;
	behaviourDebug->activeTimeoutPeriod = 0;

	auto behaviours = BehaviourStore::getActiveBehaviours();

	for (uint8_t index = 0; index < behaviours.size(); ++index) {
		if (behaviours[index] != nullptr) {
			behaviourDebug->storedBehaviours |= (1 << index);
		}
	}
	bool checkBehaviours = true;

	// Kept code similar to update() followed by computeIntendedState().
	if (!currentPresence) {
		checkBehaviours = false;
	}
	if (!isActive) {
		checkBehaviours = false;
	}
	if (!currentTime.isValid()) {
		checkBehaviours = false;
	}

	if (checkBehaviours) {
		for (uint8_t index = 0; index < behaviours.size(); ++index) {
			if (SwitchBehaviour * switchbehave = dynamic_cast<SwitchBehaviour*>(behaviours[index])) {
				// cast to switch behaviour succesful.
				// note: this may also be an extendedswitchbehaviour - which is intended!
				if (switchbehave->isValid(currentTime, currentPresence.value())) {
					behaviourDebug->activeBehaviours |= (1 << index);

					behaviourDebug->activeTimeoutPeriod |=
							switchbehave->gracePeriodForPresenceIsActive() ? (1 << index) : 0;
				}
			}

			if (ExtendedSwitchBehaviour* extendedswitchbehave = dynamic_cast<ExtendedSwitchBehaviour*>(behaviours[index])) {
				behaviourDebug->extensionActive |=
						extendedswitchbehave->extensionPeriodIsActive() ? (1 << index) : 0;
			}

			if (TwilightBehaviour * twilight = dynamic_cast<TwilightBehaviour*>(behaviours[index])) {
				if (twilight->isValid(currentTime)) {
					behaviourDebug->activeBehaviours |= (1 << index);
				}
			}
		}
	}

	evt.result.dataSize = sizeof(behaviour_debug_t);
	evt.result.returnCode = ERR_SUCCESS;
}

std::optional<uint8_t> BehaviourHandler::getValue() {
	previousIntendedState = currentIntendedState;
	return currentIntendedState;
}

bool BehaviourHandler::requiresPresence(Time t) {
	uint8_t i = 0;
	for (auto& behaviourPtr : BehaviourStore::getActiveBehaviours()) {
		i += 1;
		if (behaviourPtr != nullptr) {
			if (behaviourPtr->requiresPresence()) {
				LOGBehaviourHandlerVerbose("presence requiring behaviour found %d", i);
				if (behaviourPtr->isValid(t)) {
					LOGBehaviourHandlerVerbose("presence requiring behaviour is currently valid %d", i);
					return true;
				}
			}
		}
	}

	return false;
}

bool BehaviourHandler::requiresAbsence(Time t) {
	for (auto& behaviourPtr : BehaviourStore::getActiveBehaviours()) {
		if (behaviourPtr != nullptr) {
			if (behaviourPtr->isValid(t) && behaviourPtr->requiresAbsence()) {
				return true;
			}
		}
	}

	return false;
}
