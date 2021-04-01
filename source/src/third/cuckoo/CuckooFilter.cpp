#include <third/cuckoo/CuckooFilter.h>
#include <util/cs_RandomGenerator.h>

#include <util/cs_Crc16.h>

#include <cstring>

/* ------------------------------------------------------------------------- */
/* ---------------------------- Hashing methods ---------------------------- */
/* ------------------------------------------------------------------------- */

cuckoo_fingerprint_t CuckooFilter::hash(cuckoo_key_t key, size_t key_length_in_bytes) {
	return static_cast<cuckoo_fingerprint_t>(
			crc16(static_cast<const uint8_t*>(key), key_length_in_bytes, nullptr));
}

/* ------------------------------------------------------------------------- */

cuckoo_extended_fingerprint_t CuckooFilter::getExtendedFingerprint(
		cuckoo_fingerprint_t finger, cuckoo_index_t bucket_index) {

	return cuckoo_extended_fingerprint_t{
			.fingerprint = finger,
			.bucketA     = static_cast<cuckoo_index_t>(bucket_index),
			.bucketB     = static_cast<cuckoo_index_t>((bucket_index ^ finger) % bucketCount())};
}

cuckoo_extended_fingerprint_t CuckooFilter::getExtendedFingerprint(
		cuckoo_key_t key, size_t key_length_in_bytes) {

	cuckoo_fingerprint_t finger        = hash(key, key_length_in_bytes);
	cuckoo_fingerprint_t hashed_finger = hash(&finger, sizeof(finger));

	return cuckoo_extended_fingerprint_t{
			.fingerprint = finger,
			.bucketA     = static_cast<cuckoo_index_t>(hashed_finger % bucketCount()),
			.bucketB     = static_cast<cuckoo_index_t>((hashed_finger ^ finger) % bucketCount())};
}

/* ------------------------------------------------------------------------- */

cuckoo_fingerprint_t CuckooFilter::filterhash() {
	return hash(&data, size());
}

/* ------------------------------------------------------------------------- */
/* ---------------------------- Filter methods ----------------------------- */
/* ------------------------------------------------------------------------- */

bool CuckooFilter::add_fingerprint_to_bucket(
		cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucket_number) {
	for (size_t ii = 0; ii < data.nests_per_bucket; ++ii) {
		cuckoo_fingerprint_t& fingerprint_in_array = lookupFingerprint(bucket_number, ii);
		if (0 == fingerprint_in_array) {
			fingerprint_in_array = fingerprint;
			return true;
		}
	}

	return false;
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::remove_fingerprint_from_bucket(
		cuckoo_fingerprint_t fingerprint, cuckoo_index_t bucket_number) {
	for (cuckoo_index_t ii = 0; ii < data.nests_per_bucket; ++ii) {
		cuckoo_fingerprint_t& candidate_fingerprint_for_removal_in_array =
				lookupFingerprint(bucket_number, ii);

		if (candidate_fingerprint_for_removal_in_array == fingerprint) {
			candidate_fingerprint_for_removal_in_array = 0;

			// to keep the bucket front loaded, move the last non-zero
			// fingerprint behind ii into the slot.
			for (cuckoo_index_t jj = data.nests_per_bucket - 1; jj > ii; --jj) {
				cuckoo_fingerprint_t& last_fingerprint_of_bucket =
						lookupFingerprint(bucket_number, jj);

				if (last_fingerprint_of_bucket != 0) {
					candidate_fingerprint_for_removal_in_array = last_fingerprint_of_bucket;
				}
			}

			return true;
		}
	}

	return false;
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::move(cuckoo_extended_fingerprint_t entry_to_insert) {
	// seeding with a hash for this filter guarantees exact same sequence of
	// random integers used for moving fingerprints in the filter on every crownstone.
	uint16_t seed = filterhash();
	RandomGenerator rand(seed);

	for (size_t attempts_left = max_kick_attempts; attempts_left > 0; --attempts_left) {
		// try to add to bucket A
		if (add_fingerprint_to_bucket(entry_to_insert.fingerprint, entry_to_insert.bucketA)) {
			return true;
		}

		// try to add to bucket B
		if (add_fingerprint_to_bucket(entry_to_insert.fingerprint, entry_to_insert.bucketB)) {
			return true;
		}

		// no success, time to kick a fingerprint from one of our buckets

		// determine which bucket to kick from
		cuckoo_index_t kicked_item_bucket =
				(rand() % 2) ? entry_to_insert.bucketA : entry_to_insert.bucketB;

		// and which fingerprint index
		cuckoo_index_t kicked_item_index = rand() % data.nests_per_bucket;

		// swap entry to insert and the randomly chosen (kicked) item
		cuckoo_fingerprint_t& kicked_item_fingerprint_ref =
				lookupFingerprint(kicked_item_bucket, kicked_item_index);
		cuckoo_fingerprint_t kicked_item_fingerprint_value = kicked_item_fingerprint_ref;
		kicked_item_fingerprint_ref                        = entry_to_insert.fingerprint;
		entry_to_insert = getExtendedFingerprint(kicked_item_fingerprint_value, kicked_item_bucket);

		// next iteration will try to re-insert the footprint previously at (h,i).
	}

	// failed to re-place the last entry into the buffer after max attempts.
	data.victim = entry_to_insert;

	return false;
}

/* ------------------------------------------------------------------------- */

void CuckooFilter::init(cuckoo_index_t bucket_count, cuckoo_index_t nests_per_bucket) {
	// find ceil(log2(bucket_count)):
	data.bucket_count_log2 = 0;
	if (bucket_count > 0) {
		bucket_count--;
		while (bucket_count > 0) {
			bucket_count >>= 1;
			data.bucket_count_log2 += 1;
		}
	}

	data.nests_per_bucket = nests_per_bucket;
	clear();
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::contains(cuckoo_extended_fingerprint_t efp) {
	// loops are split to improve cache hit rate.

	// search bucketA
	for (size_t ii = 0; ii < data.nests_per_bucket; ++ii) {
		if (efp.fingerprint == lookupFingerprint(efp.bucketA, ii)) {
			return true;
		}
	}

	// search bucketB
	for (size_t ii = 0; ii < data.nests_per_bucket; ++ii) {
		if (efp.fingerprint == lookupFingerprint(efp.bucketB, ii)) {
			return true;
		}
	}

	return false;
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::add(cuckoo_extended_fingerprint_t efp) {
	if (contains(efp)) {
		return true;
	}

	if (data.victim.fingerprint != 0) {
		return false;
	}

	return move(efp);
}

/* ------------------------------------------------------------------------- */

void CuckooFilter::clear() {
	std::memset(data.bucket_array, 0x00, bufferSize());
	data.victim = cuckoo_extended_fingerprint_t{};
}

bool CuckooFilter::remove(cuckoo_extended_fingerprint_t efp) {
	if (remove_fingerprint_from_bucket(efp.fingerprint, efp.bucketA)
		|| remove_fingerprint_from_bucket(efp.fingerprint, efp.bucketB)) {
		// short ciruits nicely:
		//    tries bucketA,
		//    on fail try B,
		//    if either succes, fix data.victim.

		if (data.victim.fingerprint != 0) {
			if (add(data.victim)) {
				data.victim = cuckoo_extended_fingerprint_t{};
			}
		}

		return true;
	}

	return false;
}
