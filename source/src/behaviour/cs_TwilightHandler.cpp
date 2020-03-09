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

    uint32_t now = currenttime.timestamp();
    uint32_t winning_from_tod = 0;
    uint32_t winning_until_tod = 0;
    uint8_t winning_value = 0xff;

    // loop through all twilight behaviours searching for valid ones.
    for (auto& b : BehaviourStore::getActiveBehaviours()) {
        if (TwilightBehaviour * behave = dynamic_cast<TwilightBehaviour*>(b)) {
            // cast to twilight behaviour succesful.
            if (behave->isValid(currenttime)) {
            	// cache some values.
				uint32_t candidate_from_tod = behave->from();
				uint32_t candidate_until_tod = behave->until();

				// determine how conflict should be resolved
            	bool ok_to_overwrite_winning_value = true;

            	if(winning_value != 0xff){
            		// previous value found, conflict resolution

            		if ( CsMath::mod(candidate_from_tod - now, seconds_per_day) <
            			 CsMath::mod(winning_from_tod   - now, seconds_per_day) ) {
            				 // we essentially want "candidate.from() >= winning.from()" when overwriting.
            				 // However, this equation only works as expected when both from() values are on the
            				 // same day. The subtleties of midnight-rollover can be resolved by subtracting
            				 // currenttime and modding by seconds_per_day before comparison.

            				 // reaching here, winning.from() is strictly closer in the past than
            				 // candidate.from() so don't overwrite.
            				 ok_to_overwrite_winning_value = false;
            		}

            		if(candidate_from_tod == winning_from_tod){
            			// play the same game on until() times when the from times are identical.
            			if( CsMath::mod(candidate_until_tod - now, seconds_per_day) >
            				CsMath::mod(winning_until_tod - now, seconds_per_day)){
            				// the first invalidating behaviour should win now because that
            				// is the most narrow time-windowed behaviour.

            				// reaching here means winning.until() is strictly closer to now
            				// than the candidate, so we don't overwrite.
            				ok_to_overwrite_winning_value = false;
            			}
            		}
            	}

            	if(ok_to_overwrite_winning_value){
            		// update winning value
            		if(candidate_from_tod == winning_from_tod &&
            				candidate_until_tod == winning_until_tod){
            			// what a silly edge case, there are two (or more) twilights with the exact same
            			// boundary times. Let's use the minimum value in that case.
            			winning_value = CsMath::min(behave->value(), winning_value);
            			// note that winning_from/until times can be assigned in the same way as usual.
            			// as they are the same anyway.
            		} else {
						winning_value = behave->value();
            		}

            		// and update winning value from/until boundaries
            		winning_from_tod = candidate_from_tod;
            		winning_until_tod = candidate_until_tod;
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
