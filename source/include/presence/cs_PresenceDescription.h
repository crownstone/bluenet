/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <drivers/cs_Serial.h>

#include <cstdint>

// this typedef is expected to expand to a full class when implementing 
// the presence detection.
// Current datatype: each bit indicates whether there is some user present
// in the room labeled with that bit index.
class PresenceStateDescription{
    private:
    uint64_t val;
    public:

    PresenceStateDescription(uint64_t v = 0) : val(v) {}

    // operator uint64_t(){return val;}
    operator uint64_t() const {return val;}

    friend bool operator== (
            const PresenceStateDescription& lhs, 
            const PresenceStateDescription& rhs) { 
        return lhs.val == rhs.val; 
    }
    template<class T>
    friend bool operator== (
            const PresenceStateDescription& lhs, 
            const T& rhs) { 
        return lhs == PresenceStateDescription(rhs); 
    }
    template<class T>
    friend bool operator== (
            const T& lhs,
            const PresenceStateDescription& rhs) { 
        return rhs == lhs; 
    }

    void setRoom(uint8_t index) {
        if(index < 64){
            val |= 1 << index;
        }
    }
    void clearRoom(uint8_t index) {
        if(index < 64){
            val &= ~(1 << index);
        }
    }

    void print(){
        [[maybe_unused]] uint32_t rooms[2] = {
            static_cast<uint32_t>(val >> 0 ),
            static_cast<uint32_t>(val >> 32)
        };
        LOGd("PresenceDesc(0x%04x 0x%04x)" , rooms[1], rooms[0]);
    }
};
