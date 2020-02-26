/**
 * Author: Crownstone Team
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

extern "C" {
#include <util/cs_Syscalls.h>
}

#ifdef	NDEBUG

//! for release version ignore asserts
#define assert(expr, ...) \
	if (!(expr)) { \
		LOGe("%s", __VA_ARGS__); \
	}

#else

#define assert(expr, ...) \
	if (!(expr)) { \
		LOGe("%s", __VA_ARGS__); \
		_exit(1); \
	}

#endif
