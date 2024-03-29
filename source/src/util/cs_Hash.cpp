/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 30, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <util/cs_Hash.h>
#include <util/cs_Math.h>

// based on source(30-10-2019): https://en.wikipedia.org/wiki/Fletcher%27s_checksum
// adjusted to handle uint8_t arrays by padding with 0x00
uint32_t Fletcher(const uint8_t* const data8, const size_t len, uint32_t previousFletcherHash) {
	const uint16_t* data16 = reinterpret_cast<const uint16_t*>(data8);

	// optimization of amount of operator% calls below still holds given a previous
	// fletcher hash because the last operation always is a modulo operation.
	uint32_t c0            = previousFletcherHash >> 0 * 8 & 0xffff;
	uint32_t c1            = previousFletcherHash >> 2 * 8 & 0xffff;

	bool needsPadding      = (len % 2) != 0;
	size_t loopLen         = len / 2;  // (this rounds down when necessary)

	// 360 is chosen such that c0 and c1 are accumulated without overflow.
	// Delaying the % operator until the last possible moment reduced the
	// amount of calls to it.
	while (loopLen > 0) {
		// subtract 1 from length if it isn't even to prevent overflow.
		size_t blocklen = CsMath::min(360u, loopLen);

		for (size_t i = 0; i < blocklen; ++i) {
			c0 += *data16++;
			c1 += c0;
		}
		c0 = c0 % 0xffff;
		c1 = c1 % 0xffff;

		loopLen -= blocklen;
	}

	if (needsPadding) {
		// intentionally skipped the last byte of data in the loop above
		// padd with 0x00 to finish the has.
		c0 += data8[len - 1] | 0x00 << 8;
		c1 += c0;

		c0 = c0 % 0xffff;
		c1 = c1 % 0xffff;
	}

	// at this point the following holds:
	// c0 = \sum_{i=0}^{len} d_i \mod 0xffff
	// c1 = \sum_{i=0}^{len} (len + 1 - i) d_i) \mod 0xffff
	// (eventually can be used to parallelize the algorithm..)

	return (c1 << 16 | c0);
}
