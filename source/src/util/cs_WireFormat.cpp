/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <util/cs_WireFormat.h>

namespace WireFormat {
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

template<> 
inline uint64_t WireFormat::deserialize(uint8_t* data, size_t len){
    // assert(len == 8);
    return 
        static_cast<uint64_t>(data[7]) << 8*7 | 
        static_cast<uint64_t>(data[6]) << 8*6 | 
        static_cast<uint64_t>(data[5]) << 8*5 | 
        static_cast<uint64_t>(data[4]) << 8*4 |
        static_cast<uint64_t>(data[3]) << 8*3 | 
        static_cast<uint64_t>(data[2]) << 8*2 | 
        static_cast<uint64_t>(data[1]) << 8*1 | 
        static_cast<uint64_t>(data[0]) << 8*0;
}

template<>
inline PresencePredicate WireFormat::deserialize(uint8_t* data, size_t len){
    // assert(len == 9)
    std::array<uint8_t,9> d;
    std::copy_n(data, 9, d.begin());
    return PresencePredicate(d);
}

} // namespace WireFormat