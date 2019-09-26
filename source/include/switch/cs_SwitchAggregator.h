/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "switch/cs_SwSwitch.h"
#include "events/cs_EventListener.h"

/**
 * Handler that aggregates events related to switching such as SwitchCraft,
 * Behaviours, Twilight and App/User side actions. Based on the incoming data
 * this object decides what state to set the SwSwitch to.
 */
class SwitchAggregator : public EventListener {
    
    void handleEvent(event_t& evt);

    private:
    typedef uint16_t SwitchState;
    SwSwitch swSwitch;

    SwitchState behaviourState;
    SwitchState userOverrideState;


};