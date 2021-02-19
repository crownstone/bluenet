#include <iostream>
#include <set>
#include <string>
#include <random>
#include <algorithm>

#include "cuckoo_filter.h"
#include "cuckoo_status.h"


std::string random_string(std::string::size_type length)
{
    static auto& chrs = "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    static std::mt19937 rg{std::random_device{}()};
    static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(length);

    while(length--)
        s += chrs[pick(rg)];

    return s;
}

bool contains(const std::set<std::string>& my_set, const std::string& val) {
	return my_set.find(val) != my_set.end();
}

/**
 * Allocates FILTER_COUNT filters, then adds the same sequence to each,
 * tests if this sequence is passes containment check and frees the filters.
 * 
 * NOTE(Arend): This seems utterly pointless to me.
 */
int main (int argc, char ** argv) {
	CuckooFilter filter;
	uint32_t fails = 0;

	CuckooFilter::index_type max_buckets = std::numeric_limits<CuckooFilter::index_type>::max();
	CuckooFilter::index_type nests_per_bucket = 4;
	float load_factor = 0.75f;
	uint32_t max_items = max_buckets * nests_per_bucket;
	uint32_t num_items_to_test = max_items * load_factor;

	std::cout << "filter contains " << +max_buckets << " buckets with " << +nests_per_bucket << " items each." << std::endl;
	std::cout << "max items: " << +max_items << " num_items_to_test: " << +num_items_to_test << std::endl;

	// allocate the filter
	if (filter._new(max_buckets, nests_per_bucket) == false) {
		fails++;
	}
	std::cout << "NEW" << Status(fails) << std::endl;

	// generate a bunch of random strings
	std::set<std::string> my_mac_whitelist;
	std::set<std::string> random_mac_addresses;
	for (auto i = 0; i < 0.5*num_items_to_test; i++){
		my_mac_whitelist.insert(random_string(6));
		random_mac_addresses.insert(random_string(6));
	}

	// add the whitelisted items to the filter
	for (auto& mac : my_mac_whitelist) {
		if (filter.add(mac.data(), mac.length()) == false) {
			fails++;
		}
	}
	std::cout << "ADD 0" << Status(fails) << std::endl;
	fails = 0;

	// check if all the whitelisted items pass the filter
	for (auto& mac : my_mac_whitelist) {
		if (filter.contains(mac.data(), mac.length()) == false) {
			fails++;
		}
	}
	std::cout << "CONTAINS 0" << Status(fails) << std::endl;
	fails = 0;

	// check if the random ones fail to pass the whitelist
	// (unless they happen to be in there)
	uint32_t false_positives = 0;
	uint32_t false_negatives = 0;
	for (auto& mac : random_mac_addresses) {
		bool should_contain = contains(my_mac_whitelist, mac);
		bool filter_contains = filter.contains(mac.data(), mac.length());

		if (filter_contains != should_contain) {
			fails++;
			if (filter_contains) {
				false_positives++;
			} else {
				false_negatives++;
			}

		}
	}
	std::cout << "CONTAINS false negatives" << Status(false_negatives, random_mac_addresses.size(), 0.00f) << std::endl;
	std::cout << "CONTAINS false positives" << Status(false_positives, random_mac_addresses.size(), 0.05f) << std::endl;
	fails = 0;
	false_negatives = 0;
	false_positives = 0;


	// deallocate the filter
	if (filter._free() == false) {
		fails++;
	}
	std::cout << "FREE" << Status(fails) << std::endl;
	fails = 0;

	return 0;

} /* main() */

