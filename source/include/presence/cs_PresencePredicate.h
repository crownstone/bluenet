/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <logging/cs_Logger.h>
#include <presence/cs_PresenceDescription.h>

#include <array>

class PresencePredicate {
public:
	typedef std::array<uint8_t, 9> SerializedDataType;

	// user id restrictions?
	enum class Condition : uint8_t {
		VacuouslyTrue         = 0,
		AnyoneInSelectedRooms = 1,
		NooneInSelectedRooms  = 2,
		AnyoneInSphere        = 3,
		NooneInSphere         = 4
	};

	// private: DEBUG
	Condition _condition;
	PresenceStateDescription _presence;

public:
	PresencePredicate(Condition c, PresenceStateDescription presence);

	PresencePredicate(SerializedDataType arr);

	bool requiresPresence() const;
	bool requiresAbsence() const;

	SerializedDataType serialize();

	// parameter bit i is 1 whenever there is presence detected in the
	// room with index i.
	bool isTrue(PresenceStateDescription presence);

	void print() {
		uint64_t locationBitmask                      = _presence.getBitmask();

		[[maybe_unused]] uint32_t locationBitmasks[2] = {
				static_cast<uint32_t>(locationBitmask >> 0), static_cast<uint32_t>(locationBitmask >> 32)};
		LOGd("PresencePredicate(condition: %d presence: 0x%04x 0x%04x)",
			 static_cast<uint8_t>(_condition),
			 locationBitmasks[1],
			 locationBitmasks[0]);
	}
};
