#include <third/cuckoo/CuckooFilter.h>
#include <third/cuckoo/CuckooRand.h>

#include <iostream>

/**
 * Checks if the sequence 
 *   _new, _contains, _add, _contains, _remove, contains, _free 
 * does what it is expected to do.
 */
int main (int argc, char ** argv) {
	CuckooFilter filter;
	uint32_t fails = 0;

	CuckooFilter::index_type max_buckets = std::numeric_limits<CuckooFilter::index_type>::max();
	CuckooFilter::index_type nests_per_bucket = 4;

	// allocate the filter
	if (filter._new(max_buckets, nests_per_bucket) == false) {
		fails++;
	}
	std::cout << "NEW" << Status(fails) << std::endl;

	// check if it contains "test"
	if (filter.contains("test", 4)) {
		fails++;
	}
	std::cout << "CONTAINS 0" << Status(fails) << std::endl;
	fails = 0;

	// add "test"
	if (filter.add("test", 4) == false) {
		fails++;
	}
	std::cout << "ADD 0" << Status(fails) << std::endl;
	fails = 0;

	// check if it contains "test"
	if (filter.contains("test", 4) == false) {
		fails++;
	}
	std::cout << "CONTAINS 1" << Status(fails) << std::endl;
	fails = 0;


	// remove "test"
	if (filter.remove("test", 4) == false) {
		fails++;
	}
	std::cout << "REMOVE" << Status(fails) << std::endl;
	fails = 0;

	// check if it contains "test"
	if (filter.contains("test", 4)) {
		fails++;
	}
	std::cout << "CONTAINS 2" << Status(fails) << std::endl;
	fails = 0;

	// deallocate the filter
	if (filter._free() == false) {
		fails++;
	}
	std::cout << "FREE" << Status(fails) << std::endl;
	fails = 0;

	return 0;

} /* main() */

