/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <type_traits>

#include <logging/cs_Logger.h>


/**
 * A storage utility for objects of type Rec.
 *
 * Rec should implement:
 *   - IdType id();
 *   - bool isValid();
 *   - void invalidate();    // TODO:
 *
 * furthermore it must be default constructible.
 *
 */
template <class Rec, unsigned int Size>
class Store {
private:
	/**
	 * Current maximal number of valid records.
	 * if _currentSize is less than Size, range based for loops
	 * will loop over at most _currentSize items.
	 */
	uint16_t _currentSize = 0;

public:
	Rec _records[Size]    = {};

	Rec* begin() {
		return _records;
	}

	Rec* end() {
		return _records + _currentSize;
	}

	/**
	 * Identifies and simplifies the type returned by the id() method of Rec.
	 *
	 * Notes:
	 *  - remove_reference to avoid complications
	 *  - decltype doesn't dereference the nullptr
	 */
	typedef typename std::remove_reference<decltype(((Rec*)nullptr)->id())>::type IdType;


	/**
	 * invalidate all records.
	 */
	void clear() {
		_currentSize = 0;
		// TODO: @Bart practically we don't need to invalidate anything if size is shrunk to 0.
		for (auto& rec : *this) {
			rec.invalidate();
		}
	}

	/**
	 * linear search for the first record which has id() == id.
	 *
	 * Returns nullptr if no such element exists.
	 */
	constexpr Rec* get(const IdType& id) {
		for (auto& rec : *this) {
			if (rec.isValid() && rec.id() == id) {
				return &rec;
			}
		}
		return nullptr;
	}

	/**
	 * returns the first object `obj` in the store satisfying `p(obj) == true`.
	 * (useful if comparing to other things than `obj.id()`
	 *
	 * NOTE: this does _not_ check for isValid, because you may want to find the
	 * first invalid item for example!
	 */
	template<class UnaryPredicate>
	constexpr Rec* get(UnaryPredicate  p) {
		for (auto& obj : *this) {
			if (p(obj)) {
				return &obj;
			}
		}
		return nullptr;
	}

	/**
	 * Returns a pointer to the element in the store which minimizes
	 * the value function `val`.
	 *
	 * Only considers records with `isValid() == true`.
	 *
	 * ValueFunction is a function (or function like object) that
	 * takes an object of type `Rec` as argument and returns the value
	 * that should be minimized.
	 */
	template<class ValueFunction>
	constexpr Rec* getMin(ValueFunction getValue) {
		Rec* smallest = get([] (auto rec) { return rec.isValid();});

		if(smallest == nullptr){
			return nullptr;
		}

		for(Rec* obj = smallest + 1; obj != end(); obj++) {
			if(!obj.isValid()) {
				continue;
			}

			if (getValue(*obj) <  getValue(*smallest)) {
				smallest = obj;
			}
		}

		return smallest;
	}

	/**
	 * same as get, but will return pointer to an invalid element if
	 * such element exists, possibly incrementing _currentSize.
	 *
	 * Returns nullptr if full();
	 */
	Rec* getOrAdd(IdType id) {
		Rec* retval = nullptr;
		for (auto& rec : *this) {
			if (!rec.isValid()) {
				retval = &rec;
			}
			else if (rec.id() == id) {
				return &rec;
			}
		}

		if(retval != nullptr)  {
			// not found, but encountered an invalid record in the loop.
			return retval;
		}

		return addAtEnd();
	}

	/**
	 * returns a pointer to the first invalid record.
	 *
	 * Returns nullptr if such element doesn't exist.
	 */
	Rec* add() {
		for (auto& rec : *this) {
			if (!rec.isValid()) {
				return &rec;
			}
		}
		return nullptr;
	}

	/**
	 * increases size of the store if possible and returns
	 * the entry at the end(), after invalidating.
	 */
	Rec* addAtEnd() {
		if(_currentSize < Size) {
			// still have space, increment size and return ref to last index.
			Rec* retval = &_records[_currentSize++]; // must postcrement
			retval->invalidate();
			return retval;
		}

		return nullptr;
	}

	uint16_t size() {
		return _currentSize;
	}

	/**
	 * returns number of valid elements.
	 */
	uint16_t count() {
		uint16_t s = 0;
		for (auto& rec : *this ) {
			s += rec.isValid() ? 1 : 0;
		}
		return s;
	}

	/**
	 * returns true if all elements are occupied and valid.
	 */
	bool full() { return count() == Size; }


};
