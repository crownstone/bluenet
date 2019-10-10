/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <drivers/cs_Serial.h>
#include "switch/cs_SwSwitch.h"
#include "events/cs_EventListener.h"

#include <optional>

/**
 * Handler that aggregates events related to switching such as SwitchCraft,
 * Behaviours, Twilight and App/User side actions. Based on the incoming data
 * this object decides what state to set the SwSwitch to.
 */
class SwitchAggregator : public EventListener {
    public:
    static SwitchAggregator& getInstance();
    
    void init(SwSwitch&& s); // claims ownership over s.

    virtual void handleEvent(event_t& evt) override;
 
    /**
     * sets dimmer and relay to 0, disregarding all other state/intentions.
     */
    void developerForceOff();
    
    private:
    SwitchAggregator() = default;
    virtual ~SwitchAggregator() noexcept {};
    SwitchAggregator& operator= (const SwitchAggregator&) = delete;

    // when the swith aggregator is initialized with a board
    // that can switch, swSwitch contains that value.
    std::optional<SwSwitch> swSwitch;

    // the latest states requested by other parts of the system.
    std::optional<uint8_t> behaviourState = {};
    std::optional<uint8_t> overrideState = {};

    /**
     * Checks the behaviourState and overrideState,
     * to set the swSwitch to the desired value:
     * - when overrideState has a value, that value is used, else,
     * - when behaviourState has a value, that value is used, else
     * - no state change is made to swSwitch.
     * 
     * This method will clear the overrideState when it matches
     * the behaviourState, unless 
     */
    void updateState();
};