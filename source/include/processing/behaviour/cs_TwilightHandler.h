/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <processing/behaviour/cs_SwitchBehaviour.h>

#include <optional>


class TwilightHandler : public EventListener {
    public:

    /**
     * Computes the twilight state of this crownstone based on
     * the stored behaviours, and then dispatches an event.
     * 
     * Events:
     * - STATE_TIME
     * - EVT_TIME_SET
     * - EVT_PRESENCE_MUTATION
     * - EVT_BEHAVIOURSTORE_MUTATION
     */
    virtual void handleEvent(event_t& evt);

    private:
    /**
     * Acquires the current time and presence information. 
     * Checks the intended state by looping over the active behaviours
     * and if the intendedState differs from previousIntendedState
     * dispatch an event to communicate a state update.
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
        TimeOfDay currenttime, 
        PresenceStateDescription currentpresence);

    std::optional<uint8_t> previousIntendedState = {};
};
