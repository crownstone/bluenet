/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 30, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <cstddef>

/**
 * Computes a Fletcher32 hash for the given data, interpreting it as a stream of
 * little endian uint16_t.
 * 
 * As the Fletcher32 hash is effectively a uint16_t sized block stream hash, the computation
 * of a hash of large amounts of data can be split.
 * 
 * Warning: this only holds as long as the lengths of intermediate 
 * data chunks are a multiple of 2.
 * 
 * E.g.
 * 
 * uint8_t part0[] = { __data__ }; size_t len0; // must be 0 % 2
 * uint8_t part1[] = { __data__ }; size_t len1; // must be 0 % 2
 * uint8_t part2[] = { __data__ }; size_t len2; // may be 1 % 2
 * 
 * uint32_t fletch;
 * fletch = Fletcher(part0,len0);
 * fletch = Fletcher(part1,len1,fletch);
 * fletch = Fletcher(part2,len2,fletch);
 *  
 */
uint32_t Fletcher(uint8_t* data, const size_t len, uint32_t previousFletcherHash = 0);
