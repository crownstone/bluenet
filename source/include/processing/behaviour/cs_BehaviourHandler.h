/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <processing/behaviour/cs_Behaviour.h>


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

    void handleTime(TYPIFY(STATE_TIME)* time);

    /**
     * Given current time/presence, query the behaviourstore and check
     * if there any valid ones. 
     * 
     * Returns false when no valid behaviour is found, or more than one
     * valid behaviours contradicted eachother.
     * Returns true if a valid behaviour is found.
     * In this case intendedState contains the desired state value.
     */
    public: bool computeIntendedState(bool& intendedState, 
        Behaviour::time_t currenttime, 
        Behaviour::presence_data_t currentpresence);
};