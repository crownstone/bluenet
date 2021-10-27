/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <behaviour/cs_SwitchBehaviour.h>
#include <events/cs_EventListener.h>

#include <optional>


class BehaviourHandler : public EventListener {
public:
	/**
	 * Initialize this class:
	 * - Read settings from flash.
	 * - Start listening for events.
	 */
	void init();

    /**
     * Computes the intended behaviour state of this crownstone based on
     * the stored behaviours, and then dispatches an event for that.
     * 
     * Events:
     * - EVT_PRESENCE_MUTATION
     * - EVT_BEHAVIOURSTORE_MUTATION
     * - STATE_BEHAVIOUR_SETTINGS
     * - CMD_GET_BEHAVIOUR_DEBUG
     */
    void handleEvent(event_t& evt);

    /**
     * Acquires the current time and presence information. 
     * Checks and updates the currentIntendedState by looping over the active behaviours
     * 
     * Returns true if the currentIntendedState differs from the previousIntendedState.
     */
    bool update();

    /**
     * Returns currentIntendedState variable and updates the previousIntendedState
     * to currentIntendedState to match previousIntendedState.
     */
    std::optional<uint8_t> getValue();

    bool requiresPresence(Time t);
    bool requiresAbsence(Time t);

private:

    /**
     * Given current time/presence, query the behaviourstore and check
     * if there any valid ones. 
     * 
     * Returns an empty optional when this BehaviourHandler is inactive, or 
     * more than one valid behaviours contradicted eachother.
     * Returns a non-empty optional if a valid behaviour is found or
     * multiple agreeing behaviours have been found.
     * In this case its value contains the desired state value.
     * When no behaviours are valid at given time/presence the intended
     * value is 0. (house is 'off' by default)
     */
    std::optional<uint8_t> computeIntendedState(
        Time currenttime, 
        PresenceStateDescription currentpresence);

    void handleGetBehaviourDebug(event_t& evt);

    /**
     * Returns b, casted as switch behaviour if that cast is valid and
     * isValid(*) returns true.
     * Else, returns nullptr.
     */
    SwitchBehaviour* ValidateSwitchBehaviour(Behaviour* behave, Time currentTime,
       PresenceStateDescription currentPresence);

    /**
     * The last value returned by getValue.
     */
    std::optional<uint8_t> previousIntendedState = {};

    /**
     *  The last value that was updated by the update method.
     */ 
    std::optional<uint8_t> currentIntendedState = {};

    /******
     * setting this to false will result in a BehaviourHandler that will
     * not have an opinion about the state anymore (getValue returns std::nullopt).
     */
    bool isActive = true;
};
