# Trackable Parser Private API

This page describes the commands and packets that can manipulate the filters in the Trackable Parser component at a more granular level. These commands that are considered private API. As such, Crownstone might change this protocol to better suit any desired requirement, possibly without backward compatibility.

Status: Commands currently unimplemented, packets used in [Trackable Parser](#TRACKABLE_PARSER.md)

# Index
[Commands](#commands)
- [Add filter entry](#add-filter-entry)
- [Remove filter entry](#remove-filter-entry)
- [Add compressed filter entry](#add-compressed-filter-entry)
- [Remove compressed filter entry](#remove-compressed-filter-entry)

[Packets](#packets) 
- [Cuckoo filter data](#cuckoo-filter-data)
- [Filter entry data](#cuckoo-filter-entry-data)
- [Extended filter entry data](#extended-cuckoo-filter-entry-data)
- [Compressed cuckoo filter entry data](#compressed-cuckoo-filter-entry-data)

*************************************************************************

## Commands

### Add filter entry

Currently unimplemented.

#### Add entry packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.
[Filter Entry](#filter-entry-data) | Entry | ... | 

#### Add entry result packet

Type | Name | Length | Description
--- | --- | --- | ---
[Result](#result-code) |  | 1 | If result is not SUCCESS, no change has been made to the filter or the mesh.

*************************************************************************

### Remove filter entry

Currently unimplemented.

#### Remove entry packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to remove the entry from.
[Filter Entry](#filter-entry-data) | Entry | ... | 

#### Add entry result packet

Type | Name | Length | Description
--- | --- | --- | ---
[Result](#result-code) |  | 1 | If result is not SUCCESS, no change has been made to the filter or the mesh.

*************************************************************************

## Compressed commands

The cuckoo filter only stores fingerprints of entries, so it is possible to 
update remote filters by simply communicating these compressed (constant size)
packets rather than communicating the full entry (variable length) to achieve
the same result. This requires the host to be up to date concerning the 
implementation of the filter method and results in less communication.

### Add compressed filter entry 

Currently unimplemented.

### Add compressed entry packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.
[Filter version](#filter-version) | Version | 1 | Version of filter after the update.
[Compressed data](#compressed-filter-entry-data) | Entry | <= 5 | Filter implementation specific data describing the entry to be removed.


#### Add entry result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.

*************************************************************************

### Remove compressed filter entry

Currently unimplemented.

#### Remove compressed entry packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to remove the data entry from.
[Filter version](#filter-version) | Version | 1 | Version of filter after the update.
[Compressed data](#compressed-filter-entry-data) | Entry | <= 5 | Filter implementation specific data describing the entry to be removed.

#### Remove entry result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.

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


