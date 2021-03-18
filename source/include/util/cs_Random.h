/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 23 Feb, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <third/random/random.h>
#include <third/random/seed.h>
#include <third/random/state.h>

/**
 * Crownstones pseudo random number generator wrapper. Usefull for generating
 * sequences of pseudo random numbers that are identical across devices in the mesh.
 */
class RandomGenerator {
	private:
	Msws::State _state;

	public:
	/**
	 * The given seed need not be very high quality. The constructor will
	 * ensure that the internal state is set up to acceptable values,
	 * deterministically generated from the given seed parameter.
	 */
	RandomGenerator(uint32_t seed) : _state(Msws::seed(seed)) {}

	/**
	 * Generates a new random number and updates the internal state.
	 */
	uint32_t rand() {
		return Msws::msws(_state);
	}

	uint32_t operator()() {
		return rand();
	}
};
