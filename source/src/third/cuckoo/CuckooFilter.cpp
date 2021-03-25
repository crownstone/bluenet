#include <third/cuckoo/CuckooFilter.h>
#include <util/cs_RandomGenerator.h>

#include <util/cs_Crc16.h>

#include <cstring>


/* ------------------------------------------------------------------------- */
/* ---------------------------- Hashing methods ---------------------------- */
/* ------------------------------------------------------------------------- */

CuckooFilter::fingerprint_type CuckooFilter::hash (
  key_type key, size_t key_length_in_bytes
) {
	return static_cast<fingerprint_type>(
			crc16(
					static_cast<const uint8_t*>(key),
					key_length_in_bytes,
					nullptr));
}

/* ------------------------------------------------------------------------- */

CuckooFilter::ExtendedFingerprint CuckooFilter::getExtendedFingerprint(
    fingerprint_type finger, index_type bucket_index) {

    return ExtendedFingerprint{
        .fingerprint = finger,
        .bucketA = static_cast<index_type>(bucket_index),
        .bucketB = static_cast<index_type>((bucket_index ^ finger) % bucket_count)};
}

CuckooFilter::ExtendedFingerprint CuckooFilter::getExtendedFingerprint(
    key_type key, size_t key_length_in_bytes) {

    fingerprint_type finger = hash(key, key_length_in_bytes);
    fingerprint_type hashed_finger = hash(&finger, sizeof(finger));

    return ExtendedFingerprint{
        .fingerprint = finger,
        .bucketA = static_cast<index_type>(hashed_finger % bucket_count),
        .bucketB = static_cast<index_type>((hashed_finger ^ finger) % bucket_count)};
}

/* ------------------------------------------------------------------------- */

CuckooFilter::fingerprint_type CuckooFilter::filterhash() {
	return hash(bucket_array,
			bucket_count *
			nests_per_bucket *
			sizeof(fingerprint_type)/sizeof(uint8_t) );

	// TODO: change to
	// return hash(this, this->size());
}

/* ------------------------------------------------------------------------- */
/* ---------------------------- Filter methods ----------------------------- */
/* ------------------------------------------------------------------------- */

bool CuckooFilter::add_fingerprint_to_bucket (
	fingerprint_type        fingerprint,
	index_type              bucket_number
) {
	for (size_t ii = 0; ii < nests_per_bucket; ++ii) {
		fingerprint_type& fingerprint_in_array = lookupFingerprint(bucket_number, ii);
		if (0 == fingerprint_in_array) {
			fingerprint_in_array = fingerprint;
			return true;
		}
	}

	return false;
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::remove_fingerprint_from_bucket(
	fingerprint_type        fingerprint,
	index_type              bucket_number){
	for (index_type ii = 0; ii < nests_per_bucket; ++ii) {
		fingerprint_type& candidate_fingerprint_for_removal_in_array =
				lookupFingerprint(bucket_number, ii);

		if (candidate_fingerprint_for_removal_in_array == fingerprint) {
			candidate_fingerprint_for_removal_in_array = 0;

			// to keep the bucket front loaded, move the last non-zero
			// fingerprint behind ii into the slot.
			for(index_type jj = nests_per_bucket - 1; jj > ii; --jj) {
				fingerprint_type& last_fingerprint_of_bucket =
						lookupFingerprint(bucket_number, jj);

				if (last_fingerprint_of_bucket != 0) {
					candidate_fingerprint_for_removal_in_array =
							last_fingerprint_of_bucket;
				}
			}

			return true;
		}
	}

	return false;
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::move(ExtendedFingerprint entry_to_insert) {
	// seeding with a hash for this filter guarantees exact same sequence of
	// random integers used for moving fingerprints in the filter on every crownstone.
	uint16_t seed = filterhash();
	RandomGenerator rand(seed);
	
	for (size_t attempts_left = max_kick_attempts; attempts_left > 0; --attempts_left) {
		// try to add to bucket A
		if (add_fingerprint_to_bucket(
				entry_to_insert.fingerprint,
				entry_to_insert.bucketA)) {
			return true;
		}
		
		// try to add to bucket B
		if (add_fingerprint_to_bucket(
				entry_to_insert.fingerprint,
				entry_to_insert.bucketB)) {
			return true;
		}
		
		// no success, time to kick a fingerprint from one of our buckets
		
		// determine which bucket to kick from
		index_type kicked_item_bucket = (rand() % 2)
				? entry_to_insert.bucketA
				: entry_to_insert.bucketB;

		// and which fingerprint index
		index_type kicked_item_index = rand() % nests_per_bucket;
		
		// swap entry to insert and the randomly chosen (kicked) item
		fingerprint_type& kicked_item_fingerprint_ref = lookupFingerprint(kicked_item_bucket,	kicked_item_index);
		fingerprint_type kicked_item_fingerprint_value = kicked_item_fingerprint_ref;
		kicked_item_fingerprint_ref = entry_to_insert.fingerprint;
		entry_to_insert = getExtendedFingerprint(kicked_item_fingerprint_value, kicked_item_bucket);

		// next iteration will try to re-insert the footprint previously at (h,i).	
	}
  
	// failed to re-place the last entry into the buffer after max attempts.
	victim = entry_to_insert;

	return  false;
}

/* ------------------------------------------------------------------------- */

void CuckooFilter::init(index_type bucket_count, index_type nests_per_bucket) {
	// ASSERT(bucket_count & (bucket_count -1) == 0) // bucket count must be power of 2.

	this->bucket_count = bucket_count;
	this->nests_per_bucket = nests_per_bucket;
	clear();
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::contains(ExtendedFingerprint efp) {
  // loops are split to improve cache hit rate.

  // search bucketA
  for (size_t ii = 0; ii < nests_per_bucket; ++ii) {
    if (efp.fingerprint == lookupFingerprint(efp.bucketA, ii)) {
      return true;
    }
  }
  
  // search bucketB
  for (size_t ii = 0; ii < nests_per_bucket; ++ii) {
    if (efp.fingerprint == lookupFingerprint(efp.bucketB, ii)) {
      return true;
    }
  }
            
  return false;
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::add(ExtendedFingerprint efp) {
  if (contains(efp)) {
    return true;
  }

  if (victim.fingerprint != 0) {
    return false;
  }

  return move(efp);
}

/* ------------------------------------------------------------------------- */

void CuckooFilter::clear() {
	std::memset(bucket_array, 0x00, bufferSize());
	victim = ExtendedFingerprint{};

}

bool CuckooFilter::remove(ExtendedFingerprint efp) {
  if (remove_fingerprint_from_bucket(efp.fingerprint, efp.bucketA) ||
	  remove_fingerprint_from_bucket(efp.fingerprint, efp.bucketB)) {
	  // short ciruits nicely:
	  //    tries bucketA,
	  //    on fail try B,
	  //    if either succes, fix victim.

	  if(victim.fingerprint != 0) {
		  if(add(victim)) {
			  victim = ExtendedFingerprint{};
		  }
	  }

    return true;
  }
  
  return false;
}


