#include <behaviour/cs_BehaviourConflictResolution.h>

/*
 * returns a 'relevance', lower is less relevant.
 *  - VacuouslyTrue is least relevant.
 *  - 'in Sphere' is less relevant than 'In room'.
 *  - 'presence' is less relevant than 'absence'.
 */
auto getRelevance(PresencePredicate::Condition c) {
    switch(c) {
        case PresencePredicate::Condition::VacuouslyTrue: return 0;
        case PresencePredicate::Condition::AnyoneInSphere: return 1;
        case PresencePredicate::Condition::NooneInSphere: return 2;
        case PresencePredicate::Condition::AnyoneInSelectedRooms: return 3;
        case PresencePredicate::Condition::NooneInSelectedRooms: return 4;
        default: return -1; // worst relevance: unknown enum value
    }
};

bool PresenceIsMoreRelevant(PresencePredicate::Condition lhs, PresencePredicate::Condition rhs) {
    return getRelevance(lhs) > getRelevance(rhs);
}

bool PresenceIsMoreRelevantOrEqual(PresencePredicate::Condition lhs, PresencePredicate::Condition rhs) {
    return getRelevance(lhs) >= getRelevance(rhs);
}

// Warning: be careful about integer underflow in the subtraction. Changed the signature to
// _signed_ integers in order to circumvent extra casting here.
bool FromUntilIntervalIsMoreRelevantOrEqual(
		int32_t lhsFrom, int32_t lhsUntil, int32_t rhsFrom, int32_t rhsUntil, int32_t currentTimeOfDay) {
	constexpr uint32_t secondsPerDay = 24 * 60 * 60;

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
    uint32_t lhsFromNormalized       = CsMath::mod(lhsFrom - currentTimeOfDay, secondsPerDay);
    uint32_t rhsFromNormalized       = CsMath::mod(rhsFrom - currentTimeOfDay, secondsPerDay);
	uint32_t lhsUntilNormalized      = CsMath::mod(lhsUntil - currentTimeOfDay, secondsPerDay);
	uint32_t rhsUntilNormalized      = CsMath::mod(rhsUntil - currentTimeOfDay, secondsPerDay);

	if (lhsFromNormalized > rhsFromNormalized) {
		// essentially means lhs_from is less far in the past than rhs_from.
		return true;
	}

	if (rhsFromNormalized == lhsFromNormalized && lhsUntilNormalized <= rhsUntilNormalized) {
		// essentially means lhs_until is nearer in the future than rhs_until.
		// (Be careful about the 'or equal' part.)
		return true;
	}

	return false;
}

bool FromUntilIntervalIsMoreRelevantOrEqual(Behaviour* lhs, Behaviour* rhs, Time currentTime) {
	return FromUntilIntervalIsMoreRelevantOrEqual(
			lhs->from(), lhs->until(), rhs->from(), rhs->until(), currentTime.timeOfDay());
}

bool FromUntilIntervalIsEqual(Behaviour* lhs, Behaviour* rhs) {
	return lhs != nullptr && rhs != nullptr && lhs->from() == rhs->from() && lhs->until() == rhs->until();
}
