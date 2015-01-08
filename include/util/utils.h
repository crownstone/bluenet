/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 29, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "stdint.h"

// convert a short from LSB to MSB and vice versa
uint16_t convertEndian16(uint16_t val);

// convert an integer from LSB to MSB and vice versa
uint32_t convertEndian32(uint32_t val);

#define SIZEOF_ARRAY( a ) (sizeof( a ) / sizeof( a[ 0 ] ))

#endif /* UTILS_H_ */
