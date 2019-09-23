/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/behaviour/cs_Behaviour.h>

Behaviour::Behaviour(time_t from, time_t until, 
      presence_data_t presencemask, bool intendedState) :
    behaviourappliesfrom (from),
    behaviourappliesuntil (until),
    requiredpresencebitmask (presencemask),
    intendedStateWhenBehaviourIsValid (intendedState){
}

bool Behaviour::value() const {
    return intendedStateWhenBehaviourIsValid;
}

bool Behaviour::isValid(time_t currenttime, presence_data_t currentpresence) const{
    return isValid(currenttime) && isValid(currentpresence);
}

bool Behaviour::isValid(time_t currenttime) const{
    return behaviourappliesfrom <= behaviourappliesuntil // ensure proper midnight roll-over 
        ? (behaviourappliesfrom <= currenttime && currenttime < behaviourappliesuntil) 
        : (behaviourappliesfrom <= currenttime || currenttime < behaviourappliesuntil);
}

bool Behaviour::isValid(presence_data_t currentpresence) const{
    return currentpresence & requiredpresencebitmask;
}