#include <behaviour/cs_Behaviour.h>
#include <presence/cs_PresencePredicate.h>
#include <time/cs_Time.h>

/**
 * Returns true if lhs is strictly more relevant than rhs.
 *  - VacuouslyTrue is least relevant.
 *  - 'in Sphere' is less relevant than 'In room'.
 *  - 'presence' is less relevant than 'absence'..
 */
bool PresenceIsMoreRelevant(PresencePredicate lhs, PresencePredicate rhs);

/**
 * Returns true if;
 * - lhs_from is closer in the past than rhs_from; or
 * - lhs_until is closer in the future than rhs_until and the lhs_from == rhs_from; or
 * - lhs_from == rhs_from and lhs_until == rhs_until.
 */
bool FromUntilIntervalIsMoreRelevantOrEqual(
		int32_t lhs_from, int32_t lhs_until, int32_t rhs_from, int32_t rhs_until, int32_t current_tod);

/**
 * Wrapper for raw int32_t values.
 */
bool FromUntilIntervalIsMoreRelevantOrEqual(Behaviour* lhs, Behaviour* rhs, Time currentTime);

/**
 * Returns false if either of lhs and rhs is nullptr, else
 * returns true iff both from() and until() values match.
 */
bool FromUntilIntervalIsEqual(Behaviour* lhs, Behaviour* rhs);
