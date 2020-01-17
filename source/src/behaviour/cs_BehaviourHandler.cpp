/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Okt 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <behaviour/cs_BehaviourHandler.h>
#include <behaviour/cs_BehaviourStore.h>
#include <behaviour/cs_SwitchBehaviour.h>

#include <presence/cs_PresenceDescription.h>
#include <presence/cs_PresenceHandler.h>

#include <storage/cs_State.h>

#include <time/cs_SystemTime.h>
#include <time/cs_TimeOfDay.h>

#include <common/cs_Types.h>

#include "drivers/cs_Serial.h"


#define BehaviourHandlerDebug false

#if BehaviourHandlerDebug == true
#define LOGBehaviourHandler LOGd
#define LOGBehaviourHandler_V LOGi
#else 
#define LOGBehaviourHandler LOGnone
#define LOGBehaviourHandler_V LOGnone
#endif



void BehaviourHandler::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_PRESENCE_MUTATION: {
            LOGBehaviourHandler("Presence mutation event in BehaviourHandler");
            update();
            break;
        }
        case CS_TYPE::EVT_BEHAVIOURSTORE_MUTATION:{
            update();
            break;
        }
        case CS_TYPE::STATE_BEHAVIOUR_SETTINGS: {
        	behaviour_settings_t* settings = reinterpret_cast<TYPIFY(STATE_BEHAVIOUR_SETTINGS)*>(evt.data);
        	isActive = settings->flags.enabled;
            LOGi("settings isActive=%u", isActive);
            update();
            break;
        }
        case CS_TYPE::CMD_GET_BEHAVIOUR_DEBUG: {
        	handleGetBehaviourDebug(evt);
        	break;
        }
        default:{
            // ignore other events
            break;
        }
    }
}

bool BehaviourHandler::update(){
    if (!isActive) {
        currentIntendedState = std::nullopt;
    } else {
        Time time = SystemTime::now();
        std::optional<PresenceStateDescription> presence = PresenceHandler::getCurrentPresenceDescription();

        if (!presence) {
            LOGBehaviourHandler_V("Not updating, because presence data is missing");
        } else {
            currentIntendedState = computeIntendedState(time, presence.value());
        }
    }

    return true;
}

std::optional<uint8_t> BehaviourHandler::computeIntendedState(
       Time currentTime, 
       PresenceStateDescription currentPresence) {
    if (!isActive) {
        return {};
    }

    if (!currentTime.isValid()) {
        LOGBehaviourHandler("Current time invalid, computed intended state: empty");
        return {};
    }

    LOGBehaviourHandler("BehaviourHandler compute intended state");
    std::optional<uint8_t> intendedValue = {};
    
    for (auto& b : BehaviourStore::getActiveBehaviours()) {
        if(SwitchBehaviour * switchbehave = dynamic_cast<SwitchBehaviour*>(b)) {
            // cast to switch behaviour succesful.
            // note: this may also be an extendedswitchbehaviour - which is intended!
            if (switchbehave->isValid(currentTime)) {
                LOGBehaviourHandler_V("valid time on behaviour: ");
            }
            if (switchbehave->isValid(currentTime, currentPresence)) {
                LOGBehaviourHandler_V("presence also valid");
                if constexpr (BehaviourHandlerDebug) {
                    switchbehave->print();
                }

                if (intendedValue){
                    if (switchbehave->value() != intendedValue.value()) {
                        // found a conflicting behaviour
                        // TODO(Arend): add more advance conflict resolution according to document.
                        return std::nullopt;
                    }
                } else {
                    // found first valid behaviour
                    intendedValue = switchbehave->value();
                }
            }
        }
    }

    // reaching here means no conflict. An empty intendedValue should thus be resolved to 'off'
    return intendedValue.value_or(0);
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
	behaviourDebug->activeBehaviours = 0;
	behaviourDebug->extensionActive = 0; // TODO: how to calculate this?
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
		auto behaviours = BehaviourStore::getActiveBehaviours();
		for (uint8_t index = 0; index < behaviours.size(); ++index) {
			if (SwitchBehaviour * switchbehave = dynamic_cast<SwitchBehaviour*>(behaviours[index])) {
				// cast to switch behaviour succesful.
				// note: this may also be an extendedswitchbehaviour - which is intended!
				if (switchbehave->isValid(currentTime, currentPresence.value())) {
					behaviourDebug->activeBehaviours |= (1 << index);
				}
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

std::optional<uint8_t> BehaviourHandler::getValue(){
    previousIntendedState = currentIntendedState;
    return currentIntendedState;
}

bool BehaviourHandler::requiresPresence(Time t){
    uint8_t i = 0;
    for (auto& behaviour_ptr : BehaviourStore::getActiveBehaviours()){
        i += 1;
        if(behaviour_ptr != nullptr){
            if(behaviour_ptr->requiresPresence()){
                LOGBehaviourHandler_V("presence requiring behaviour found %d", i);
                if(behaviour_ptr->isValid(t)) {
                    LOGBehaviourHandler_V("presence requiring behaviour is currently valid %d", i);
                    return true;
                }
            }
        }
    }

    return false;
}

bool BehaviourHandler::requiresAbsence(Time t){
    for (auto& behaviour_ptr : BehaviourStore::getActiveBehaviours()){
        if(behaviour_ptr != nullptr){
            if(behaviour_ptr->isValid(t) && behaviour_ptr->requiresAbsence()) {
                return true;
            }
        }
    }

    return false;
}
