# Whitelist

This page describes the commands and packets that affect Bluenets whitelist
filter component public API.

## Table of contents

- [Command](#commands)
- [Packets](#packets)

## Bulk filter commands

- [Add filter](#initialize-filter)
- [Replace filter](#replace-filter)
- [Remove filter](#clear-filter)

*************************************************************************

## Commands

### Add filter

Allocates a new filter with the given parameters.

#### Add filter packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.
[Filter Type](#filter-type) | Type | 1 | Version of filter after the update.
[Filter parameters](#filter-specific-data) | Params | ... | Filter type specific parameters.

#### Add filter result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.

*************************************************************************

### Replace filter

Deallocates the given filter.

#### Replace filter packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.

#### Replace filter result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.


*************************************************************************

### Remove filter

Deallocates the given filter.

#### Remove filter packet

Type | Name | Length | Description
--- | --- | --- | ---
[Filter ID](#filter-id) | Id | 1 | Which filter to add the entry to.

#### Remove filter result packet

A [Result](#result-code) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.


*************************************************************************

## Packets


### Filter parameters


### Cuckoo filter parameters

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | Max key count | 1 | Power of 2. Maximum number of different objects that can be stored. (Slightly lower cap in practice ~90%)
uint8_t | Keys per bucket | 1 | Power of 2, usually 1, 2, 4 or 8. Tradeoff between insertion, containment check speed and storage.
uint8_t | Max kick | 1 | Max insertion attempts. Influences practical max  capacity.
uint32_t | Seed | 4 | Seed for the hash to use. Must be equal for all crownstones in mesh to facilitate compressed commands.


### Filter input type
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

### Result code
A uin8_t constant explaining the operations result.

Value | Explanation
--- | ---
0 | SUCCESS
1 | NO_SPACE
2 | FILTER_ID_NOT_FOUND
3 | VERSION_MISMATCH
4 | COMPRESSION_UNAVAILABLE




