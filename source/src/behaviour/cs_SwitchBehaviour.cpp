/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_SwitchBehaviour.h>

#include <logging/cs_Logger.h>
#include <time/cs_SystemTime.h>
#include <util/cs_WireFormat.h>

#include <algorithm>

#define LOGBehaviour_V LOGnone

SwitchBehaviour::SwitchBehaviour(
		uint8_t intensity,
		uint8_t profileid,
		DayOfWeekBitMask activeDaysOfWeek,
		TimeOfDay from,
		TimeOfDay until,
		PresenceCondition presencecondition) :
	Behaviour(Behaviour::Type::Switch, intensity, profileid, activeDaysOfWeek, from, until),
	presenceCondition(presencecondition)
{
}

SwitchBehaviour::SwitchBehaviour(std::array<uint8_t, 1+26> arr) : 
	Behaviour(          WireFormat::deserialize<Behaviour>(         arr.data() +  0, 14)),
	presenceCondition(  WireFormat::deserialize<PresenceCondition>( arr.data() + 14, 13)) {
}

SwitchBehaviour::SerializedDataType SwitchBehaviour::serialize() {
	SerializedDataType result;
	std::copy_n(std::begin(Behaviour::serialize()),  WireFormat::size<Behaviour>(),            std::begin(result) + 0);
	std::copy_n(std::begin(WireFormat::serialize(presenceCondition)), WireFormat::size<PresenceCondition>(),    std::begin(result) + 14);

	return result;
}

uint8_t* SwitchBehaviour::serialize(uint8_t* outbuff, size_t max_size) {
	const auto size = SwitchBehaviour::serializedSize();

	if (max_size < size) {
		return 0;
	}

	return std::copy_n(std::begin(SwitchBehaviour::serialize()), size, outbuff);
}


size_t SwitchBehaviour::serializedSize() const {
	return WireFormat::size<SwitchBehaviour>();
}

bool SwitchBehaviour::requiresPresence() { 
	return presenceCondition.pred.requiresPresence();
}
bool SwitchBehaviour::requiresAbsence() {
	return presenceCondition.pred.requiresAbsence();
}

bool SwitchBehaviour::isValid(Time currentTime, PresenceStateDescription currentPresence) {
	return isValid(currentTime) && isValid(currentPresence);
}

bool SwitchBehaviour::gracePeriodForPresenceIsActive() {
	if (prevInRoomTimeStamp) {
		return CsMath::Interval(
				SystemTime::up(),
				presenceCondition.timeOut,
				true)
		.ClosureContains(*prevInRoomTimeStamp);
	}
	return false;
}

bool SwitchBehaviour::isValid(PresenceStateDescription currentPresence) {
	LOGBehaviour_V("isValid(presence) called");
	if (!requiresPresence() && !requiresAbsence()) {
		LOGBehaviour_V("vacuously true");
		return true;
	}
	if (requiresPresence()) {
		if (_isValid(currentPresence)) {
			// 9-1-2020 TODO Bart @ Arend: this relies on isValid(presence) to be called often.
			prevInRoomTimeStamp = SystemTime::up();
			LOGBehaviour_V("return true");
			return true;
		}
		if (prevInRoomTimeStamp) {
			//    		presenceCondition.timeOut = 20;
			if (gracePeriodForPresenceIsActive()) {
				// presence invalid but we're in the grace period.
				LOGBehaviour_V("left room(s) within time out: %d in [%d %d]", *prevInRoomTimeStamp, SystemTime::up() - presenceCondition.timeOut, SystemTime::up() );
				return true;
			}
			else {
				// fell out of grace, lets delete prev val.
				LOGBehaviour_V("left room(s) timed out: %d in [%d %d]", *prevInRoomTimeStamp, SystemTime::up() - presenceCondition.timeOut, SystemTime::up() );
				prevInRoomTimeStamp.reset();
				return false;
			}
		}
		// Not valid, and not in timeout period.
		LOGBehaviour_V("return false");
		return false;
	}
	if (requiresAbsence()) {
		bool notInRoom = _isValid(currentPresence);
		if (!notInRoom) {
			// 9-1-2020 TODO Bart @ Arend: this relies on isValid(presence) to be called often.
			prevInRoomTimeStamp = SystemTime::up();
		}
		if (prevInRoomTimeStamp) {
			//    		presenceCondition.timeOut = 20;
			if (gracePeriodForPresenceIsActive()) {
				LOGBehaviour_V("left room(s) within time out: %d in [%d %d]", *prevInRoomTimeStamp, SystemTime::up() - presenceCondition.timeOut, SystemTime::up() );
				return false;
			}
			else {
				LOGBehaviour_V("left room(s) timed out: %d in [%d %d]", *prevInRoomTimeStamp, SystemTime::up() - presenceCondition.timeOut, SystemTime::up() );
				prevInRoomTimeStamp.reset();
				return true;
			}
		}

		// When not in room, and not recently left the room.
		if (notInRoom) {
			LOGBehaviour_V("return true");
			return true;
		}

		// Not valid, and not in grace period.
		LOGBehaviour_V("return false");
		return false;
	}
	return true;
}

bool SwitchBehaviour::_isValid(PresenceStateDescription currentPresence) {
	return presenceCondition(currentPresence);
}

void SwitchBehaviour::print() {
#if CS_SERIAL_NRF_LOG_ENABLED == 0
	LOGd("SwitchBehaviour: %02d:%02d:%02d - %02d:%02d:%02d %3d%%, days(0x%x), presencetype(%d), timeout(%d) (%s)",
			from().h(), from().m(), from().s(),
			until().h(), until().m(), until().s(),
			activeIntensity,
			activeDays,
			presenceCondition.pred.cond,
			presenceCondition.timeOut,
			(isValid(SystemTime::now()) ? "valid" : "invalid")
	);
	presenceCondition.pred.RoomsBitMask.print();
#endif
}
