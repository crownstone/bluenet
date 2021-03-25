#pragma once

#include <stddef.h>
#include <cstdint>

/**
 * TODO(Arend):
 * - add test assert(std::is_standard_layout_v<CuckooFilter>);
 * - add test assert(std::is_trivial_v<CuckooFilter>);
 *
 * CuckooFilter datatype is made s.t. the buffer is at the back of the
 * struct, in one contiguous chunk of memory. Thus avoiding pointer assignment
 * on construction. It is fully memcpy-able.
 *
 * CuckooFilter is not aware of its buffer size. That must be managed by its owner.
 */
class CuckooFilter {
   public:
    typedef const void* key_type;
    typedef uint16_t fingerprint_type;
    typedef uint8_t index_type;

    static constexpr size_t max_kick_attempts = 100;

    /**
     * Data chunk that can be reused in several functions.
     */
    class ExtendedFingerprint {
       public:
        fingerprint_type fingerprint;
        index_type bucketA;  // the bucket this fingerprint should be put in
        index_type bucketB;  // and the alternative bucket.
    };

    // -------------------------------------------------------------
    // Run time variables
    // -------------------------------------------------------------

    index_type bucket_count;
    index_type nests_per_bucket;  // rename to fingerprints_per_bucket.

    // if this is != 0, it was kicked from the filter
    // and can't be reinserted because the filter is full.
    ExtendedFingerprint victim = {};

    fingerprint_type bucket_array[];  // 'flexible array member'

   private:
    /**
     * Note: bucket_count _must_ be a power of two. (which is validated at construction/init)
     */
    ExtendedFingerprint getExtendedFingerprint(key_type key, size_t key_length_in_bytes);

    /**
     * Note: bucket_count _must_ be a power of two. (which is validated at construction/init)
     */
    ExtendedFingerprint getExtendedFingerprint(
        fingerprint_type fingerprint, index_type bucket_index);

    // -------------------------------------------------------------
    // ----- Private methods -----
    // -------------------------------------------------------------

    /**
     * A hash of this filters current contents.
     * TODO(Arend): replace with CRC16
     * TODO(Arend): incorporate victim and other settings.
     */
    fingerprint_type filterhash();

    /**
     * Hashes the given key (data) into a fingerprint.
     */
    fingerprint_type hash(key_type key, size_t key_length_in_bytes);

    /**
     * Returns a reference to the fingerprint at the given coordinates.
     */
    fingerprint_type& lookupFingerprint(index_type bucket_number, index_type finger_index) {
        return bucket_array[(bucket_number * nests_per_bucket) + finger_index];
    }

    /**
     * Returns true if there was an empty space in the bucket and placement
     * was successful, returns false otherwise.
     *
     * Does _not_ check for duplicates.
     */
    bool add_fingerprint_to_bucket(fingerprint_type fingerprint, index_type bucket_number);

    /**
     * Returns true if found and removed, returns false if not found.
     */
    bool remove_fingerprint_from_bucket(fingerprint_type fingerprint, index_type bucket_number);

    // puts the fingerprint in the filter, moving aside anything
    // that is in the way unless it takes more than max_kick_attempts.
    // return _OK or _FULL.
    // This doesn't check if the given fingerprint is already contained in the filter.
    bool move(ExtendedFingerprint entry_to_insert);

    // uses extended fingerprints to facilitate reuse of computed hash values.
    bool add(ExtendedFingerprint efp);
    bool remove(ExtendedFingerprint efp);
    bool contains(ExtendedFingerprint efp);

   public:
    // -------------------------------------------------------------
    // These might be useful for fingerprints coming from the mesh.
    // -------------------------------------------------------------
    bool add(fingerprint_type finger, index_type bucket_index) {
        return add(getExtendedFingerprint(finger, bucket_index));
    }

    bool remove(fingerprint_type finger, index_type bucket_index) {
        return remove(getExtendedFingerprint(finger, bucket_index));
    }

    bool contains(fingerprint_type finger, index_type bucket_index) {
        return contains(getExtendedFingerprint(finger, bucket_index));
    }

    // -------------------------------------------------------------
    // these might be useful for stuff that comes in from commands
    // -------------------------------------------------------------

    bool add(key_type key, size_t key_length_in_bytes) {
        return add(getExtendedFingerprint(key, key_length_in_bytes));
    }

    bool remove(key_type key, size_t key_length_in_bytes) {
        return remove(getExtendedFingerprint(key, key_length_in_bytes));
    }

    bool contains(key_type key, size_t key_length_in_bytes) {
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

    static constexpr size_t fingerprintCount(index_type bucket_count, index_type nests_per_bucket) {
        return bucket_count * nests_per_bucket;
    }

    /**
     * Size of the byte buffer in bytes.
     */
    static constexpr size_t bufferSize(index_type bucket_count, index_type nests_per_bucket) {
        return fingerprintCount(bucket_count, nests_per_bucket) * sizeof(fingerprint_type);
    }

    constexpr size_t bufferSize() { return bufferSize(bucket_count, nests_per_bucket); }

    /**
     * Total number of bytes a CuckooFilter with the given parameters would occupy.
     *
     * As per cppreference.com:
     *   "sizeof, and the assignment operator ignore the flexible array member."
     *   https://en.cppreference.com/w/c/language/struct
     */
    static constexpr size_t size(index_type bucket_count, index_type nests_per_bucket) {
        return sizeof(CuckooFilter) + bufferSize(bucket_count, nests_per_bucket);
    }

    /**
     * Total number of bytes this instance occypies.
     * Use this function instead of sizeof for this class to take the buffer into account.
     */
    constexpr size_t size() { return size(bucket_count, nests_per_bucket); }

    /**
     * Sets the size members, clears victim and buffer. No sizechecks done, but
     *
     *  WARNING: bucket count must be power of 2.
     */
    void init(index_type bucket_count, index_type nests_per_bucket);
};
