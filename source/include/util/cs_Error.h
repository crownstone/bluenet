/**
 * Author: Crownstone Team
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once


#ifdef	NDEBUG
//! for release version ignore asserts
#define assert(expr, message) \
	if (!(expr)) { \
		LOGe("%s", message); \
	}

#else

extern "C" {
#include <util/cs_Syscalls.h>
}

#define assert(expr, message) \
	if (!(expr)) { \
		LOGe("%s", message); \
		_exit(1); \
	}

#endif
