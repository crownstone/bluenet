/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 6, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_TwilightBehaviour.h>

TwilightBehaviour::TwilightBehaviour(
		uint8_t intensity,
		uint8_t profileid,
		DayOfWeekBitMask activedaysofweek,
		TimeOfDay from,
		TimeOfDay until) :
		Behaviour(Behaviour::Type::Twilight, intensity, profileid, activedaysofweek, from, until)
{
}

TwilightBehaviour::TwilightBehaviour(SerializedDataType arr)
: Behaviour(std::move(arr)) {

}

TwilightBehaviour::SerializedDataType TwilightBehaviour::serialize() {
	return Behaviour::serialize();
}

uint8_t* TwilightBehaviour::serialize(uint8_t* outbuff, size_t max_size) {
	const auto size = serializedSize();

	if (max_size < size) {
		return 0;
	}

	return std::copy_n(std::begin(serialize()), size, outbuff);
}

size_t TwilightBehaviour::serializedSize() const {
	return WireFormat::size<TwilightBehaviour>();
}

void TwilightBehaviour::print() {
#if CS_SERIAL_NRF_LOG_ENABLED == 0
	LOGd("TwilightBehaviour: type(%d) %02d:%02d:%02d - %02d:%02d:%02d %3d%%, days(%x), profile(%d)",
			static_cast<uint8_t>(typ),
			from().h(), from().m(), from().s(),
			until().h(), until().m(), until().s(),
			activeIntensity,
			activeDays,
			profileId
	);
#endif
}
