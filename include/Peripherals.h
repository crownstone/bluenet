/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+
 */
#ifndef PERIPHERALS_T
#define PERIPHERALS_T

#include <stdint.h>

//#include "nordic_common.h"
struct peripheral_t {
//	uint8_t addr[ble_gap_addr_len];
	char addrs[28];
	uint16_t occurences;
	int8_t rssi;
};


//static std::vector<peripheral_t> _history;
#define history_size 10

extern peripheral_t _history[history_size];

extern uint8_t freeidx;


#endif

