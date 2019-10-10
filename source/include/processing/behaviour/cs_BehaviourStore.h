/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <processing/behaviour/cs_Behaviour.h>

#include <array>
#include <optional>

/**
 * Keeps track of the behaviours that are active on this crownstone.
 */
class BehaviourStore : public EventListener {
    private:
    static constexpr size_t MaxBehaviours = 10;
    static std::array<std::optional<Behaviour>,MaxBehaviours> activeBehaviours;
    
    public:
    /**
     * handles events concerning updates of the active behaviours on this crownstone.
     */
    virtual void handleEvent(event_t& evt);

    /**
     * Stores the given behaviour [b] at given [index] in the activeBehaviours array.
     * 
     * Note: currently doesn't persist states.
     * 
     * Returns true on success, false if [index] is out of range.
     */
    bool saveBehaviour(Behaviour b, size_t index);

    static inline const std::array<std::optional<Behaviour>,MaxBehaviours>& getActiveBehaviours() {
        return activeBehaviours;
    }
};