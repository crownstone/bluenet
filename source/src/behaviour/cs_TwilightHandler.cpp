/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_TwilightHandler.h>
#include <behaviour/cs_TwilightBehaviour.h>
#include <behaviour/cs_BehaviourStore.h>

#include <time/cs_SystemTime.h>

void TwilightHandler::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_PRESENCE_MUTATION: {
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

bool TwilightHandler::update(){
    Time time = SystemTime::now();

    uint8_t intendedState = computeIntendedState(time);
    if(previousIntendedState == intendedState){
        return false;
    }

    previousIntendedState = intendedState;
    return true;
}

uint8_t TwilightHandler::computeIntendedState(Time currenttime){
    // return minimal value among the valid twilights.
    // TODO
    uint8_t min_twilight = 100;

     for (auto& b : BehaviourStore::getActiveBehaviours()){
        if(TwilightBehaviour * behave = dynamic_cast<TwilightBehaviour*>(b)){
            // cast to twilight behaviour succesful.
            if (behave->isValid(currenttime)){
                min_twilight = CsMath::min(min_twilight, behave->value());
            }
        }
    }

    return min_twilight;
}

uint8_t TwilightHandler::getValue(){
    return previousIntendedState;
}