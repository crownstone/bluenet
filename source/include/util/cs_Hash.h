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
 * Note: previousFletcherHash = 0 indicates a fresh new hash
 * 
 * Note: data will be padded with 0x00 if len%2 != 0, so if an array
 *   uint8_t dat[]; 
 * has last byte equal to 0x00 and len is positive, even then:
 *   Fletcher(dat,sizeof(dat)) == Fletcher(dat, sizeof(dat)-1)
 * 
 * As the Fletcher32 hash is effectively a uint16_t sized block stream hash, the computation
 * of a hash of large amounts of data can be split. * 
 * 
 * Warning: this only holds as long as the lengths of intermediate 
 * data chunks are a multiple of 2, each call to Fletcher(...) will round the len
 * parameter up to a multiple of 2 and padd the data with 0x00 if achieve that.
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
uint32_t Fletcher(const uint8_t* const data, const size_t len, uint32_t previousFletcherHash = 0);

/**
 * @brief Calculates a djb2 hash of given data.
 *
 * See http://www.cse.yorku.ca/~oz/hash.html for implementation details
 *
 * @param[in] Pointer to the data.
 * @param[in] Size of the data.
 * @retval    The hash.
 */
inline uint16_t Djb2(const uint8_t* data, const uint16_t size) {
	uint16_t hash = 5381;
	for (int i=0; i<size; ++i) {
		hash = ((hash << 5) + hash) + data[i];
	}
	return hash;
}
