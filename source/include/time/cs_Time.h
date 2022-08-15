/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <stdint.h>
#include <time/cs_DayOfWeek.h>
#include <time/cs_TimeOfDay.h>

class Time {
private:
	uint32_t posixTimeStamp;

public:
	Time(uint32_t posixTime) : posixTimeStamp(posixTime) {}

	uint32_t timestamp() { return posixTimeStamp; }

	bool isValid() { return posixTimeStamp != 0; }

	/**
	 * See: http://stackoverflow.com/questions/36357013/day-of-week-from-seconds-since-epoch
	 * With timestamp=0 = Thursday 1-January-1970 00:00:00
	 */
	DayOfWeek dayOfWeek() { return DayOfWeek(1 << ((posixTimeStamp / (60 * 60 * 24) + 4) % 7)); }

	TimeOfDay timeOfDay() { return TimeOfDay(TimeOfDay::BaseTime::Midnight, posixTimeStamp); }
};
