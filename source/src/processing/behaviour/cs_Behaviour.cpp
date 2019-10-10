/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/behaviour/cs_Behaviour.h>
#include <drivers/cs_Serial.h>

Behaviour::Behaviour(time_t from, time_t until, 
      presence_data_t presencemask, uint8_t intendedState) :
    behaviourappliesfrom (from),
    behaviourappliesuntil (until),
    requiredpresencebitmask (presencemask),
    intendedStateWhenBehaviourIsValid (intendedState){
}

uint8_t Behaviour::value() const {
    return intendedStateWhenBehaviourIsValid;
}

Behaviour::time_t Behaviour::from() const {
    return behaviourappliesfrom; 
}

Behaviour::time_t Behaviour::until() const {
    return behaviourappliesuntil; 
}

bool Behaviour::isValid(time_t currenttime, presence_data_t currentpresence) const{
    return isValid(currenttime) && isValid(currentpresence);
}

bool Behaviour::isValid(time_t currenttime) const{
    return from() < until() // ensure proper midnight roll-over 
        ? (from() <= currenttime && currenttime < until()) 
        : (from() <= currenttime || currenttime < until());
}

bool Behaviour::isValid(presence_data_t currentpresence) const{
    return currentpresence & requiredpresencebitmask;
}