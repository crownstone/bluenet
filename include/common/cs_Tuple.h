/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <vector>
#include <cstdint>

/** A tuple is a vector with a templated type and a public constructor.
 * @param T templated element which goes into the vector
 */
template<typename T> class tuple : public std::vector<T> {
public:
	//! Default constructor
	tuple() {}
};

/** A fixed tuple is a vector with a templated type and a reserved capacity.
 * @param T templated type which goes into the vector
 * @param capacity Predefined capacity of the underlying std::vector.
 */
template<typename T, uint8_t capacity> class fixed_tuple : public tuple<T> {
public:
	//! Constructor reserves capacity in vector
	fixed_tuple<T, capacity>() : tuple<T>() {this->reserve(capacity);}

};


