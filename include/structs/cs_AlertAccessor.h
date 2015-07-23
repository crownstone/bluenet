/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 15, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

enum AlertType {
	ALERT_SIMPLE           = 0,
	ALERT_EMAIL            = 1,
	ALERT_NEWS             = 2,
	ALERT_CALL             = 3,
	ALERT_MISSED_CALL      = 4,
	ALERT_SMS              = 5,
	ALERT_VOICE_MAIL       = 6,
	ALERT_SCHEDULE         = 7,
	ALERT_PRIORITY         = 8,
	ALERT_INSTANT_MESSAGE  = 9,

	ALERT_TEMP_LOW         = 100,
	ALERT_TEMP_HIGH        = 101
};

/* The new alert struct can be casted to/from a uint16_t
 *
 */
struct __attribute__((__packed__)) new_alert_t {
	uint8_t type;
	uint8_t num;
};

/* The alert notifications control point can be casted to/from a uint16_t
 *
 */
struct __attribute__((__packed__)) alert_control_point_t {
	uint8_t command;
	uint8_t type;
};

//class AlertAccessor : public BufferAccessor {
//
//};
