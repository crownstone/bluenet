/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 25, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <presence/cs_PresenceCondition.h>
#include <util/cs_WireFormat.h>
#include <logging/cs_Logger.h>

PresenceCondition::PresenceCondition(PresencePredicate p, uint32_t t) : 
		predicate(p), timeOut(t) {}

PresenceCondition::PresenceCondition(SerializedDataType arr): 
		PresenceCondition(
				WireFormat::deserialize<PresencePredicate>(arr.data() + 0, 9),
				WireFormat::deserialize<uint32_t>(         arr.data() + 9, 4)) {
}

size_t PresenceCondition::serializedSize() const {
	return WireFormat::size<PresenceCondition>();
}

PresenceCondition::SerializedDataType PresenceCondition::serialize() {
	SerializedDataType result;
	std::copy_n(std::begin(WireFormat::serialize(predicate)),    9, std::begin(result) + 0);
	std::copy_n(std::begin(WireFormat::serialize(timeOut)), 4, std::begin(result) + 9);

	return result;
}

uint8_t* PresenceCondition::serialize(uint8_t* outbuff, size_t maxSize) {
	if (maxSize != 0) {
		if (outbuff == nullptr || maxSize < serializedSize()) {
			return outbuff;
		}
	}

	auto serialized_repr = serialize();

	return std::copy_n (
			std::begin (serialized_repr),
			WireFormat::size<PresenceCondition>(),
			outbuff );
}

bool PresenceCondition::isTrue(PresenceStateDescription currentPresence) {
	return predicate.isTrue(currentPresence);
}
