/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 6, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

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
#include <time/cs_DayOfWeek.h>
#include <time/cs_TimeOfDay.h>
#include <util/cs_WireFormat.h>

#include <optional>

class TwilightBehaviour : public Behaviour {
public:
	typedef std::array<uint8_t, WireFormat::size<Behaviour>()> SerializedDataType;

	virtual ~TwilightBehaviour() = default;
	TwilightBehaviour(
			uint8_t intensity, uint8_t profileid, DayOfWeekBitMask activedaysofweek, TimeOfDay from, TimeOfDay until);

	TwilightBehaviour(SerializedDataType arr);
	SerializedDataType serialize();

	virtual uint8_t* serialize(uint8_t* outbuff, size_t max_size) override;
	virtual size_t serializedSize() const override;

	virtual void print() override;

	virtual Type getType() const override { return Type::Twilight; }

	// Because of the definition of isValid(PresenceStateDescription) in this class
	// the base class function with the same name is shadowed. This using statement
	// reintroduces the function name in this class's scope.
	using Behaviour::isValid;
};
