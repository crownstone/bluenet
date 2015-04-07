/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <util/cs_Utils.h>

namespace BLEutil {

// convert a short from LSB to MSB and vice versa
uint16_t convertEndian16(uint16_t val) {
	return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
}

// convert an integer from LSB to MSB and vice versa
uint32_t convertEndian32(uint32_t val) {
	return ((val >> 24) & 0xFF)
		 | ((val >> 8) & 0xFF00)
		 | ((val & 0xFF00) << 8)
		 | ((val & 0xFF) << 24);
}

}
