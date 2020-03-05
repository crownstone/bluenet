/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_TwilightHandler.h>
#include <behaviour/cs_TwilightBehaviour.h>
#include <behaviour/cs_BehaviourStore.h>

#include <test/cs_Test.h>
#include <time/cs_SystemTime.h>
#include <drivers/cs_Serial.h>

#define LOGTwilightHandler LOGnone

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

    if (! time.isValid()) {
        LOGTwilightHandler("Current time invalid, twilight update returns false");
        return false;
    }

    uint8_t intendedState = computeIntendedState(time);
    if (previousIntendedState == intendedState) {
        return false;
    }

    previousIntendedState = intendedState;

    TEST_PUSH_EXPR_D(this,"previousIntendedState",previousIntendedState);
    return true;
}

uint8_t TwilightHandler::computeIntendedState(Time currenttime){
    constexpr uint32_t seconds_per_day = 24*60*60;

	// return the value of the last started valid twilight
    uint32_t now = currenttime.timestamp();
    uint32_t winning_from_tod;
    uint8_t winning_value = 0xff;



    for (auto& b : BehaviourStore::getActiveBehaviours()) {
        if (TwilightBehaviour * behave = dynamic_cast<TwilightBehaviour*>(b)) {
            // cast to twilight behaviour succesful.
            if (behave->isValid(currenttime)) {
            	bool ok_to_overwrite_winning_value = true;

            	if(winning_value != 0xff){
            		// previous value found, conflict resolution
            		uint32_t candidate_from_tod = behave->from();

            		if ( CsMath::mod(candidate_from_tod - now, seconds_per_day) <
            			 CsMath::mod(winning_from_tod   - now, seconds_per_day) ) {
            				 // we want candidate.from() > winning.from() when overwriting.
            				 // This equation only holds when both from() values are on the
            				 // same day. This is enforced by subtracting currenttime and modding
            				 // by seconds_per_day before comparison.

            				 // reaching here, winning.from() is closer in the past than
            				 // candidate.from() so don't overwrite.
            				 ok_to_overwrite_winning_value = false;
            		}

            	}

            	// update best
            	if(ok_to_overwrite_winning_value){
            		winning_value = behave->value();
            		winning_from_tod = behave->from();
            	}
            }
        }
    }

    // if no winning_value is found at all, return 100.
    return winning_value == 0xff ? 100 : winning_value;
}

uint8_t TwilightHandler::getValue(){
    return previousIntendedState;
}
