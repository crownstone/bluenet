/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Okt 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <processing/behaviour/cs_BehaviourHandler.h>
#include <processing/behaviour/cs_BehaviourStore.h>
#include <processing/behaviour/cs_SwitchBehaviour.h>

#include <presence/cs_PresenceDescription.h>
#include <presence/cs_PresenceHandler.h>

#include <time/cs_SystemTime.h>
#include <time/cs_TimeOfDay.h>

#include <common/cs_Types.h>

#include "drivers/cs_Serial.h"

#define LOGBehaviourHandler_V LOGnone
#define LOGBehaviourHandler LOGd

void BehaviourHandler::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_PRESENCE_MUTATION: {
            LOGBehaviourHandler("Presence mutation event in BehaviourHandler");
            update();
            break;
        }
        case CS_TYPE::STATE_TIME:{
            update();
            break;
        }
        case CS_TYPE::EVT_TIME_SET: {
            update();
            break;
        }
        case CS_TYPE::EVT_BEHAVIOURSTORE_MUTATION:{
            update();
            break;
        }
        default:{
            // ignore other events
            break;
        }
    }
}

void BehaviourHandler::update(){
    TimeOfDay time = SystemTime::now();
    std::optional<PresenceStateDescription> presence = PresenceHandler::getCurrentPresenceDescription();

    if(!presence){
        LOGBehaviourHandler_V("%02d:%02d:%02d, not updating, because presence data is missing",time.h(),time.m(),time.s());
        return;
    } 

    uint64_t p = presence.value();
    uint32_t p0 = p & 0xffffffff;
    uint32_t p1 = (p>>32) & 0xffffffff;
    LOGBehaviourHandler("presencedescription: %x %x", p1,p0);

    auto intendedState = computeIntendedState(time, presence.value());
    if(intendedState){
        if(previousIntendedState == intendedState){
            LOGBehaviourHandler_V("%02d:%02d:%02d, no behaviour change",time.h(),time.m(),time.s());
            return;
        }

        LOGBehaviourHandler("%02d:%02d:%02d, new behaviour value",time.h(),time.m(),time.s());
        previousIntendedState = intendedState;
        
        uint8_t intendedValue = intendedState.value();
        event_t behaviourStateChange(
            CS_TYPE::EVT_BEHAVIOUR_SWITCH_STATE,
            &intendedValue,
            sizeof(uint8_t)
        );

        behaviourStateChange.dispatch();
    } else {
        LOGBehaviourHandler_V("%02d:%02d:%02d, conflicting behaviours",time.h(),time.m(),time.s());
    }
}

std::optional<uint8_t> BehaviourHandler::computeIntendedState(
       TimeOfDay currentTime, 
       PresenceStateDescription currentPresence){
    std::optional<uint8_t> intendedValue = {};
    
    for (auto& b : BehaviourStore::getActiveBehaviours()){
        if (b.has_value() && b->isValid(currentTime, currentPresence)){
            if (intendedValue){
                if (b->value() != intendedValue.value()){
                    // found a conflicting behaviour
                    // TODO(Arend): add more advance conflict resolution according to document.
                    return std::nullopt;
                }
            } else {
                // found first valid behaviour
                intendedValue = b->value();
            }
        }
    }

    // reaching here means no conflict. An empty intendedValue should thus be resolved to 'off'
    return intendedValue.value_or(0);
}
