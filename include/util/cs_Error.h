/**
 * Author: Christopher mason
 * Date: 21 Sep., 2013
 * License: TODO
 */
#pragma once

#define NRF51_CRASH(x) __asm("BKPT"); \
	while (1) {}
