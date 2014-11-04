/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+
 */

#ifndef _error_h
#define _error_h

#define NRF51_CRASH(x) __asm("BKPT"); \
	while (1) {}

#endif /* _error_h */
