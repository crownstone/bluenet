/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 5, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef INCLUDE_CFG_CS_WHITELIST_H_
#define INCLUDE_CFG_CS_WHITELIST_H_

#include <stdint.h>

// Number of addresses in the white list
#define WHITELIST_LENGTH 3

// Remember that the bluetooth address is reversed!
static const uint8_t WhiteList[WHITELIST_LENGTH * BLE_GAP_ADDR_LEN] = {
		0x0C, 0x59, 0x5F, 0x7F, 0xBF, 0xE3,
		0xBE, 0xA0, 0xA7, 0x3F, 0x8F, 0xDF,
		0x7C, 0xFD, 0x98, 0x12, 0xB7, 0xC0,
};



#endif /* INCLUDE_CFG_CS_WHITELIST_H_ */
