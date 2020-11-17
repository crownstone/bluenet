/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#ifndef SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_
#define SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_

#include <events/cs_EventListener.h>

class NearestCrownstoneTracker : public EventListener {
public:
	void init() {}

	void handleEvent(event_t& evt);
};

#endif /* SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_ */
