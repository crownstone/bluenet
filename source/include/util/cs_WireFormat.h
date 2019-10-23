/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <presence/cs_PresencePredicate.h>

#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace WireFormat {

template <class T>
T deserialize(uint8_t* data, size_t len);

// ========== Specializations =========

template<> 
uint32_t WireFormat::deserialize(uint8_t* data, size_t len);

template<> 
int32_t WireFormat::deserialize(uint8_t* data, size_t len);

template<> 
uint64_t WireFormat::deserialize(uint8_t* data, size_t len);

template<>
PresencePredicate WireFormat::deserialize(uint8_t* data, size_t len);

} // namespace WireFormat