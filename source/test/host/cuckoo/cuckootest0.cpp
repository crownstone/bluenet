#include <iostream>
#include <limits>

#include "cuckoo_filter.h"
#include "cuckoo_status.h"

/**
 * Checks if 
 *   _new, 
 *   _add(0), _add(1), ... , _add(max_items),
 *   _contains(0), _contains(1), ..., _contains(max_items),
 *   _free
 * does what it's supposed to do.
 * A load factor is incorporated since cuckoo filters will not fit their theoretical
 * max load.
 */
int main (int argc, char ** argv) {
	CuckooFilter filter;
	uint32_t fails = 0;
  
	CuckooFilter::index_type max_buckets = std::numeric_limits<CuckooFilter::index_type>::max();
	CuckooFilter::index_type nests_per_bucket = 4;
	float load_factor = 0.75f;
	uint32_t max_items = max_buckets * nests_per_bucket;
	uint32_t num_items_to_test = max_items * load_factor;


	// allocate the filter
	if (filter._new(max_buckets, nests_per_bucket) == false) {
		fails++;
	}
	std::cout << "NEW" << Status(fails) << std::endl;

	// Add a lot of integers
	for (uint32_t i = 0; i < num_items_to_test; i++) {
		if (filter.add(&i, sizeof(i)) == false) {
			fails++;
		}
	}
	std::cout << "ADD" << Status(fails) << std::endl;
	fails = 0;

	// check if they ended up in the filter
	for (uint32_t i = 0; i < num_items_to_test; i++) {
		if (filter.contains(&i, sizeof(i)) == false) {
			fails++;
		}
	}
	std::cout << "CONTAINS" << Status(fails) << std::endl;
	fails = 0;

	// deallocate the filter
	if (filter._free() == false) {
		fails++;
	}
	std::cout << "FREE" << Status(fails) << std::endl;
	fails = 0;

	return 0;

} /* main() */

