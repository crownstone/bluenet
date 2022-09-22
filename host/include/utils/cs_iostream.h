/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <behaviour/cs_BehaviourStore.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceCondition.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <utils/date.h>

#include <iostream>
#include <type_traits>
#include <optional>

/**
 * Allows streaming DayOfWeek objects to std::cout and other streams.
 */
std::ostream& operator<<(std::ostream& out, DayOfWeek t);

/**
 * Allows streaming TimeOfDay objects to std::cout and other streams.
 */
std::ostream& operator<<(std::ostream& out, TimeOfDay t);

/**
 * Allows streaming Time objects to std::cout and other streams.
 */
std::ostream& operator<<(std::ostream& out, Time t);

/**
 * Allows streaming std::optional<T> objects to std::cout and other streams, given T is a type for
 * which the relevant operator<< has been implemented.
 */
template<class T>
std::ostream & operator<< (std::ostream &out, std::optional<T> p_opt) {
    if (p_opt ){
    	return out << "{" << (std::is_unsigned<T>::value ? +p_opt.value() : p_opt.value()) << "}";
    } else {
    	return out << "std::nullopt";
    }
}
