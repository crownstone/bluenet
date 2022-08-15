/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <vector>

/** BaseClass
 */
template <uint8_t N = 1>
class BaseClass {
public:
	BaseClass() {
		for (uint8_t i = 0; i < N; ++i) {
			_initialized[i] = false;
		}
	}

	inline bool isInitialized(uint8_t i = 0) { return _initialized[i]; }

	inline void setInitialized(uint8_t i = 0) { _initialized[i] = true; }

	inline void setUninitialized(uint8_t i = 0) { _initialized[i] = false; }

private:
	bool _initialized[N];
};
