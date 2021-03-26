#pragma once

#include <third/cuckoo/CuckooFilterStructs.h>

/**
 * A CuckooFilter is a probabilistic datatype is made for approximate membership queries.
 *
 * This implementation consists of the POD data structure `cuckoo_filter_data_t` and a
 * 'view class' `Cuckoofilter`, which is used to manipulate the data struct.
 *
 * The cuckoo_filter_data_t struct is not aware of its size. That must be managed by the owning
 * scope, which can query the required size in bytes for the given amount of fingerprints through
 * the CuckooFilter class.
 */
class CuckooFilter {
   public:
	static constexpr size_t max_kick_attempts = 100;

   private:
	// -------------------------------------------------------------
	// Run time variables
	// -------------------------------------------------------------

	cuckoo_filter_data_t& data;

	/**
	 * Note: bucket_count _must_ be a power of two. (which is validated at construction/init)
	 */
	cuckoo_extended_fingerprint_t getExtendedFingerprint(cuckoo_key_t key, size_t key_length_in_bytes);

	/**
	 * Note: bucket_count _must_ be a power of two. (which is validated at construction/init)
	 */
	cuckoo_extended_fingerprint_t getExtendedFingerprint(cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucket_index);

	// -------------------------------------------------------------
	// ----- Private methods -----
	// -------------------------------------------------------------

	/**
	 * A hash of this filters current contents.
	 * TODO(Arend): incorporate victim and other settings.
	 */
	cuckoo_fingerprint_t filterhash();

	/**
	 * Hashes the given key (data) into a fingerprint.
	 */
	cuckoo_fingerprint_t hash(cuckoo_key_t key, size_t key_length_in_bytes);

	/**
	 * Returns a reference to the fingerprint at the given coordinates.
	 */
	cuckoo_fingerprint_t& lookupFingerprint(cuckoo_index_t bucket_number, cuckoo_index_t finger_index) {
		return data.bucket_array[(bucket_number * data.nests_per_bucket) + finger_index];
	}

	/**
	 * Returns true if there was an empty space in the bucket and placement
	 * was successful, returns false otherwise.
	 *
	 * Does _not_ check for duplicates.
	 */
	bool add_fingerprint_to_bucket(cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucket_number);

	/**
	 * Returns true if found and removed, returns false if not found.
	 */
	bool remove_fingerprint_from_bucket(cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucket_number);

	/*
	 * Puts the fingerprint in the filter, moving aside anything that is in the way unless it takes
	 * more than max_kick_attempts. return _OK or _FULL. This doesn't check if the given fingerprint
	 * is already contained in the filter.
	 */
	bool move(cuckoo_extended_fingerprint_t entry_to_insert);

	bool add(cuckoo_extended_fingerprint_t efp);
	bool remove(cuckoo_extended_fingerprint_t efp);
	bool contains(cuckoo_extended_fingerprint_t efp);

   public:
	// -------------------------------------------------------------
	// These might be useful for fingerprints coming from the mesh.
	// -------------------------------------------------------------
	bool add(cuckoo_fingerprint_t finger, cuckoo_index_t bucket_index) {
		return add(getExtendedFingerprint(finger, bucket_index));
	}

	bool remove(cuckoo_fingerprint_t finger, cuckoo_index_t bucket_index) {
		return remove(getExtendedFingerprint(finger, bucket_index));
	}

	bool contains(cuckoo_fingerprint_t finger, cuckoo_index_t bucket_index) {
		return contains(getExtendedFingerprint(finger, bucket_index));
	}

	// -------------------------------------------------------------
	// these might be useful for stuff that comes in from commands
	// -------------------------------------------------------------

	bool add(cuckoo_key_t key, size_t key_length_in_bytes) {
		return add(getExtendedFingerprint(key, key_length_in_bytes));
	}

	bool remove(cuckoo_key_t key, size_t key_length_in_bytes) {
		return remove(getExtendedFingerprint(key, key_length_in_bytes));
	}

	bool contains(cuckoo_key_t key, size_t key_length_in_bytes) {
		return contains(getExtendedFingerprint(key, key_length_in_bytes));
	}

	// -------------------------------------------------------------
	// Init/deinit like stuff.
	// -------------------------------------------------------------

	/**
	 * Memsets the bucket_array to 0x00 and sets victim to 0.
	 * No changes are made to size.
	 */
	void clear();

	static constexpr size_t fingerprintCount(cuckoo_index_t bucket_count, cuckoo_index_t nests_per_bucket) {
		return bucket_count * nests_per_bucket;
	}

	/**
	 * Size of the byte buffer in bytes.
	 */
	static constexpr size_t bufferSize(cuckoo_index_t bucket_count, cuckoo_index_t nests_per_bucket) {
		return fingerprintCount(bucket_count, nests_per_bucket) * sizeof(cuckoo_fingerprint_t);
	}

	constexpr size_t bufferSize() { return bufferSize(data.bucket_count, data.nests_per_bucket); }

	/**
	 * Total number of bytes a CuckooFilter with the given parameters would occupy.
	 *
	 * As per cppreference.com:
	 *   "sizeof, and the assignment operator ignore the flexible array member."
	 *   https://en.cppreference.com/w/c/language/struct
	 */
	static constexpr size_t size(cuckoo_index_t bucket_count, cuckoo_index_t nests_per_bucket) {
		return sizeof(CuckooFilter) + bufferSize(bucket_count, nests_per_bucket);
	}

	/**
	 * Total number of bytes this instance occypies.
	 * Use this function instead of sizeof for this class to take the buffer into account.
	 */
	constexpr size_t size() { return size(data.bucket_count, data.nests_per_bucket); }

	/**
	 * Wraps a data struct into a CuckooFilter object
	 */
	CuckooFilter(cuckoo_filter_data_t& data) : data(data) {}

	/**
	 * Sets the size parameters and then  clear()s this object so that the bucket_array is
	 * filled with precisely enough 0x00s to represent an empty filter.
	 *
	 * @param[in] bucket_array must be a power of 2.
	 */

	void init(cuckoo_index_t bucket_count, cuckoo_index_t nests_per_bucket);
};
