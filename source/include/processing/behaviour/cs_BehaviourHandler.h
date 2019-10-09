/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <processing/behaviour/cs_Behaviour.h>

#include <optional>


class BehaviourHandler : public EventListener {
    public:

    /**
     * Computes the intended state of this crownstone based on
     * the stored behaviours, and then dispatches a switch event
     * for that state.
     * 
     * Events:
     * - STATE_TIME
     * - EVT_PRESENCE_MUTATION
     */
    virtual void handleEvent(event_t& evt);

    private:
    /**
     * Acquires the current time and presence information. 
     * Checks the intended state by looping over the active behaviours
     * and if necessary dispatch an event to communicate a state update.
     */
    void update();

    /**
     * Given current time/presence, query the behaviourstore and check
     * if there any valid ones. 
     * 
     * Returns an empty optional when no valid behaviour is found, or 
     * more than one valid behaviours contradicted eachother.
     * Returns a non-empty optional if a valid behaviour is found or
     * multiple agreeing behaviours have been found.
     * In this case its value contains the desired state value.
     */
    std::optional<uint8_t> computeIntendedState(
        Behaviour::time_t currenttime, 
        Behaviour::presence_data_t currentpresence);
};