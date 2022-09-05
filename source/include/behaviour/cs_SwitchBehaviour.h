/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <behaviour/cs_Behaviour.h>
#include <presence/cs_PresenceCondition.h>
#include <stdint.h>
#include <util/cs_WireFormat.h>

#include <optional>
#include <test/cs_TestAccess.h>
#include <logging/cs_Logger.h>

/**
 * Object that defines when a state transition should occur.
 *
 * It abstracts predicates such as:
 * "fade to 100% in 10 minutes, starting 30 minutes before sunrise, if anyone is in this room"
 */
class SwitchBehaviour : public Behaviour {
    friend class TestAccess<SwitchBehaviour>;
public:
	typedef std::array<uint8_t, WireFormat::size<Behaviour>() + WireFormat::size<PresenceCondition>()>
			SerializedDataType;

	virtual ~SwitchBehaviour() { LOGw("destroying switchbehahviour"); };

	SwitchBehaviour(
			uint8_t intensity,
			uint8_t profileid,
			DayOfWeekBitMask activedaysofweek,
			TimeOfDay from,
			TimeOfDay until,
			PresenceCondition presencecondition);

	SwitchBehaviour(SerializedDataType arr);
	SerializedDataType serialize();

	virtual uint8_t* serialize(uint8_t* outbuff, size_t max_size) override;
	virtual size_t serializedSize() const override;

	virtual Type getType() const override { return Type::Switch; }

	virtual void print();

	// =========== Semantics ===========

	// Returns true if time passed since between prevInRoomTimeStamp and current up time
	// is less than presenceCondition.timeout. Else, returns false.
	bool gracePeriodForPresenceIsActive();

	virtual bool requiresPresence() override;
	virtual bool requiresAbsence() override;

	/**
	 * Does the behaviour apply to the current situation?
	 * If from() == until() the behaviour isValid all day.
	 **/
	virtual bool isValid(Time currenttime, PresenceStateDescription currentpresence);

	// Presence description is cached in order to prevent
	// that the behaviour flickers when a user is on the border of two rooms.
	// (not there is a timeout in the presencehandler to check if the user hasn't disappeared,
	// but it tries to describe the location as accurately as possible. Thus, when a user is
	// detected in another room, the presence is immediately updated.)
	bool isValid(PresenceStateDescription currentpresence);  // cached version

	// Because of the definition of isValid(PresenceStateDescription) in this class
	// the base class function with the same name is shadowed. This using statement
	// reintroduces the function name in this class's scope.
	using Behaviour::isValid;

protected:
	// (serialized field)
	PresenceCondition presenceCondition;

private:
	// unserialized fields (runtime values)
	std::optional<uint32_t> prevInRoomTimeStamp = {};  // when was the last call to _isValid that returned true?

	// uncached version of ::isValid
	bool _isValid(PresenceStateDescription currentpresence);
};
