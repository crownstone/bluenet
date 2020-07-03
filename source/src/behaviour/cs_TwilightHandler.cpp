/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_TwilightHandler.h>
#include <behaviour/cs_TwilightBehaviour.h>
#include <behaviour/cs_BehaviourStore.h>
#include <behaviour/cs_BehaviourConflictResolution.h>

#include <test/cs_Test.h>
#include <time/cs_SystemTime.h>
#include <drivers/cs_Serial.h>

#define LOGTwilightHandler LOGd



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
        case CS_TYPE::STATE_BEHAVIOUR_SETTINGS: {
			behaviour_settings_t* settings = reinterpret_cast<TYPIFY(STATE_BEHAVIOUR_SETTINGS)*>(evt.data);
			isActive = settings->flags.enabled;
			LOGTwilightHandler("TwilightHandler.isActive=%u", isActive);
			TEST_PUSH_B(this, isActive);
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
    auto nextIntendedState = computeIntendedState(time);

    bool valuechanged = currentIntendedState != nextIntendedState;
    currentIntendedState = nextIntendedState;

    TEST_PUSH_EXPR_D(this,"currentIntendedState",(currentIntendedState? (int)currentIntendedState.value() : -1));
    return valuechanged;
}

std::optional<uint8_t> TwilightHandler::computeIntendedState(Time currenttime){
	if(!isActive){
		return {};
	}

	if(!currenttime.isValid()){
		return {};
	}

    uint32_t now_tod = currenttime.timeOfDay();
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

				bool ok_to_overwrite_winning_value;

            	if(winning_value == 0xff){
            		// no previous values, anything will do
            		ok_to_overwrite_winning_value = true;
            	} else {
            		// previous value found, conflict resolution
					ok_to_overwrite_winning_value = FromUntilIntervalIsMoreRelevantOrEqual(
							candidate_from_tod, candidate_until_tod,
							winning_from_tod, winning_until_tod,
							now_tod
					);
            	}

				if(ok_to_overwrite_winning_value){
					// what a silly edge case, there are two (or more) twilights with the exact same
					// boundary times. Let's use the minimum value in that case.
					if(candidate_from_tod == winning_from_tod &&
							candidate_until_tod == winning_until_tod){
						winning_value = CsMath::min(behave->value(), winning_value);
					} else {
						winning_value = behave->value();
					}

					// update winning value and winning from/until boundaries
					winning_from_tod = candidate_from_tod;
					winning_until_tod = candidate_until_tod;
				}
            }
        }
    }

    // if no winning_value is found at all, return 100.
    return winning_value == 0xff ? 100 : winning_value;
}

std::optional<uint8_t> TwilightHandler::getValue(){
    return currentIntendedState;
}
