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
#include <time/cs_SystemTime.h>

#include "drivers/cs_Serial.h"


void BehaviourHandler::handleEvent(event_t& evt){
    // LOGd("BehaviourHandler::handleEvent %d", evt.type);
    switch(evt.type){
        case CS_TYPE::STATE_TIME:
        case CS_TYPE::EVT_PRESENCE_MUTATION:
            update();
        default:{
            // ignore other events
            break;
        }
    }
}

void BehaviourHandler::update(){
    // TODO(Arend 24-09-2019): get presence from scheduler
    bool nextState = false;
    TimeOfDay time = SystemTime::now();
    Behaviour::presence_data_t presence = 0xff; // everyone present as dummy value.
    
    LOGd("BehaviourHandler::update %02d:%02d:%02d",time.h(),time.m(),time.s());

    if(computeIntendedState(nextState, time, presence)){
        // TODO(Arend 24-09-2019): send nextState to SwitchController
        event_t behaviourStateChange(
            CS_TYPE::EVT_BEHAVIOUR_SWITCH_STATE,
            &nextState,
            TypeSize(CS_TYPE::EVT_BEHAVIOUR_SWITCH_STATE)
        );
        EventDispatcher::getInstance().dispatch(behaviourStateChange);
    }
}

bool BehaviourHandler::computeIntendedState(bool& intendedState,
        Behaviour::time_t currentTime, 
        Behaviour::presence_data_t currentPresence){
    
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

// void TestBehaviourHandler(uint32_t time, uint8_t presence){
// 	bool intendedState = false;
// 	if(_behaviourHandler.computeIntendedState(intendedState,
// 			time,
// 			presence)
// 	){
// 		LOGd("TestBehaviourHandler(%d,%d) -> valid: %d intent: %d",
// 			time,presence,
// 			true, intendedState? 1: 0);
// 	} else {
// 		LOGd("TestBehaviourHandler(%d,%d) -> valid: %d intent: %d",
// 			time,presence,
// 			false, intendedState? 1 : 0);
// 	}
// }

// void TestCases(){
//     // add some test behaviours to the store
//     Behaviour morningbehaviour(
//         9*60*60,
//         11*60*60,
//         0b10101010,
//         true
//     );
//     Behaviour morningbehaviour_overlap(
//         8*60*60,
//         10*60*60,
//         0b10101010,
//         true
//     );
//     Behaviour morningbehaviour_conflict(
//         10*60*60,
//         12*60*60,
//         0b10101010,
//         false
//     );
//     Behaviour eveningbehaviour(
//         22*60*60,
//         23*60*60,
//         0b10101010,
//         true
//     );

//     saveBehaviour(morningbehaviour,0);
//     saveBehaviour(eveningbehaviour,1);
//     saveBehaviour(morningbehaviour_overlap,2);
//     saveBehaviour(morningbehaviour_conflict,3);

//     // check boundaries
//     TestBehaviourHandler(22*60*60-1, 0xff); // 0 0
//     TestBehaviourHandler(22*60*60,   0xff); // 1 1
//     TestBehaviourHandler(23*60*60-1, 0xff); // 1 1
//     TestBehaviourHandler(23*60*60,   0xff); // 0 0

//     // check confict and overlap
//     TestBehaviourHandler( 8*60*60, 0xff); // 1 1 // 1 behaviour
//     TestBehaviourHandler( 9*60*60, 0xff); // 1 1 // 2 behaviours agree
//     TestBehaviourHandler(10*60*60, 0xff); // 0 x // 2 behaviours conflict
//     TestBehaviourHandler(11*60*60, 0xff); // 1 0 // 1 behaviour edge case
//     TestBehaviourHandler(12*60*60, 0xff); // 0 0 // 0 behaviours
// }