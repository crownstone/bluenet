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

    /**
     * When swSwitch is locked, only CMD_SWITCH_LOCKED events will be handled.
     * Else events may alter the intended states and subsequently trigger an 
     * actual state change.
     */
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
     * - if swSwitch doesn't allow switching, nothing happens, else,
     * - when overrideState has a value, swSwitch is set to that value, else,
     * - when behaviourState has a value, swSwitch is set to that value, else,
     * - nothing happens.
     * 
     * This method will clear the overrideState when it matches
     * the behaviourState, unless the switch is locked.
     */
    void updateState();


    /**
     * Triggers an updateState() call on all handled events and adjusts
     * at least one of behaviourState or overrideState.
     */
    void handleStateIntentionEvents(event_t & evt);
    
    /**
     * Tries to set source as owner of the switch.
     * Returns true on success, false if switch is already owned by a different source, and given source does not overrule it.
     */
    bool checkAndSetOwner(cmd_source_t source);

    /**
     * Which source claimed the switch.
     *
     * Until timeout, nothing with a different source can set the switch.
     * Unless that source overrules the current source.
     */
    cmd_source_t _source = cmd_source_t(CS_CMD_SOURCE_NONE);
    uint32_t _ownerTimeoutCountdown = 0;
};