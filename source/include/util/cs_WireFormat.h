/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <cstddef>

namespace WireFormat {

template <class T>
inline T deserialize(uint8_t* data, size_t len);

// full specialization required to be in cpp file 
template<> 
inline uint32_t WireFormat::deserialize(uint8_t* data, size_t len){
    // assert(len == 4);
    return data[3] << 8*3 | data[2] << 8*2 | data[1] << 8*1 | data[0]  << 8*0;
}

template<> 
inline int32_t WireFormat::deserialize(uint8_t* data, size_t len){
    // assert(len == 4);
    return data[3] << 8*3 | data[2] << 8*2 | data[1] << 8*1 | data[0]  << 8*0;
}

} // namespace WireFormat

