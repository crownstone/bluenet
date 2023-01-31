/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_Behaviour.h>
#include <behaviour/cs_ExtendedSwitchBehaviour.h>
#include <behaviour/cs_SwitchBehaviour.h>
#include <behaviour/cs_TwilightBehaviour.h>
#include <util/cs_WireFormat.h>

namespace WireFormat {

// -------------------- specialization for deserialize --------------------

template <>
uint8_t WireFormat::deserialize(uint8_t* data, size_t len) {
	// TODO(Arend): assert length
	return data[0];
}

template <>
uint32_t WireFormat::deserialize(uint8_t* data, size_t len) {
	// TODO(Arend): assert length
	return data[3] << (8 * 3) | data[2] << (8 * 2) | data[1] << (8 * 1) | data[0] << (8 * 0);
}

template <>
int32_t WireFormat::deserialize(uint8_t* data, size_t len) {
	// TODO(Arend): assert length
	return data[3] << (8 * 3) | data[2] << (8 * 2) | data[1] << (8 * 1) | data[0] << (8 * 0);
}

template <>
uint64_t WireFormat::deserialize(uint8_t* data, size_t len) {
	// TODO(Arend): assert length
	return static_cast<uint64_t>(data[7]) << (8 * 7) | static_cast<uint64_t>(data[6]) << (8 * 6)
		   | static_cast<uint64_t>(data[5]) << (8 * 5) | static_cast<uint64_t>(data[4]) << (8 * 4)
		   | static_cast<uint64_t>(data[3]) << (8 * 3) | static_cast<uint64_t>(data[2]) << (8 * 2)
		   | static_cast<uint64_t>(data[1]) << (8 * 1) | static_cast<uint64_t>(data[0]) << (8 * 0);
}

// -------------------- specialization for serialize --------------------

std::array<uint8_t, 1> serialize(const uint8_t& obj) {
	LOGWireFormat("serialize uint8_t");
	return {obj};
}

std::array<uint8_t, 4> serialize(const uint32_t& obj) {
	LOGWireFormat("serialize uint32_t");
	return {static_cast<uint8_t>(obj >> (0 * 8)),
			static_cast<uint8_t>(obj >> (1 * 8)),
			static_cast<uint8_t>(obj >> (2 * 8)),
			static_cast<uint8_t>(obj >> (3 * 8))};
}

std::array<uint8_t, 4> serialize(const int32_t& obj) {
	LOGWireFormat("serialize int32_t");
	return {static_cast<uint8_t>(obj >> (0 * 8)),
			static_cast<uint8_t>(obj >> (1 * 8)),
			static_cast<uint8_t>(obj >> (2 * 8)),
			static_cast<uint8_t>(obj >> (3 * 8))};
}

std::array<uint8_t, 8> serialize(const uint64_t& obj) {
	LOGWireFormat("serialize uint64_t");
	return {static_cast<uint8_t>(obj >> (0 * 8)),
			static_cast<uint8_t>(obj >> (1 * 8)),
			static_cast<uint8_t>(obj >> (2 * 8)),
			static_cast<uint8_t>(obj >> (3 * 8)),
			static_cast<uint8_t>(obj >> (4 * 8)),
			static_cast<uint8_t>(obj >> (5 * 8)),
			static_cast<uint8_t>(obj >> (6 * 8)),
			static_cast<uint8_t>(obj >> (7 * 8))};
}

}  // namespace WireFormat
