/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <presence/cs_PresenceCondition.h>
#include <test/cs_TestAccess.h>
#include <testaccess/cs_PresencePredicate.h>

template <>
class TestAccess<PresenceCondition> {
public:
	TestAccess<PresencePredicate> predicate;
	uint32_t timeOut;

	TestAccess() { reset(); }

	void reset() {
		predicate.reset();
		timeOut = 60 * 5;
	}

	PresenceCondition get() { return {predicate.get(), timeOut}; }

	static std::ostream& toStream(std::ostream& out, PresenceCondition& obj) {
		return out << "{"
				   << "predicate: " << obj.predicate << ", "
				   << "timeOut: " << +obj.timeOut << "}";
	}
};

/**
 * Allows streaming PresenceCondition objects to std::cout and other streams.
 * global definition forwards to TestAccess to elevate access rights for inspection.
 */
std::ostream& operator<<(std::ostream& out, PresenceCondition& s) {
	return TestAccess<PresenceCondition>::toStream(out, s);
}
