/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <behaviour/cs_BehaviourHandler.h>
#include <test/cs_TestAccess.h>

template <>
class TestAccess<BehaviourHandler> {
   public:
    static SwitchBehaviour* resolveSwitchBehaviour(
        BehaviourHandler& behaviourHandler,
        Time currentTime,
        PresenceStateDescription currentPresence) {
        return behaviourHandler.resolveSwitchBehaviour(currentTime, currentPresence);
    }

    /**
     * A very non-standard setup method. This needs to be upgraded to better match what bluenet
     * does.
     * @param behaviourHandler
     * @param presenceHandler
     * @param behaviourStore
     */
    static void setup(
        BehaviourHandler& behaviourHandler,
        PresenceHandler* presenceHandler,
        BehaviourStore* behaviourStore) {
        behaviourHandler._behaviourStore = behaviourStore;
        behaviourHandler._presenceHandler = presenceHandler;
    }
};
