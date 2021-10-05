/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <type_traits>

/**
 * A storage utility for objects of type Rec.
 *
 * Rec should implement:
 *   - IdType id();
 *   - bool isValid();
 *   - void clear();    // TODO:
 *
 * furthermore it must be default constructible.
 *
 */
template <class Rec, unsigned int Size>
class Store {
private:

public:
	Rec _records[Size]    = {};


	/**
	 * Identifies and simplifies the type returned by the id() method of Rec.
	 *
	 * Notes:
	 *  - remove_reference to avoid complications
	 *  - decltype doesn't dereference the nullptr
	 */
	typedef std::remove_reference<decltype(((Rec*)nullptr)->id())> IdType;

	/**
	 * invalidate all records.
	 */
	void clear() {
		for (auto& rec : _records) {
			rec.clear();
		}
	}

	/**
	 * linear search for the first record which has id() == id.
	 *
	 * Returns nullptr if no such element exists.
	 */
	Rec* get(const IdType& id) {
		for (auto& rec : _records) {
			if (rec.isValid() && rec.id() == id) {
				return &rec;
			}
		}
		return nullptr;
	}

	/**
	 * same as get, but will return pointer to an invalid element if
	 * such element exists.
	 *
	 * Returns nullptr if full();
	 */
	Rec* getOrAdd(IdType id) {
		Rec* retval = nullptr;
		for (auto& rec : _records) {
			if (rec.isValid()) {
				if (rec.id() == id) {
					return &rec;
				}
			}
			else {
				if (retval == nullptr) {
					retval = &rec;
				}
			}
		}
		return retval;
	}

	/**
	 * returns a pointer to the first invalid record.
	 *
	 * Returns nullptr if such element doesn't exist.
	 */
	Rec* add() {
		for (auto& rec : _records) {
			if (!rec.isValid()) {
				return &rec;
			}
		}
		return nullptr;
	}

	/**
	 * returns number of valid elements.
	 */
	uint16_t size() {
		uint16_t s = 0;
		for (auto& rec : _records ) {
			s += rec.isValid() ? 1 : 0;
		}
		return s;
	}

	/**
	 * returns true if all elements are valid.
	 */
	bool full() { return size() == Size; }


	Rec* begin() {
		return _records;
	}

	Rec* end() {
		return _records + Size;
	}
};
