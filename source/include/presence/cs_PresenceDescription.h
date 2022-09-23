/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <logging/cs_Logger.h>
#include <util/cs_Utils.h>
#include <test/cs_TestAccess.h>

#include <cstdint>

#define LOGPresenceDescriptionDebug LOGvv

/**
 * Class that holds the presence of a profile.
 *
 * When the Nth bit is set, the profile is present at location N.
 */
class PresenceStateDescription {
    friend class TestAccess<PresenceStateDescription>;
private:
	uint64_t _bitmask;

public:
	PresenceStateDescription(uint64_t bitmask = 0) : _bitmask(bitmask) {}

	//	operator uint64_t() const { return _bitmask; }

	friend bool operator==(const PresenceStateDescription& lhs, const PresenceStateDescription& rhs) {
		return lhs._bitmask == rhs._bitmask;
	}

	void setLocation(uint8_t locationId) {
		// TODO: is this if even needed?
		if (locationId < sizeof(_bitmask) * 8) {
			CsUtils::setBit(_bitmask, locationId);
		}
	}

	uint64_t getBitmask() { return _bitmask; }

	void print() {
		[[maybe_unused]] uint32_t bitmasks[2] = {
				static_cast<uint32_t>(_bitmask >> 0), static_cast<uint32_t>(_bitmask >> 32)};
        LOGPresenceDescriptionDebug("PresenceDesc(0x%04x 0x%04x)", bitmasks[1], bitmasks[0]);
	}
};
