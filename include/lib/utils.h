/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 29, 2014
 * License: LGPLv3+
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

// convert a short from LSB to MSB and vice versa
static inline uint16_t convertEndian(uint16_t val) {
	return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
}

// convert an integer from LSB to MSB and vice versa
static inline uint32_t convertEndian(uint32_t val) {
	return ((val >> 24) & 0xFF)
		 | ((val >> 8) & 0xFF00)
		 | ((val & 0xFF00) << 8)
		 | ((val & 0xFF) << 24);
}


#endif /* UTILS_HPP_ */
