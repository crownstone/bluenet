# Trackable Parser Private API

This page describes the packets that related Cuckoo filters. Thes filters can be used in the Trackable Parser component at a more granular level.

# Index

[Packets](#packets) 
- [Cuckoo filter data](#cuckoo-filter-data)
- [Filter entry data](#cuckoo-filter-entry-data)
- [Extended filter entry data](#extended-cuckoo-filter-entry-data)
- [Compressed cuckoo filter entry data](#compressed-cuckoo-filter-entry-data)

*************************************************************************

## Compressed commands

The cuckoo filter only stores fingerprints of entries, so it is possible to 
update remote filters by simply communicating these compressed (constant size)
packets rather than communicating the full entry (variable length) to achieve
the same result. This requires the host to be up to date concerning the 
implementation of the filter method and results in less communication.


*************************************************************************

## Packets


### Cuckoo filter data

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | number of buckets log2 | 1 | Cuckoofilter will contain 2^(num buckets) buckets.
uint8_t | Keys per bucket | 1 | Each bucket will contain at most this amount of fingerprints. 
[extended fingerprint](#extended-cuckoo-filter-entry-data) | victim fingerprint | 4 | Fingerprint of last item that failed to be inserted, 0 if none.
uint8_t[] | fingerprint array | 2^N*K | Fingerprint array. Here N = number of buckets log 2, K is number of fingerprints per bucket


### Cuckoo filter entry data
Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | Len | 1 | Length of the data array.
uint8_t | Data | Len | Actual data of the entry 


### Extended cuckoo filter entry data
Extended fingerprints with bucket index A/B swapped identify the same entry in the cuckoo filter.

Type | Name | Length | Description
--- | --- | --- | ---
uint16_t | Fingerprint   | 2 | Fingerprint of the data to add
uint8_t | Bucket index A | 1 | Index of the bucket to that the fingerprint belongs to.
uint8_t | Bucket index B | 1 | Alternative bucket that the fingerprint belongs to.


### Compressed cuckoo filter entry data
Type | Name | Length | Description
--- | --- | --- | ---
uint16_t | Fingerprint | 2 | Fingerprint of the data to add
uint8_t | Bucket index | 1 | Index of the bucket to that the fingerprint belongs to.


