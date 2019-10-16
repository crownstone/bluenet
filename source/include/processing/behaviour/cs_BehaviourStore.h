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

    class InterfaceB {
        /**
         * If expected_master_hash is not equal to the current
         * value that hash() would return, nothing happens
         * and 0xffffffff is returned. Else,
         * returns an index in range [0,MaxBehaviours) on succes, 
         * or 0xffffffff if it couldn't be saved.
         * 
         * A hash is computed and saved together with [b].
         */
         size_t save(Behaviour b, uint32_t expected_master_hash);

        /**
         * Replace the behaviour at [index] with [b], if the hash of the 
         * currently saved behaviour at [index] is not equal to 
         * [expected_hash] nothing will happen and false is returned.
         * if these hashes coincide, postcondition is identical to the
         * postcondition of calling save(b) when it returns [index].
         */
        bool replace(Behaviour b, uint32_t expected_hash, size_t index);

        /**
         * deletes the behaviour at [index], if the hash of the 
         * currently saved behaviour at [index] is not equal to 
         * [expected_hash] nothing will happen and false is returned,
         * else the behaviour is removed from storage.
         */
        bool remove(uint32_t expected_hash, size_t index);

        /**
         *  returns the stored behaviour at [index].
         */
        Behaviour get(size_t index);

        /**
         * returns a map with the currently occupied indices and the 
         * behaviours at those indices.
         */
        std::vector<std::pair<size_t,Behaviour>> get();

        /**
         * returns the hash of the behaviour at [index]. If no behaviour
         * is stored at this index, 0xffffffff is returned.
         */
        uint32_t hash(size_t index);

        /**
         * returns a hash value that takes all state indices into account.
         * this value is expected to change after any call to update/save/remove.
         * 
         * A (phone) application can compute this value locally given the set of 
         * index/behaviour pairs it expects to be present on the Crownstone.
         * Checking if this differs from the one received in the crownstone state message
         * enables the application to resync.
         */
        uint32_t hash();

    } interfaceB;


    static inline const std::array<std::optional<Behaviour>,MaxBehaviours>& getActiveBehaviours() {
        return activeBehaviours;
    }
};