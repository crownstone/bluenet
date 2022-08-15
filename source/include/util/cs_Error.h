/**
 * Author: Crownstone Team
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

// These macros can end up in C code, so use the C logger.
#include <logging/cs_CLogger.h>

#ifdef NDEBUG
// For release version ignore asserts
#define assert(expr, message) \
	if (!(expr)) {            \
		CLOGe("%s", message); \
	}

#else

#include <util/cs_Syscalls.h>

#define assert(expr, message) \
	if (!(expr)) {            \
		CLOGe("%s", message); \
		_exit(1);             \
	}

#endif
