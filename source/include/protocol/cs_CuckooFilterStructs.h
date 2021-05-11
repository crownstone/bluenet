/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 26, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <stddef.h>
#include <cstdint>

typedef const void* cuckoo_key_t;
typedef uint16_t cuckoo_fingerprint_t;
typedef uint8_t cuckoo_index_t;

/**
 * Representation of an object (O) in this filter comprises of a fingerprint (F)
 * and a the bucket index (A) where this fingerprint is located. Each object
 * has an alternative location (B). Together (C):= (F,A,B) contains all the information
 * that the filter retains about the original object (O). This data chunk (C) can be
 * reused in several functions and thus got its own struct defintion.
 */
struct __attribute__((__packed__)) cuckoo_extended_fingerprint_t {
	cuckoo_fingerprint_t fingerprint;
	cuckoo_index_t bucketA;  // the bucket this fingerprint should be put in
	cuckoo_index_t bucketB;  // and the alternative bucket.
};

struct __attribute__((__packed__)) cuckoo_compressed_fingerprint_t {
	cuckoo_fingerprint_t fingerprint;
	cuckoo_index_t bucket;  // the bucket this fingerprint should be put in
};



/**
 * Data content of the cuckoo filter.
 */
struct __attribute__((__packed__)) cuckoo_filter_data_t {
	cuckoo_index_t bucketCountLog2; // we save the log2 value since bucket_count needs to be a power of 2.
	cuckoo_index_t nestsPerBucket;
	cuckoo_extended_fingerprint_t victim;
	cuckoo_fingerprint_t bucketArray[];  // 'flexible array member'
};
