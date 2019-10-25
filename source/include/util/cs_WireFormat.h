/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <processing/behaviour/cs_Behaviour.h>
#include <presence/cs_PresencePredicate.h>
#include <presence/cs_PresenceCondition.h>
#include <time/cs_TimeOfDay.h>

#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace WireFormat {

// data will be copied as few times as possible, but the constructed
// object is not emplaced over the [data] pointer passed as parameter.
template <class T>
T deserialize(uint8_t* data, size_t len);

// ========== Specializations =========

template<>
uint8_t WireFormat::deserialize(uint8_t* data, size_t len);

template<> 
uint32_t WireFormat::deserialize(uint8_t* data, size_t len);

template<> 
int32_t WireFormat::deserialize(uint8_t* data, size_t len);

template<> 
uint64_t WireFormat::deserialize(uint8_t* data, size_t len);

template<>
TimeOfDay WireFormat::deserialize(uint8_t* data, size_t len);

template<>
PresencePredicate WireFormat::deserialize(uint8_t* data, size_t len);

template<>
PresenceCondition WireFormat::deserialize(uint8_t* data, size_t len);

template<>
Behaviour WireFormat::deserialize(uint8_t* data, size_t len);


} // namespace WireFormat