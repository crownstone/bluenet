#pragma once

#include <cstdlib>
#include <iostream>

#define assert(expr, message) \
	if (!(expr)) { \
		printf(message);\
		exit(-1); \
	}
