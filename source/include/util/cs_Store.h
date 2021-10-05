/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * A storage utility for objects of type Rec.
 *
 * Rec should implement:
 *   - KeyType key();
 *   - bool isValid();
 *   - void invalidate();
 *
 * furthermore it must be default constructible.
 *
 */
template <class Rec, unsigned int Size>
class Store {
private:
	Rec _records[Size]    = {};

public:
	/**
	 * The type of object on which the records are checked for equality.
	 * (ref removed to avoid complications)
	 */
	typedef std::remove_reference<decltype(Rec::key())> KeyType;

	/**
	 * invalidate all records.
	 */
	void clear() {
		for (auto& rec : _record) {
			rec.invalidate();
		}
	}

	/**
	 * linear search for the first record which has key() == k.
	 *
	 * Returns nullptr if no such element exists.
	 */
	Rec* get(const KeyType& k) {
		for (auto& rec : _records) {
			if (rec.isValid() && rec.key() == k) {
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
	Rec* getOrAdd(KeyType k) {
		Rec* retval = nullptr;
		for (auto& rec : _records) {
			if (rec.isValid()) {
				if (rec.key() == k) {
					return &rec;
				}
			} else {
				if (retval == nullptr) {
					// store the first invalid record as possible return value.
					retval = &rec;
				}
			}
		}
		return retval;
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
};
