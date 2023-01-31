/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <presence/cs_PresenceDescription.h>
#include <test/cs_TestAccess.h>

#include <bitset>

template <>
class TestAccess<PresenceStateDescription> {
public:
	uint64_t _bitmask;

	TestAccess() { reset(); }

	void reset() { _bitmask = 0; }

	PresenceStateDescription get() { return {_bitmask}; }

	static std::ostream& toStream(std::ostream& out, PresenceStateDescription& obj) {
		return out << "{"
				   << "_bitmask: " << std::bitset<64>(obj._bitmask) << "}";
	}

	static std::ostream& toStreamVerbose(std::ostream& out, PresenceStateDescription p) {
		int i = 0;
		out << "rooms: {";
		for (auto bitmask = p.getBitmask(); bitmask != 0; bitmask >>= 1) {
			out << (i ? ", " : "") << i;
			i++;
		}
		out << "}";
		return out;
	}
};

std::ostream& operator<<(std::ostream& out, PresenceStateDescription& obj) {
	return TestAccess<PresenceStateDescription>::toStream(out, obj);
}
