/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <cfg/cs_Config.h>

#define DAILY_REPEAT_BIT_SUNDAYS    0
#define DAILY_REPEAT_BIT_MONDAYS    1
#define DAILY_REPEAT_BIT_TUESDAYS   2
#define DAILY_REPEAT_BIT_WEDNESDAYS 3
#define DAILY_REPEAT_BIT_THURSDAYS  4
#define DAILY_REPEAT_BIT_FRIDAYS    5
#define DAILY_REPEAT_BIT_SATURDAYS  6
#define DAILY_REPEAT_BIT_ALL_DAYS   7

#define SCHEDULE_TIME_TYPE_REPEAT      0
#define SCHEDULE_TIME_TYPE_DAILY       1
#define SCHEDULE_TIME_TYPE_ONCE        2

#define SCHEDULE_ACTION_TYPE_PWM       0
#define SCHEDULE_ACTION_TYPE_FADE      1
#define SCHEDULE_ACTION_TYPE_TOGGLE    2

/**
 * A schedule that is executed daily, on certain days of the week.
 */
struct __attribute__((__packed__)) schedule_time_daily_t {
	/**
	 * Only perform action on certain days these days of the week. Bitmask, see DAILY_REPEAT_*.
	 * Check against (1 << current_day_of_week)
	 * If (dayOfWeek & DAILY_REPEAT_ALL_DAYS), then the other bits are ignored.
	 */
	uint8_t dayOfWeekBitmask;
	uint8_t reserved;
};

struct __attribute__((__packed__)) schedule_time_repeat_t {
	uint16_t repeatTime; //! Repeat every X minutes. 0 is not allowed.
};

struct __attribute__((__packed__)) schedule_action_pwm_t {
	uint8_t pwm;
	uint8_t reserved[2];
};

struct __attribute__((__packed__)) schedule_action_fade_t {
	uint8_t pwmEnd;
	uint16_t fadeDuration; //! Number of seconds it takes to get to pwmEnd.
};

struct __attribute__((__packed__)) schedule_entry_t {
	uint8_t reserved;

	/**
	 * Combined time and action type.
	 * Defined as SCHEDULE_TIME_TYPE_.. + (SCHEDULE_ACTION_TYPE_.. << 4)
	 */
	uint8_t type;

	//! What to override. Bitmask, see SCHEDULE_OVERRIDE_*
	uint8_t overrideMask;

	/**
	 * Posix timestamp of next time this schedule should be executed.
	 * Set to 0 when this entry is empty.
	 */
	uint32_t nextTimestamp;
	union {
		schedule_time_repeat_t repeat;
		schedule_time_daily_t daily;
	};
	union {
		schedule_action_pwm_t pwm;
		schedule_action_fade_t fade;
	};
};

/**
 * A list of schedule entries.
 */
struct __attribute__((__packed__)) schedule_list_t {
	uint8_t size;
	schedule_entry_t list[MAX_SCHEDULE_ENTRIES];
};

/**
 * Sets the schedule list to the default.
 * This is a full schedule with cleared entries.
 */
constexpr void cs_schedule_list_set_default(schedule_list_t *schedule) {
	schedule->size = MAX_SCHEDULE_ENTRIES;
	for (int i=0; i<MAX_SCHEDULE_ENTRIES; ++i) {
		schedule->list[i].nextTimestamp = 0;
	}
}
