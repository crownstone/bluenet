/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Okt 18, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <algorithm>

#ifdef HOST_TARGET
#include <limits>
#endif

namespace CsMath{

/**
 * returns lhs+rhs, clamping return values that would roll over
 * to the min/max values of lhs.
 */
template<class T, class U>
constexpr inline T SafeAdd (T lhs, U rhs) {
	T min = std::numeric_limits<T>::lowest();
	T max = std::numeric_limits<T>::max();

	if(rhs > 0) {
		return (lhs > max - rhs) ? max : T(lhs + rhs);
	}
	if(rhs < 0) {
		return (lhs < min - rhs) ? min : T(lhs + rhs);
	}
	return lhs;
}

/**
 * roll over safe variant of ++some_value;
 */
template<class T, class U=int>
constexpr inline T& Increase(T& obj, U diff=1) {
	obj = SafeAdd(obj, diff);
	return obj;
}

/**
 * roll over safe variant of --some_value;
 */
template<class T, class U=int>
constexpr inline T& Decrease(T& obj, U diff=1) {
	obj = SafeAdd(obj, -diff);
	return obj;
}

/**
 * Identical to the other decrease function.
 *
 * Use this variation if you get errors like:
 * 'cannot bind packed field to ...&' or
 * 'taking address of packed member ... may result in an unaligned pointer value'.
 */
template<class T, class M, class U=int>
constexpr inline decltype(auto) DecreaseMember(T& obj, M member, U diff=1) {
	obj.*member = SafeAdd(obj.*member, -diff);
	return obj.*member;
}

/**
 * Returns the canonical representation of [v] considered as element
 * of Z/mZ, regardless of any silly features C++ board thought that
 * would be better than actual math when using non-positive input.
 *
 * Return value is always in the half open interval [0,m).
 * For any values v,n and m, mod(v,m) == mod(v+ n*m, m) holds.
 *
 * Template implemented for integral types.
 */
template <class T, class S>
auto mod(T v, S m) -> decltype(v % m) {
	if (m == 0) {
		return v;
	}
	if (m < 0) {
		// flip [m] doesn't change the value.
		return mod(v, -m);
	}
	if (v < 0) {
		// flip [v] twice won't change it either, but be careful not to return
		// [m] instead of 0 when val is a multiple of m.
		if (auto neg_v = mod(-v, m)) {
			// hidden recursion in if clause will only go one level deep ;)
			return m - neg_v;
		}
		else {
			return 0;
		}
	}
	// m positive, v non-negative, yay C++ actually works in that case.
	return v % m;
}

template <class T, class S>
auto min(T l, S r) {
	return l < r ? l : r;
}

template <class T, class S>
auto max(T l, S r) {
	return l > r ? l : r;
}

/**
 * Returns:
 *  - lower if value <= lower
 *  - upper if value >= upper
 *  - value else.
 */
template <class V, class L, class U>
auto clamp(V value, L lower, U upper) {
	return min(max(value, lower), upper);
}

/**
 * Represents an interval by two unsigned integers [base, base + diff].
 * (base + diff doesn't have to be representable in the current type,
 * the interval represented will wrap around to 0)
 */
template <class T, class S = T>
class Interval {
private:
	T low, high;
	bool invert;

public:
	Interval(T base, S diff, bool inv = false) : low(base), high(base + (inv ? -diff : diff)), invert(inv) {
		// notes:
		// - addition base+diff is allowed to overflow/underflow.
		// - having a different type S for diff allows negative
		//   values without conflicting type resolutions.
		if ((diff < 0) ^ invert) {
			std::swap(low, high);
		}
	}

	T lowerbound() { return low; }
	T upperbound() { return high; }

	// E.g.
	// Interval<uint8_t> i(200,100)
	// i.contains(0) == true.
	// considers this interval as half open (lower closed) so that the return value is
	// true if [val] is enclosed in the interval or equal to the lower boundary.
	bool contains(T val) {
		return low < high                             // reversed interval?
					   ? (low <= val && val < high)   // nope, both must hold
					   : (low <= val || val < high);  // yup, only one can hold
	}

	// considers this interval as open ended so that the return value is
	// true if [val] is enclosed in the interval or on the boundary.
	bool ClosureContains(T val) {
		return low < high                              // reversed interval?
					   ? (low <= val && val <= high)   // nope, both must hold
					   : (low <= val || val <= high);  // yup, only one can hold
	}

	// considers this interval as open ended so that the return value is
	// true if [val] is strictly enclosed in the interval.
	bool InteriorContains(T val) {
		return low < high                            // reversed interval?
					   ? (low < val && val < high)   // nope, both must hold
					   : (low < val || val < high);  // yup, only one can hold
	}
};

/**
 * Rounds a numeric value.
 */
template <class T>
T round(float val) {
	// since numeric types are implicitly converted from float by flooring them,
	// adding 0.5 results in conventional rounding.
	return T(val + 0.5);
}
} // namespace CsMath
