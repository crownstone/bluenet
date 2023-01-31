/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <behaviour/cs_SwitchBehaviour.h>
#include <test/cs_TestAccess.h>
#include <testaccess/cs_PresenceCondition.h>
#include <utils/cs_iostream.h>

#include <bitset>
#include <iomanip>
#include <iostream>
#include <ios>

template <>
class TestAccess<SwitchBehaviour> {
public:
	uint8_t intensity;
	uint8_t profileId;
	DayOfWeekBitMask activedaysofweek;
	TimeOfDay from;
	TimeOfDay until;
	TestAccess<PresenceCondition> presencecondition;

	enum PrintStyle {
		Verbose,
		AsBytes,
	};
	static PrintStyle _printStyle;

	TestAccess() : from(TimeOfDay::Sunrise()), until(TimeOfDay::Sunset()) { reset(); }

	void reset() {
		intensity        = 100;
		profileId        = 0;
		activedaysofweek = 0b01111111;
		from             = TimeOfDay::Sunrise();
		until            = TimeOfDay::Sunset();
		presencecondition.reset();
	}

	SwitchBehaviour get() { return {intensity, profileId, activedaysofweek, from, until, presencecondition.get()}; }

	// same as get, but allocate object on the heap.
	SwitchBehaviour* getNew() { return new auto(get()); }

	struct BehaviourStoreItem {
		uint8_t _index;
		SwitchBehaviour* _behaviour;
	};

	BehaviourStoreItem getItem(uint8_t index) { return BehaviourStoreItem{._index = index, ._behaviour = getNew()}; }

	static std::ostream& toStream(std::ostream& out, SwitchBehaviour& s) {
		if (_printStyle == Verbose) {
			out << "{"
				<< "from: " << s.from() << ", "
				<< "until: " << s.until() << ", "
				<< "value: " << std::setw(3) << std::setfill(' ') << +s.value() << ", "
				<< "profileId:" << +s.profileId << ", "
				<< "activeDays: " << std::bitset<8>(s.activeDays) << ", "
				<< "presenceCondition: " << s.presenceCondition << "}"
				<< "[gracePeriodForPresenceIsActive(): " << s.gracePeriodForPresenceIsActive() << "]";
		}

		if (_printStyle == AsBytes) {
			out << "{" << std::showbase << std::hex;
			bool firstByte = true;
			for (auto byte : s.serialized()) {
				out << (firstByte ? "" : ", ") << +byte;
				firstByte = false;
			}
			out << "}" << std::dec << std::noshowbase;
		}

		return out;
	}
};

TestAccess<SwitchBehaviour>::PrintStyle TestAccess<SwitchBehaviour>::_printStyle = Verbose;

/**
 * Allows streaming SwitchBehaviour objects to std::cout and other streams.
 * global definition forwards to TestAccess to elevate access rights for inspection.
 */
std::ostream& operator<<(std::ostream& out, SwitchBehaviour& s) {
	return TestAccess<SwitchBehaviour>::toStream(out, s);
}
