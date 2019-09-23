/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/behaviour/cs_BehaviourHandler.h>
#include <processing/behaviour/cs_BehaviourStore.h>
#include <processing/behaviour/cs_Behaviour.h>

#include <events/cs_EventDispatcher.h>
#include <common/cs_Types.h>

#include "drivers/cs_Serial.h"


void BehaviourHandler::handleEvent(event_t& evt){
    // LOGd("BehaviourHandler::handleEvent %d", evt.type);
    switch(evt.type){
        case CS_TYPE::STATE_TIME:{
            LOGd("received time");
            if(evt.size == sizeof(TYPIFY(STATE_TIME))) {
				handleTime(reinterpret_cast<TYPIFY(STATE_TIME)*>(evt.data));
            }
        }
        // case CS_TYPE::SPHERE_ENTER{

        // }
        // case CS_TYPE::SPHERE_EXIT{

        // }
        default:{
            // ignore other events
            break;
        }
    }
}

void BehaviourHandler::handleTime(TYPIFY(STATE_TIME)* time){
    // update time cache
}

void BehaviourHandler::init(){
    LOGd("BehaviourHandler::init");
    EventDispatcher::getInstance().addListener(this);
}

bool BehaviourHandler::computeIntendedState(bool& intendedState){
    Behaviour::time_t currentTime = 0xffff;
    Behaviour::presence_data_t currentPresence = 0xff;

    bool foundValidBehaviour = false;
    bool firstIntendedState = false;

    for (const Behaviour& b : BehaviourStore::getActiveBehaviours()){
        if (b.isValid(currentTime, currentPresence)){
            if (foundValidBehaviour){
                if (b.value() != firstIntendedState){
                    // found a conflicting behaviour
                    return false;
                }
            } else {
                // found first valid behaviour
                firstIntendedState = b.value();
                foundValidBehaviour = true;
            }
        }
    }

    if(foundValidBehaviour){
        // only set the ref parameter when no conflict was found
        intendedState = firstIntendedState;
        return true;
    }

    return false;
}