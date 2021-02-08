# Whitelist



## Table of contents

- [Command packets](#command-packets)
- [Common fields](#common-fields)
- [Filter specific data](#filter-specific-data)

## Commands overview

- [Add](#add-entry)
- [Remove](#remove-entry)
- [Initialize](#initialize-filter)
- [Clear](#clear-filter)
- [Add compressed](#add-compressed-entry)
- [Remove compressed](#remove-compressed-entry)

*************************************************************************

## Command packets

### Initialize filter

Allocates a the given filter with the given parameters.

#### Initialize packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.
[Filter Type](#filter-type) | Type | 1 | Version of filter after the update.
[Filter parameters](#filter-specific-data) | Params | ... | Filter type specific parameters.

#### Initialize result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.

*************************************************************************

### Clear filter

Deallocates the given filter.

#### Initialize packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.

#### Initialize result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.

*************************************************************************

### Add entry

The crownstone receiving this command will try to add the entry in the 
given filter. When this is successful, it will handle resynchronisation
of the filter in the mesh.

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

### Remove entry

The crownstone receiving this command will try to remove the Data entry in the 
given filter. When this is successful, it will handle resynchronisation
of the filter in the mesh.

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

## Compressed command packets

### Add compressed entry 

Entries can be pre-comressed on the host side of the command. This requires
the host to be up to date concerning the implementation of the filter method
and results in less communication.

When filter version is set to 0, the crownstone always accepts the command and will increment its local version for the given filter id. Else, a crownstone only accepts the command if the received filter version is superior to its current version for the given id.


This command may only available for select filter types.

#### Add compressed entry packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.
[Filter version](#filter-version) | Version | 1 | Version of filter after the update.
[Compressed data](#compressed-filter-entry-data) | Entry | <= 5 | Filter implementation specific data describing the entry to be removed.


#### Add entry result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.

*************************************************************************

### Remove compressed entry

Entries can be pre-comressed on the host side of the command. This requires
the host to be up to date concerning the implementation of the filter method
and results in less communication.

When filter version is set to 0, the crownstone always accepts the command and will increment its local version for the given filter id. Else, a crownstone only accepts the command if the received filter version is superior to its current version for the given id.

This command may only available for select filter types.

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

## Common fields

### Filter ID
A `uint8_t` value to determine which filter in the firmware an operation should be applied on.
Filter IDs that are not in this list will result in `FILTER_ID_NOT_FOUND` return values.

Value | Explanation
--- | ---
0 | MAC filter
1 | AD identifyable data filter
2 | AD category data filter
... | Other filters may follow

### Filter version
A `uint8_t` "lollipop" value used for determining if a filter is up-to-date. 


Value | Description
--- | --- 
0 | Unknown, unset. Always inferior to non-zero versions.
other | For non zero versions `v` and all `1 <= n <127`, `v` is inferior to `v+n` including lollipop roll over.

### Filter type
Value | Description
--- | --- 
0 | Cuckoo filter
1 | Raw list filter.

### Filter entry data
Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | Len | 1 | Length of the data array.
uint8_t | Data | Len | Actual data of the entry 



### Result code
A uin8_t constant explaining the operations result.

Value | Explanation
--- | ---
0 | SUCCESS
1 | NO_SPACE
2 | FILTER_ID_NOT_FOUND
3 | VERSION_MISMATCH
4 | COMPRESSION_UNAVAILABLE

*************************************************************************

## Filter specific data

### Cuckoo filter parameters

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | Max key count | 1 | Power of 2. Maximum number of different objects that can be stored. (Slightly lower cap in practice ~90%)
uint8_t | Keys per bucket | 1 | Power of 2, usually 1, 2, 4 or 8. Tradeoff between insertion, containment check speed and storage.
uint8_t | Max kick | 1 | Max insertion attempts. Influences practical max  capacity.
uint32_t | Seed | 4 | Seed for the hash to use. Must be equal for all crownstones in mesh to facilitate compressed commands.

### Raw list filter parameters

Raw list filter can be implemented later, simply copy the entries into a list
and filter lets anyone on the list pass.


*************************************************************************

## Compressed filter entry data

For mesh messages, different filter types may have different filter data
compression. The compressed data consists of at most 5 bytes.

### Compressed cuckoo filter entry data
Type | Name | Length | Description
--- | --- | --- | ---
uint16_t | Fingerprint | 2 | Fingerprint of the data to add
uint8_t | Bucket index | 1 | Index of the bucket to add the data to (i.e. h1, h2)

