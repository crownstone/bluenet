/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cs_TwilightHandler.h>

void TwilightHandler::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_PRESENCE_MUTATION: {
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

void TwilightHandler::update(){
    TimeOfDay time = SystemTime::now();
    std::optional<PresenceStateDescription> presence = PresenceHandler::getCurrentPresenceDescription();

    if(!presence){
        return;
    } 

    auto intendedState = computeIntendedState(time, presence.value());
    if(intendedState){
        if(previousIntendedState == intendedState){
            return;
        }

        previousIntendedState = intendedState;
        
        uint8_t intendedValue = intendedState.value();
        event_t behaviourStateChange(
            CS_TYPE::EVT_BEHAVIOUR_SWITCH_STATE,
            &intendedValue,
            sizeof(uint8_t)
        );

        behaviourStateChange.dispatch();
    }
}

std::optional<uint8_t> TwilightHandler::computeIntendedState(TimeOfDay currenttime, PresenceStateDescription currentpresence){
    // return minimal value among the valid twilights.
    return {};
}