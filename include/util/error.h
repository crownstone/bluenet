/**
 * Author: Christopher mason
 * Date: 21 Sep., 2013
 * License: TODO
 */
#ifndef _error_h
#define _error_h

#define NRF51_CRASH(x) __asm("BKPT"); \
	while (1) {}

#endif /* _error_h */
