/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "events/cs_EventListener.h"


class BehaviourHandler : public EventListener {
    public:

    /**
     * Register at the EventDispatcher.
     */
    void init();

    /**
     * Computes the intended state of this crownstone based on
     * the stored behaviours, and then dispatches a switch event
     * for that state.
     * 
     * Events:
     * - sunrise/set.
     * - Time.
     * - OnEnter
     * - OnExit
     */
    virtual void handleEvent(event_t& evt);


    private:
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
    bool computeIntendedState(bool & intendedState);
};