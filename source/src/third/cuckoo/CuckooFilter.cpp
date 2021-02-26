#include <third/cuckoo/CuckooFilter.h>
#include <util/cs_Random.h>

#include <cstring>

/* ------------------------------------------------------------------------- */
/* ---------------------------- Hashing methods ---------------------------- */
/* ------------------------------------------------------------------------- */

static inline CuckooFilter::fingerprint_type
murmurhash (
  const void           *key,
  uint32_t              key_length_in_bytes
) {
  uint32_t              c1 = 0xcc9e2d51;
  uint32_t              c2 = 0x1b873593;
  uint32_t              r1 = 15;
  uint32_t              r2 = 13;
  uint32_t              m = 5;
  uint32_t              n = 0xe6546b64;
  uint32_t              h = 0;
  uint32_t              k = 0;
  uint32_t              seed = 0xCA4E73D0; // NOTE: just a random value, might not be very good.
  uint8_t              *d = (uint8_t *) key;
  const uint32_t       *chunks = NULL;
  const uint8_t        *tail = NULL;
  int                   i = 0;
  int                   l = (key_length_in_bytes / sizeof(uint32_t));

  h = seed;

  chunks = (const uint32_t *) (d + l * sizeof(uint32_t));
  tail = (const uint8_t *) (d + l * sizeof(uint32_t));

  for (i = -l; i != 0; ++i) {
    k = chunks[i];
    k *= c1;
    k = (k << r1) | (k >> (32 - r1));
    k *= c2;
    h ^= k;
    h = (h << r2) | (h >> (32 - r2));
    h = h * m + n;
  }

  k = 0;
  switch (key_length_in_bytes & 3) {
    case 3: k ^= (tail[2] << 16);
    case 2: k ^= (tail[1] << 8);
    case 1:
      k ^= tail[0];
      k *= c1;
      k = (k << r1) | (k >> (32 - r1));
      k *= c2;
      h ^= k;
  }

  h ^= key_length_in_bytes;
  h ^= (h >> 16);
  h *= 0x85ebca6b;
  h ^= (h >> 13);
  h *= 0xc2b2ae35;
  h ^= (h >> 16);

  return static_cast<CuckooFilter::fingerprint_type>(h);
}

/* ------------------------------------------------------------------------- */

CuckooFilter::fingerprint_type CuckooFilter::scramble(fingerprint_type fingerprint) {
	// this implementation hinges on the fact that 2**16+1 is prime,
	// and that n-> n^17 is a bijection mod 2**16 + 1.
	static_assert(sizeof(uint16_t) == sizeof(fingerprint_type));

	uint64_t x = fingerprint; // can be uint32_t if making this 0 adjustment
	                          // after each square-opration below.
	if (x == 0) {
		// lift 0 to 2**16 to get rid of silly problems involving 0.
		x = 0x10000;
	}

	uint32_t y  = (x * x) % 0x10001; // y == x^2  mod (2**16+1)
	y = (y * y) % 0x10001;           // y == x^4  mod (2**16+1)
	y = (y * y) % 0x10001;           // y == x^8  mod (2**16+1)
	y = (y * y) % 0x10001;           // y == x^16 mod (2**16+1)
	y = (y * x) % 0x10001;           // y == x^17 mod (2**16+1)

	return static_cast<fingerprint_type>(y); // intentional truncation of 0x10000 to 0.
}

/* ------------------------------------------------------------------------- */

CuckooFilter::ExtendedFingerprint CuckooFilter::getExtendedFingerprint(
		fingerprint_type fingerprint) {
	fingerprint_type hashed_finger = scramble(fingerprint);
	return ExtendedFingerprint {
		.fingerprint = fingerprint,
		.bucketA = static_cast<index_type>(hashed_finger % bucket_count),
		.bucketB = static_cast<index_type>((hashed_finger ^ fingerprint) % bucket_count)
	};
}

/* ------------------------------------------------------------------------- */

CuckooFilter::ExtendedFingerprint CuckooFilter::getExtendedFingerprint(
		key_type key, size_t key_length_in_bytes) {
	return getExtendedFingerprint(murmurhash(key, key_length_in_bytes));
}

/* ------------------------------------------------------------------------- */

CuckooFilter::fingerprint_type CuckooFilter::filterhash() {
	return murmurhash(bucket_array,
			bucket_count *
			nests_per_bucket *
			sizeof(fingerprint_type)/sizeof(uint8_t) );
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
		entry_to_insert = getExtendedFingerprint(kicked_item_fingerprint_value);

		// next iteration will try to re-insert the footprint previously at (h,i).	
	}
  
	// failed to re-place the last entry into the buffer after max attempts.
	victim = entry_to_insert.fingerprint;

	return  false;
}

/* ------------------------------------------------------------------------- */
bool CuckooFilter::_new (
		index_type bucket_count,
		index_type nests_per_bucket,
		void* buffer,
		size_t bufferSize) {
  if(bucket_array != nullptr) {
	  return false;
  }

  const auto fingerprintCount = bucket_count * nests_per_bucket;
  const auto allocSize = fingerprintCount * sizeof(fingerprint_type);

  if (bufferSize < allocSize) {
	  return false;
  }

  bucket_array = reinterpret_cast<fingerprint_type*>(buffer);
  memset(bucket_array, 0x00, allocSize);

  this->bucket_count = bucket_count;
  this->nests_per_bucket = nests_per_bucket;
  this->victim = 0;

  return true;
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

  if (victim != 0) {
    return false;
  }

  return move(efp);
}

/* ------------------------------------------------------------------------- */

bool CuckooFilter::remove(ExtendedFingerprint efp) {
  if (remove_fingerprint_from_bucket(efp.fingerprint, efp.bucketA) ||
	  remove_fingerprint_from_bucket(efp.fingerprint, efp.bucketB)) {
	  // short ciruits nicely:
	  //    tries bucketA,
	  //    on fail try B,
	  //    if either succes, fix victim.

	  if(victim != 0) {
		  if(add(victim)) {
			  victim = 0;
		  }
	  }

    return true;
  }
  
  return false;
}


