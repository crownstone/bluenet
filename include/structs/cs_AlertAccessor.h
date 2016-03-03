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

	//! Numbers above 100 are free to use. We'll use 7 bits so we can combine 7 different alarms
	ALERT_BLUENET_BASE     = 128, //! binary 1000 0000
//	ALERT_TEMP_LOW         = 129, //! binary 1000 0001
//	ALERT_TEMP_HIGH        = 130, //! binary 1000 0010
};

//! Usage: AlertType type = ALERT_BLUENET_BASE
//!        type |= 1 << ALERT_TEMP_LOW_POS;
//!        type |= 1 << ALERT_TEMP_HIGH_POS;
#define ALERT_TEMP_LOW_POS 0
#define ALERT_TEMP_HIGH_POS 1


/** The new alert struct can be casted to/from a uint16_t
 *
 */
struct __attribute__((__packed__)) new_alert_t {
	uint8_t type;
	uint8_t num;
};

/** The alert notifications control point can be casted to/from a uint16_t
 *
 */
struct __attribute__((__packed__)) alert_control_point_t {
	uint8_t command;
	uint8_t type;
};

//class AlertAccessor : public BufferAccessor {
//
//};
