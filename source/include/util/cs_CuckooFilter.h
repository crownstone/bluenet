/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 02 Apr., 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 *
 * This library is forked from the public [github
 * repository](https://github.com/jonahharris/libcuckoofilter) and initially added to bluenet on
 * 19-02-2021. Code has been extensively refactored for idiomatic use of C++ and many implementation
 * details have changed e.g. to fix implicit narrowing/widening of integers, large recursion and
 * type punning in allocations.
 *
 * Those changes have been made by the Crownstone Team and fall under the project license mentioned
 * above.
 *
 * The original code and the extent to which is required by applicable law is left under its
 * original license included below and is attributed to the original author Jonah H. Harris
 * <jonah.harris@gmail.com>.
 *
 * The MIT License
 *
 * Copyright (c) 2015 Jonah H. Harris
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <protocol/cs_CuckooFilterStructs.h>
#include <util/cs_FilterInterface.h>

/**
 * A CuckooFilter is a probabilistic datatype is made for approximate membership queries.
 *
 * This implementation consists of the POD data structure `cuckoo_filter_data_t` and a
 * 'view class' `Cuckoofilter`, which is used to manipulate the data struct.
 *
 * The cuckoo_filter_data_t struct is not aware of its size. That must be managed by the owning
 * scope, which can query the required size in bytes for the given amount of fingerprints through
 * the CuckooFilter class.
 *
 * The 'elements' that one can `add` to a `CuckooFilter` are of type `cuckoo_key_t` which are
 * simply sequences of bytes. To check if an element is contained in a filter, refer to the
 * `contains` methods.
 *
 * CuckooFilter data contents are stored in an array of fingerprints which is segmented into
 * 'buckets'. These buckets are used for indexing, and although for an element, the bucket index and
 * fingerprint are deterministic, the index inside the bucket is dependent on order of insertion.
 *
 * More details can be found in `https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf`.
 * "Cuckoo Filter: Better Than Bloom" by Bin Fan, Dave Andersen, and Michael Kaminsky
 */
class CuckooFilter : public FilterInterface {
public:
	// -------------------------------------------------------------
	// Interface methods.
	// -------------------------------------------------------------

	bool isValid() override;

	bool contains(const void* key, size_t keyLengthInBytes) override {
		return contains(getExtendedFingerprint(key, keyLengthInBytes));
	}

	/**
	 * Total number of bytes this instance occypies.
	 * Use this function instead of sizeof for this class to take the buffer into account.
	 */
	constexpr size_t size() { return size(bucketCount(), _data->nestsPerBucket); }

	// -------------------------------------------------------------
	// These might be useful for fingerprints coming from the mesh.
	// -------------------------------------------------------------
	bool add(cuckoo_fingerprint_t finger, cuckoo_index_t bucketIndex) {
		return add(getExtendedFingerprint(finger, bucketIndex));
	}

	bool remove(cuckoo_fingerprint_t finger, cuckoo_index_t bucketIndex) {
		return remove(getExtendedFingerprint(finger, bucketIndex));
	}

	bool contains(cuckoo_fingerprint_t finger, cuckoo_index_t bucketIndex) {
		return contains(getExtendedFingerprint(finger, bucketIndex));
	}

	// -------------------------------------------------------------
	// these might be useful for stuff that comes in from commands
	// -------------------------------------------------------------

	bool add(cuckoo_key_t key, size_t keyLengthInBytes) { return add(getExtendedFingerprint(key, keyLengthInBytes)); }

	bool remove(cuckoo_key_t key, size_t keyLengthInBytes) {
		return remove(getExtendedFingerprint(key, keyLengthInBytes));
	}


	/**
	 * Reduces a key (element) to a compressed fingerprint, consisting of
	 * the fingerprint of the key and its associated primary position in the fingerprint array.
	 */
	cuckoo_compressed_fingerprint_t getCompressedFingerprint(cuckoo_key_t key, size_t keyLengthInBytes);

	// -------------------------------------------------------------
	// Init/deinit like stuff.
	// -------------------------------------------------------------

	/**
	 * Wraps a data struct into a CuckooFilter object
	 */
	CuckooFilter(cuckoo_filter_data_t* data) : _data(data) {}

	/**
	 * Use this with loads of care, _data is never checked to be non-nullptr in this class.
	 */
	CuckooFilter() : _data(nullptr) {}

	/**
	 * Memsets the bucket_array to 0x00 and sets victim to 0.
	 * No changes are made to size.
	 */
	void clear();

	/**
	 * Sets the size parameters and then  clear()s this object so that the bucket_array is
	 * filled with precisely enough 0x00s to represent an empty filter.
	 *
	 * @param[in] bucket_count_log2 number of buckets allocated will be rounde up to nearest
	 * 		power of 2 because of theoretical requirements.
	 */

	void init(cuckoo_index_t bucketCount, cuckoo_index_t nestsPerBucket);

	// -------------------------------------------------------------
	// Sizing helpers
	// -------------------------------------------------------------

	static constexpr size_t fingerprintCount(size_t bucketCount, cuckoo_index_t nestsPerBucket) {
		return bucketCount * nestsPerBucket;
	}

	/**
	 * Size of the byte buffer in bytes.
	 */
	static constexpr size_t bufferSize(size_t bucketCount, cuckoo_index_t nestsPerBucket) {
		return fingerprintCount(bucketCount, nestsPerBucket) * sizeof(cuckoo_fingerprint_t);
	}

	/**
	 * Total number of bytes a CuckooFilter with the given parameters would occupy.
	 *
	 * As per cppreference.com:
	 *   "sizeof, and the assignment operator ignore the flexible array member."
	 *   https://en.cppreference.com/w/c/language/struct
	 */
	static constexpr size_t size(cuckoo_index_t bucketCount, cuckoo_index_t nestsPerBucket) {
		return sizeof(cuckoo_filter_data_t) + bufferSize(bucketCount, nestsPerBucket);
	}

	/**
	 * Actual bucket count value may be bigger than a cuckoo_index_t can hold.
	 */
	constexpr size_t bucketCount() { return 1 << _data->bucketCountLog2; }

	constexpr size_t bufferSize() { return bufferSize(bucketCount(), _data->nestsPerBucket); }


private:
	// -------------------------------------------------------------
	// Compile time settings
	// -------------------------------------------------------------

	static constexpr size_t MAX_KICK_ATTEMPTS = 100;

	// -------------------------------------------------------------
	// Run time variables
	// -------------------------------------------------------------

	cuckoo_filter_data_t* _data;

	// -------------------------------------------------------------
	// ----- Private methods -----
	// -------------------------------------------------------------

	/**
	 * Reduces a key (element) to an extended fingerprint, consisting of
	 * the fingerprint of the key and its associated positions in the fingerprint array.
	 *
	 * Note: bucketCount _must_ be a power of two. (which is validated at construction/init)
	 */
	cuckoo_extended_fingerprint_t getExtendedFingerprint(cuckoo_key_t key, size_t keyLengthInBytes);

	/**
	 * Expands a fingerprint and the index where it is placed into an extended fingerprint,
	 * by computing the alternative index in the fingerprint array.
	 *
	 * Note: bucketCount _must_ be a power of two. (which is validated at construction/init)
	 */
	cuckoo_extended_fingerprint_t getExtendedFingerprint(cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucketIndex);

	/**
	 * A crc16 hash of this filters current contents (_data).
	 */
	cuckoo_fingerprint_t filterHash();

	/**
	 * Hashes the given key into a fingerprint.
	 */
	cuckoo_fingerprint_t hashToFingerprint(cuckoo_key_t key, size_t keyLengthInBytes);

	/**
	 * Hashes the given key to obtain an (untruncated) bucket index.
	 */
	cuckoo_fingerprint_t hashToBucket(cuckoo_key_t key, size_t keyLengthInBytes);

	/**
	 * Returns a reference to the fingerprint at the given coordinates.
	 */
	cuckoo_fingerprint_t& lookupFingerprint(cuckoo_index_t bucketIndex, cuckoo_index_t fingerIndex) {
		return _data->bucketArray[(bucketIndex * _data->nestsPerBucket) + fingerIndex];
	}

	/**
	 * Returns true if there was an empty space in the bucket and placement
	 * was successful, returns false otherwise.
	 *
	 * Does _not_ check for duplicates.
	 */
	bool addFingerprintToBucket(cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucketIndex);

	/**
	 * Returns true if found and removed, returns false if not found.
	 */
	bool removeFingerprintFromBucket(cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucketIndex);

	/*
	 * Puts the fingerprint in the filter, moving aside anything that is in the way unless it takes
	 * more than max_kick_attempts. return true on success, false when max kick attempts was
	 * reached. This doesn't check if the given fingerprint is already contained in the filter.
	 */
	bool move(cuckoo_extended_fingerprint_t entryToInsert);

	// -------------------------------------------------------------
	// Core methods for manipulating this->data
	// -------------------------------------------------------------

	bool add(cuckoo_extended_fingerprint_t efp);
	bool remove(cuckoo_extended_fingerprint_t efp);
	bool contains(cuckoo_extended_fingerprint_t efp);
};
