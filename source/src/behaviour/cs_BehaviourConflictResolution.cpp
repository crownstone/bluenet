#include <behaviour/cs_BehaviourConflictResolution.h>

bool FromUntilIntervalIsMoreRelevantOrEqual(
		int32_t lhs_from, int32_t lhs_until,
		int32_t rhs_from, int32_t rhs_until,
		int32_t current_tod){
	constexpr uint32_t seconds_per_day = 24*60*60;

	// Note: I think the current_tod parameter actually isn't necessary:
	// we only compare behavious when they are both active, meaning that current_tod
	// is contained in the intersection of the intervals. All values satisfying that
	// restriction seem to have the same return value. (Needs to be rigidly proven I guess..)
	//
	// If this would be the case the parameter current_tod can be taken any value in this
	// intersection which simplifies the model significantly.

	// First we normalize the from/until times so that current_tod corresponds to '0'.
	// That way, comparisons like lhs_from < rhs_from do not suffer from subtleties concerning
	// midnight roll over and unsigned integer over/underflow.
	//
	// Warning: be careful about integer underflow in the subtraction. Changed the signature to
	// _signed_ integers in order to circumvent extra casting here.
	uint32_t lhs_from_normalized = CsMath::mod(lhs_from - current_tod, seconds_per_day);
	uint32_t rhs_from_normalized = CsMath::mod(rhs_from - current_tod, seconds_per_day);
	uint32_t lhs_until_normalized = CsMath::mod(lhs_until - current_tod, seconds_per_day);
	uint32_t rhs_until_normalized = CsMath::mod(rhs_until - current_tod, seconds_per_day);

	if ( lhs_from_normalized > rhs_from_normalized ) {
			 // essentially means lhs_from is less far in the past than rhs_from.

			return true;
	}

	if( rhs_from_normalized == lhs_from_normalized &&
		lhs_until_normalized <=	rhs_until_normalized){
		// essentially means lhs_until is nearer in the future than rhs_until.
		// (Be careful about the 'or equal' part.)
		return true;
	}

	return false;
}

bool FromUntilIntervalIsMoreRelevantOrEqual(Behaviour* lhs, Behaviour* rhs, Time currentTime){
	return FromUntilIntervalIsMoreRelevantOrEqual(
			lhs->from(), lhs->until(),
			rhs->from(), rhs->until(),
			currentTime.timeOfDay());
}


bool FromUntilIntervalIsEqual(
		Behaviour* lhs, Behaviour* rhs){
	return 	lhs != nullptr &&
			rhs != nullptr &&
			lhs->from() == rhs->from() &&
			lhs->until() == rhs->until();
}

