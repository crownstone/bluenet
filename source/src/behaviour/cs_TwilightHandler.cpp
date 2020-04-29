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

bool FromUntilIntervalIsMoreRelevantOrEqual(
		int32_t lhs_from, int32_t lhs_until,
		int32_t rhs_from, int32_t rhs_until,
		int32_t current_tod){
	constexpr uint32_t seconds_per_day = 24*60*60;

	// Note: I think the current_tod actually isn't necessary: in practice,
	// we only compare behavious when they are both active, meaning that current_tod
	// is contained in the intersection of the intervals. All values satisfying that
	// restriction seem to have the same return value. (Needs to be rigidly proven I guess..)
	//
	// If this would be the case the parameter current_tod can be taken any value in this
	// intersection which simplifies the model significantly.

	// be careful about integer underflow in the subtraction. Changed the signature to _signed_
	// integers in order to circumvent extra casting here.
	uint32_t lhs_from_normalized = CsMath::mod(lhs_from - current_tod, seconds_per_day);
	uint32_t rhs_from_normalized = CsMath::mod(rhs_from - current_tod, seconds_per_day);

	uint32_t lhs_until_normalized = CsMath::mod(lhs_until - current_tod, seconds_per_day);
	uint32_t rhs_until_normalized = CsMath::mod(rhs_until - current_tod, seconds_per_day);

	if ( lhs_from_normalized > rhs_from_normalized ) {
			 // Essentially "lhs.from() > rhs.from()" means lhs.from is less far in the past.
			 // However, this equation only works as expected when both from() values are on the
			 // same day. The subtleties of midnight-rollover must be resolved by subtracting
			 // currenttime and modding by seconds_per_day before comparison.

//			LOGd("lhs is more specific: from() is less far in the past: lhs.f (%d) > rhs.f (%d) at time %d",
////					lhs_from_normalized, rhs_from_normalized, current_tod);
//					lhs_from, rhs_from, current_tod);
			return true;
	}

	if( rhs_from_normalized == lhs_from_normalized &&
		lhs_until_normalized <=	rhs_until_normalized){
		// play the same game on until() times when the from times are identical.
		// The until time that is nearest in the future should win now, so
		// the comparison sign reverses. Be careful about the 'or equal' part too.

//		LOGd("lhs is more specific: from()s are equal and until() is less far in the future: lhs.u (%d) <= rhs.u(%d) at %d",
//				//lhs_until_normalized, rhs_until_normalized, current_tod);
//				lhs_until, rhs_until, current_tod);
		return true;
	}
//
//	LOGd("FromUntilIntervalIsMoreRelevantOrEqual returns false, lhs/current [%d - %d], rhs/prevwinn [%d - %d]",
//			lhs_from_normalized, lhs_until_normalized,rhs_from_normalized, rhs_until_normalized);
//	LOGd("FromUntilIntervalIsMoreRelevantOrEqual input params, lhs/current [%d - %d], rhs/prevwinn [%d - %d]",
//				lhs_from, lhs_until,rhs_from, rhs_until);
//	LOGd("FromUntilIntervalIsMoreRelevantOrEqual current_tod %d", current_tod);
	return false;
}

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
			LOGi("TwilightHandler.isActive=%u", isActive);
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

    uint8_t winning_index = 0xff;
    uint8_t current_index = 0x0;

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
					LOGd("Twilight conflict res: overwriting winning value (index %d is better than %d) at %02d:%02d h",
							current_index, winning_index, currenttime.timeOfDay().h(), currenttime.timeOfDay().m());

					// what a silly edge case, there are two (or more) twilights with the exact same
					// boundary times. Let's use the minimum value in that case.
					if(candidate_from_tod == winning_from_tod &&
							candidate_until_tod == winning_until_tod){
						LOGd("exact interal match, using minimum twilight value");
						winning_value = CsMath::min(behave->value(), winning_value);
					} else {
						winning_value = behave->value();
					}

					// update winning value and winning from/until boundaries
					winning_from_tod = candidate_from_tod;
					winning_until_tod = candidate_until_tod;

					winning_index = current_index;
				}
            }
        }

        current_index++;
    }

    // if no winning_value is found at all, return 100.
    return winning_value == 0xff ? 100 : winning_value;
}

std::optional<uint8_t> TwilightHandler::getValue(){
    return currentIntendedState;
}
