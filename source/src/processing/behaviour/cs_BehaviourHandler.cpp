/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <processing/behaviour/cs_BehaviourHandler.h>
#include <processing/behaviour/cs_BehaviourStore.h>
#include <processing/behaviour/cs_Behaviour.h>

#include <presence/cs_PresenceDescription.h>

#include <time/cs_SystemTime.h>
#include <time/cs_TimeOfDay.h>

#include <common/cs_Types.h>

#include "drivers/cs_Serial.h"

#define LOGBehaviourHandler LOGnone

void BehaviourHandler::handleEvent(event_t& evt){
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
    TimeOfDay time = SystemTime::now();
   PresenceStateDescription presence = 0xff; // everyone present as dummy value.
    
   LOGBehaviourHandler("BehaviourHandler::update %02d:%02d:%02d",time.h(),time.m(),time.s());

    auto intendedState = computeIntendedState(time, presence);
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

std::optional<uint8_t> BehaviourHandler::computeIntendedState(
       TimeOfDay currentTime, 
       PresenceStateDescription currentPresence){
    std::optional<uint8_t> intendedValue = {};
    
    for (const auto& b : BehaviourStore::getActiveBehaviours()){
        if (b && b->isValid(currentTime, currentPresence)){
            if (intendedValue){
                if (b->value() != intendedValue.value()){
                    // found a conflicting behaviour
                    // TODO(Arend): add more advance conflict resolution according to document.
                    LOGd("conflicting behaviours found");
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
