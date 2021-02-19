#include <iostream>

#include "cuckoo_filter.h"
#include "cuckoo_status.h"


/**
 * Basic usage:
 * alocate, test if empty by one value
 * add item, test if filter contains that item
 * remove item, test if filter contains that item
 * dealocate
 */
int main (int argc, char ** argv) {
	CuckooFilter filter;
	uint32_t fails = 0;

	// allocate the filter array
	if (filter._new(50, 4) == false) {
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
}

