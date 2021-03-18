#include <iostream>
#include <limits>

#include "cuckoo_filter.h"
#include "cuckoo_status.h"

/**
 * Checks if 
 *   _add(0), _add(1), ... , _add(max_items),
 *   _contains(0), _contains(1), ..., _contains(max_items),
 * does what it's supposed to do.
 * A load factor is incorporated since cuckoo filters will not fit their theoretical
 * max load.
 */
int main (int argc, char ** argv) {
	// Settings for this test
	const CuckooFilter::index_type max_buckets = 128;
	const CuckooFilter::index_type nests_per_bucket = 4;
	const float load_factor = 0.75f;
	
	// allocate buffer and cast to CuckooFilter
	uint8_t filterBuffer[CuckooFilter::size(max_buckets, nests_per_bucket)];	
	CuckooFilter* filter = reinterpret_cast<CuckooFilter*>(filterBuffer);
	filter->init(max_buckets, nests_per_bucket);

	// setup test variables
	const uint32_t max_items = max_buckets * nests_per_bucket;
	const uint32_t num_items_to_test = max_items * load_factor;
	uint32_t fails = 0;

	// Add a lot of integers
	for (uint32_t i = 0; i < num_items_to_test; i++) {
		uint8_t* i_pun_ptr = reinterpret_cast<uint8_t*>(&i);
		if (filter->add(i_pun_ptr, sizeof(i)) == false) {
			fails++;
		}
	}
	std::cout << "ADD" << Status(fails) << std::endl;
	fails = 0;

	// check if they ended up in the filter
	for (uint32_t i = 0; i < num_items_to_test; i++) {
		uint8_t* i_pun_ptr = reinterpret_cast<uint8_t*>(&i);
		if (filter->contains(i_pun_ptr, sizeof(i)) == false) {
			fails++;
		}
	}
	std::cout << "CONTAINS" << Status(fails) << std::endl;
	fails = 0;

	return 0;

} /* main() */

