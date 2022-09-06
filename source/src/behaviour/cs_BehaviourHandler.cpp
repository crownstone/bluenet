/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Okt 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourConflictResolution.h>
#include <behaviour/cs_BehaviourHandler.h>
#include <behaviour/cs_BehaviourStore.h>
#include <behaviour/cs_SwitchBehaviour.h>
#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <presence/cs_PresenceDescription.h>
#include <presence/cs_PresenceHandler.h>
#include <storage/cs_State.h>
#include <test/cs_Test.h>
#include <time/cs_SystemTime.h>
#include <time/cs_TimeOfDay.h>

#define LOGBehaviourHandlerDebug LOGvv
#define LOGBehaviourHandlerVerbose LOGvv

cs_ret_code_t BehaviourHandler::init() {
	TYPIFY(STATE_BEHAVIOUR_SETTINGS) settings;

	State::getInstance().get(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, &settings, sizeof(settings));
	_isActive = settings.flags.enabled;

	LOGi("Init: _isActive=%u", _isActive);
	_presenceHandler = getComponent<PresenceHandler>();
	_behaviourStore  = getComponent<BehaviourStore>();

	listen();

	return ERR_SUCCESS;
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
		case CS_TYPE::CMD_GET_BEHAVIOUR_DEBUG: {
			handleGetBehaviourDebug(evt);
			break;
		}
		case CS_TYPE::STATE_BEHAVIOUR_SETTINGS: {
			behaviour_settings_t* settings = CS_TYPE_CAST(STATE_BEHAVIOUR_SETTINGS, evt.data);
			onBehaviourSettingsChange(*settings);
			break;
		}
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto meshMsg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, evt.data);
			if (meshMsg->type == CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS) {
				auto packet = meshMsg->getPacket<CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS>();
				onBehaviourSettingsMeshMsg(packet);
				evt.result.returnCode = ERR_SUCCESS;
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING: {
			auto request                    = CS_TYPE_CAST(EVT_MESH_SYNC_REQUEST_OUTGOING, evt.data);
			request->bits.behaviourSettings = onBehaviourSettingsOutgoingSyncRequest();
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING: {
			auto request = CS_TYPE_CAST(EVT_MESH_SYNC_REQUEST_INCOMING, evt.data);
			if (request->bits.behaviourSettings) {
				onBehaviourSettingsIncomingSyncRequest();
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_FAILED: {
			onMeshSyncFailed();
			break;
		}
		default: {
			// ignore other events
			break;
		}
	}
}

bool BehaviourHandler::update() {
	if (!_isActive || _presenceHandler == nullptr) {
		currentIntendedState = std::nullopt;
	}
	else {
		Time time                                        = SystemTime::now();
		std::optional<PresenceStateDescription> presence = _presenceHandler->getCurrentPresenceDescription();

		if (!presence) {
			LOGBehaviourHandlerVerbose("Not updating, because presence data is missing");
		}
		else {
			currentIntendedState = computeIntendedState(time, presence.value());
		}
	}

	return true;
}

SwitchBehaviour* BehaviourHandler::ValidateSwitchBehaviour(
		Behaviour* behave, Time currentTime, PresenceStateDescription currentPresence) const {
	if (SwitchBehaviour* switchbehave = dynamic_cast<SwitchBehaviour*>(behave)) {
		if (switchbehave->isValid(currentTime, currentPresence)) {
			return switchbehave;
		}
	}

	return nullptr;
}

SwitchBehaviour* BehaviourHandler::resolveSwitchBehaviour(
        Time currentTime, PresenceStateDescription currentPresence) const {
    LOGBehaviourHandlerDebug("BehaviourHandler computeIntendedState resolves");

    // 'best' meaning most relevant considering from/until time window.
    SwitchBehaviour* currentBestSwitchBehaviour = nullptr;
    for (auto candidateBehaviour : _behaviourStore->getActiveBehaviours()) {
        SwitchBehaviour* candidateSwitchBehaviour =
                ValidateSwitchBehaviour(candidateBehaviour, currentTime, currentPresence);

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
        if (FromUntilIntervalIsEqual(currentBestSwitchBehaviour, candidateSwitchBehaviour)) {
            // when interval coincides, lowest intensity behaviour wins:
            if (candidateSwitchBehaviour->value() < currentBestSwitchBehaviour->value()) {
                currentBestSwitchBehaviour = candidateSwitchBehaviour;
            }
        }
        else if (FromUntilIntervalIsMoreRelevantOrEqual(
                candidateSwitchBehaviour, currentBestSwitchBehaviour, currentTime)) {
            // when interval is more relevant, that behaviour wins
            currentBestSwitchBehaviour = candidateSwitchBehaviour;
        }
    }

    return currentBestSwitchBehaviour;

}

std::optional<uint8_t> BehaviourHandler::computeIntendedState(
		Time currentTime, PresenceStateDescription currentPresence) const {
    if (!_isActive) {
        LOGBehaviourHandlerDebug("Behaviour handler is inactive, computed intended state: empty");
        return {};
    }
    if (!currentTime.isValid()) {
        LOGBehaviourHandlerDebug("Current time invalid, computed intended state: empty");
        return {};
    }
    if (_behaviourStore == nullptr) {
        LOGBehaviourHandlerDebug("BehaviourStore is nullptr, computed intended state: empty");
        return {};
    }

    Behaviour* bestMatchinSwitchBehaviour = resolveSwitchBehaviour(currentTime, currentPresence);
    if(bestMatchinSwitchBehaviour) {
        return bestMatchinSwitchBehaviour->value();
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

	Time currentTime                  = SystemTime::now();
	std::optional<PresenceStateDescription> currentPresence =
			_presenceHandler == nullptr ? std::nullopt : _presenceHandler->getCurrentPresenceDescription();

	// Set time.
	behaviourDebug->time = currentTime.isValid() ? currentTime.timestamp() : 0;

	// Set sunrise and sunset.
	TYPIFY(STATE_SUN_TIME) sunTime;
	State::getInstance().get(CS_TYPE::STATE_SUN_TIME, &sunTime, sizeof(sunTime));
	behaviourDebug->sunrise          = sunTime.sunrise;
	behaviourDebug->sunset           = sunTime.sunset;

	// Set behaviour enabled.
	behaviourDebug->behaviourEnabled = _isActive;

	// Set presence.
	for (uint8_t i = 0; i < 8; ++i) {
		behaviourDebug->presence[i] = 0;
	}
	behaviourDebug->presence[0]         = currentPresence ? currentPresence.value().getBitmask() : 0;

	// Set active behaviours.
	behaviourDebug->storedBehaviours    = 0;
	behaviourDebug->activeBehaviours    = 0;
	behaviourDebug->extensionActive     = 0;
	behaviourDebug->activeTimeoutPeriod = 0;

	if (_behaviourStore == nullptr) {
		LOGMeshWarning("BehaviourStore is nullptr, no info available.");
		evt.result.dataSize   = sizeof(behaviour_debug_t);
		evt.result.returnCode = ERR_SUCCESS;
		return;
	}

	auto behaviours = _behaviourStore->getActiveBehaviours();

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
	if (!_isActive) {
		checkBehaviours = false;
	}
	if (!currentTime.isValid()) {
		checkBehaviours = false;
	}

	if (checkBehaviours) {
		for (uint8_t index = 0; index < behaviours.size(); ++index) {
			if (SwitchBehaviour* switchbehave = dynamic_cast<SwitchBehaviour*>(behaviours[index])) {
				// cast to switch behaviour succesful.
				// note: this may also be an extendedswitchbehaviour - which is intended!
				if (switchbehave->isValid(currentTime, currentPresence.value())) {
					behaviourDebug->activeBehaviours |= (1 << index);

					behaviourDebug->activeTimeoutPeriod |=
							switchbehave->gracePeriodForPresenceIsActive() ? (1 << index) : 0;
				}
			}

			if (ExtendedSwitchBehaviour* extendedswitchbehave =
						dynamic_cast<ExtendedSwitchBehaviour*>(behaviours[index])) {
				behaviourDebug->extensionActive |= extendedswitchbehave->extensionPeriodIsActive() ? (1 << index) : 0;
			}

			if (TwilightBehaviour* twilight = dynamic_cast<TwilightBehaviour*>(behaviours[index])) {
				if (twilight->isValid(currentTime)) {
					behaviourDebug->activeBehaviours |= (1 << index);
				}
			}
		}
	}

	evt.result.dataSize   = sizeof(behaviour_debug_t);
	evt.result.returnCode = ERR_SUCCESS;
}

std::optional<uint8_t> BehaviourHandler::getValue() {
	previousIntendedState = currentIntendedState;
	return currentIntendedState;
}

bool BehaviourHandler::requiresPresence(Time t) {
	if (_behaviourStore == nullptr) {
		return false;
	}

	uint8_t i = 0;
	for (auto& behaviourPtr : _behaviourStore->getActiveBehaviours()) {
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
	if (_behaviourStore == nullptr) {
		return false;
	}

	for (auto& behaviourPtr : _behaviourStore->getActiveBehaviours()) {
		if (behaviourPtr != nullptr) {
			if (behaviourPtr->isValid(t) && behaviourPtr->requiresAbsence()) {
				return true;
			}
		}
	}

	return false;
}

// --------------------------- synchronization ---------------------------

void BehaviourHandler::onBehaviourSettingsChange(behaviour_settings_t settings) {
	_isActive = settings.flags.enabled;
	LOGi("onBehaviourSettingsChange active=%u", _isActive);
	TEST_PUSH_B(this, _isActive);
	UartHandler::getInstance().writeMsg(
			UART_OPCODE_TX_MESH_SET_BEHAVIOUR_SETTINGS, reinterpret_cast<uint8_t*>(&settings), sizeof(settings));
	update();
}

void BehaviourHandler::onBehaviourSettingsMeshMsg(behaviour_settings_t settings) {
	LOGBehaviourHandlerDebug("onBehaviourSettingsMeshMsg settings=%u", settings.asInt);
	// When syncing, handle all responses, and merge them to a single conclusion.
	// This handles the case that there are different opionons in the mesh.
	// Don't update _isActive until we handled all responses.
	if (!_behaviourSettingsSynced) {
		if (_receivedBehaviourSettings) {
			// Already receive behaviour settings: merge them.

			// Enabled overrules disabled.
			settings.flags.enabled = settings.flags.enabled || _receivedBehaviourSettings.value().flags.enabled;
		}
		_receivedBehaviourSettings = settings;
	}
	else {
		TYPIFY(STATE_BEHAVIOUR_SETTINGS) stateSettings = settings;
		State::getInstance().set(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, &stateSettings, sizeof(stateSettings));
		// State set triggers onBehaviourSettingsChange().
	}
}

bool BehaviourHandler::onBehaviourSettingsOutgoingSyncRequest() {
	if (_behaviourSettingsSynced) {
		return false;
	}
	LOGBehaviourHandlerDebug("onBehaviourSettingsOutgoingSyncRequest");

	// If we already sent a sync request, we should have received all the responses by now.
	// So finalize the sync if we received any responses.
	tryFinalizeBehaviourSettingsSync();
	return !_behaviourSettingsSynced;
}

void BehaviourHandler::onMeshSyncFailed() {
	if (_behaviourSettingsSynced) {
		// Overall sync failed, but behaviour settings did sync.
		return;
	}
	LOGBehaviourHandlerDebug("Sync failed");

	// If we sent a sync request, we should have received all the responses by now.
	// So finalize the sync if we received any responses.
	tryFinalizeBehaviourSettingsSync();

	// Consider synced anyway, so that we will respond to sync requests in the future.
	_behaviourSettingsSynced = true;
}

void BehaviourHandler::tryFinalizeBehaviourSettingsSync() {
	if (!_receivedBehaviourSettings) {
		// We didn't receive any sync response.
		return;
	}
	LOGBehaviourHandlerDebug("Finalize behaviour settings sync");

	_behaviourSettingsSynced                       = true;

	// After we received all the sync responses, we store the final settings.
	TYPIFY(STATE_BEHAVIOUR_SETTINGS) stateSettings = _receivedBehaviourSettings.value();
	State::getInstance().set(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, &stateSettings, sizeof(stateSettings));
	// This will trigger onBehaviourSettingsChange() to be called.
}

void BehaviourHandler::onBehaviourSettingsIncomingSyncRequest() {
	if (!_behaviourSettingsSynced) {
		// Only send a sync response if we are in sync.
		return;
	}
	LOGBehaviourHandlerDebug("Send behaviour settings response");

	TYPIFY(STATE_BEHAVIOUR_SETTINGS) settings;
	State::getInstance().get(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, &settings, sizeof(settings));

	TYPIFY(CMD_SEND_MESH_MSG) meshMsg;
	meshMsg.type                   = CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS;
	meshMsg.reliability            = CS_MESH_RELIABILITY_LOWEST;
	meshMsg.urgency                = CS_MESH_URGENCY_LOW;
	meshMsg.flags.flags.broadcast  = true;
	meshMsg.flags.flags.doNotRelay = false;
	meshMsg.payload                = reinterpret_cast<uint8_t*>(&settings);
	meshMsg.size                   = sizeof(settings);

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
}
