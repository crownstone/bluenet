/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <presence/cs_PresencePredicate.h>
#include <time/cs_SystemTime.h>
#include <util/cs_WireFormat.h>

PresencePredicate::PresencePredicate(Condition c, PresenceStateDescription presence)
		: _condition(c), _presence(presence) {}

PresencePredicate::PresencePredicate(std::array<uint8_t, 9> arr)
		: PresencePredicate(
				static_cast<Condition>(arr[0]),
				PresenceStateDescription(WireFormat::deserialize<uint64_t>(arr.data() + 1, 8))) {}

bool PresencePredicate::isTrue(PresenceStateDescription presence) {
	switch (_condition) {
		case Condition::VacuouslyTrue: return true;
		case Condition::AnyoneInSelectedRooms: return (presence.getBitmask() & _presence.getBitmask()) != 0;
		case Condition::NooneInSelectedRooms: return (presence.getBitmask() & _presence.getBitmask()) == 0;
		case Condition::AnyoneInSphere: return presence.getBitmask() != 0;
		case Condition::NooneInSphere: return presence.getBitmask() == 0;
	}

	return false;
}

bool PresencePredicate::requiresPresence() const {
	return _condition == Condition::AnyoneInSelectedRooms || _condition == Condition::AnyoneInSphere;
}

bool PresencePredicate::requiresAbsence() const {
	return _condition == Condition::NooneInSelectedRooms || _condition == Condition::NooneInSphere;
}

PresencePredicate::SerializedDataType PresencePredicate::serialize() {
	SerializedDataType result;
	std::copy_n(std::begin(WireFormat::serialize(static_cast<uint8_t>(_condition))), 1, std::begin(result) + 0);
	std::copy_n(
			std::begin(WireFormat::serialize(static_cast<uint64_t>(_presence.getBitmask()))),
			8,
			std::begin(result) + 1);
	return result;
}
