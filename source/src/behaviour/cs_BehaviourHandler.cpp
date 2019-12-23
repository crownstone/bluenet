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

#include <time/cs_SystemTime.h>
#include <time/cs_TimeOfDay.h>

#include <common/cs_Types.h>

#include "drivers/cs_Serial.h"

#define LOGBehaviourHandler_V LOGnone
#define LOGBehaviourHandler LOGnone



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
        case CS_TYPE::CMD_BEHAVIOURHANDLER_SETTINGS:{
            isActive =  evt.getData()[0] != 0;
            LOGBehaviourHandler("behaviourhandler settings isActive:%s", (isActive ? "true" : "false"));
            update();
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
        TimeOfDay time = SystemTime::now();
        std::optional<PresenceStateDescription> presence = PresenceHandler::getCurrentPresenceDescription();

        if (!presence) {
            LOGBehaviourHandler_V("%02d:%02d:%02d, not updating, because presence data is missing",time.h(),time.m(),time.s());
        } else {
            currentIntendedState = computeIntendedState(time, presence.value());
        }
    }

    return true;//currentIntendedState != previousIntendedState;
}

std::optional<uint8_t> BehaviourHandler::computeIntendedState(
       TimeOfDay currentTime, 
       PresenceStateDescription currentPresence){
    if (!isActive) {
        return {};
    }

    LOGBehaviourHandler("BehaviourHandler compute intended state");
    std::optional<uint8_t> intendedValue = {};
    
    for (auto& b : BehaviourStore::getActiveBehaviours()){
        if(SwitchBehaviour * switchbehave = dynamic_cast<SwitchBehaviour*>(b)){
            // cast to switch behaviour succesful.
            if(switchbehave->isValid(currentTime)){
                LOGBehaviourHandler_V("valid time on behaviour: ");
            }
            if (switchbehave->isValid(currentTime, currentPresence)){
                LOGBehaviourHandler_V("presence also valid");
                if (intendedValue){
                    if (switchbehave->value() != intendedValue.value()){
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

std::optional<uint8_t> BehaviourHandler::getValue(){
    previousIntendedState = currentIntendedState;
    return currentIntendedState;
}

bool BehaviourHandler::requiresPresence(TimeOfDay t){
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

bool BehaviourHandler::requiresAbsence(TimeOfDay t){
    for (auto& behaviour_ptr : BehaviourStore::getActiveBehaviours()){
        if(behaviour_ptr != nullptr){
            if(behaviour_ptr->isValid(t) && behaviour_ptr->requiresAbsence()) {
                return true;
            }
        }
    }

    return false;
}