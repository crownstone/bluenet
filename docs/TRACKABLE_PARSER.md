# Trackable Parser

This page describes the commands and packets that affect Bluenets Trackable Parser component public API.

Status: *Under active development. Protocol NOT FIXED YET*.

## Table of contents

[Commands](#commands)
- [Upload filter](#upload-filter)
- [Remove filter](#remove-filter)
- [Commit filter changes](#commit-filter-changes)
- [Get filter summaries](#get-filter-summaries)

[Packets](#packets)
- [Tracking filter data](#tracking-filter-data)
- [Tracking filter meta data](#tracking-filter-meta-data)
- [Cuckoo filter data](#cuckoo-filter-data)
- [Filter input type](#filter-input-type)
- [Filter version](#filter-version)
- [Filter flags](#filter-flags)

*************************************************************************

## Commands

### Upload filter

Command to upload a filter. All chunks will be merged by the Crownstone.

The `chunk` must be part of a [tracking filter data](#tracking-filter-data). 

If a previously committed filter with the given filterId is already present on the Crownstone, it will be removed prior to handling the chunk.

#### Upload filter packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | filterId | 1 | Which filter to add the entry to.
uint16_t | chunkStartIndex | 2 |  Offset in bytes of this chunk.
uint16_t[] | totalSize | 2 |
uint16_t | chunkSize | 2 |
uint8_t[] | chunk | ... | 


#### Upload filter result packet

A [result code](./PROTOCOL.md#result-codes) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.
- `SUCCESS`: Chunk has been copied into the desired filter.
- `INVALID_MESSAGE`: Chunk would overflow total size of the filter. Message dropped.
- `WRONG_STATE`: Total size changed between upload commands. Filter deallocated. 
- `NO_SPACE`: Message dropped.


*************************************************************************

### Remove filter

Deallocates the given filter and starts progress.

#### Remove filter packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | filterId | 1 | Which filter to add the entry to.

#### Remove filter result packet

A [result code](./PROTOCOL.md#result-codes) packet is returned on this command.
- `SUCCESS`: filter was found and deallocated. Progress was started.
- `NOT_FOUND`: filter wasn't found. Progress was started.

*************************************************************************

### Commit filter changes

A commit filter changes command signifies the end of a sequence of changes. When this is received the firmware
will perform several consistency checks:
- Crc values are recomputed where necessary
- Filters are checked for size consistency (e.g. allocated space for a tracking filter must match the cuckoo filter size definition)

Any malformed filters may immediately be deallocated to save resources and prevent firmware crashes. When return value is not `SUCCESS`, query the status with a [get filter summaries](#get-filter-summaries) command for more information.

#### Commit filter packet

Empty.

#### Commit filter result packet

A [result code](./PROTOCOL.md#result-codes) packet is returned on this command. 

- `SUCCESS`: all filters passed the consistency checks and the master crc matches.
- `MISMATCH`:the master crc did not match but no consistency checks failed.
- `WRONG_STATE`: some filters have been deleted due to failed consistency checks.


*************************************************************************

### Get filter summaries

Obtain a summary of the state of all filters currently on the device.

#### Get filter summaries packet

Empty.

#### Get filter summaries result packet
Type | Name | Length | Description
--- | --- | --- | ---
uint16_t | masterVersion | 2 | Synchronization version of the TrackableParser at time of constructing this result packet.  
uint16_t | masterCrc | 2 | Master crc at time of constructing this result packet.
[Filter summary](#filter-summary) | summaries | < MAX_FILTERS_IDS * 6 | Summaries of all filters currently on the device.


A [result code](./PROTOCOL.md#result-codes) packet is returned on this command.
- `SUCCESS`: No filter modifications in progress.
- `BUSY`: There was a filter modification in progress while handling this command.


*************************************************************************

## Packets

### Tracking filter data

Type | Name | Length | Description
--- | --- | --- | ---
[Tracking filter meta data](#tracking-filter-meta-data) | metadata | | Metadata determining how the filter behaves
[Cuckoo filter data](#cuckoo-filter-data) | filter | | Byte representation of the filter


### Tracking filter meta data

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | protocol | 1 | Filter protocol of this filter. 
[Filter version](#filter-version) | version | 1 | Synchronisation version of this filter.
uint16_t | profileId | 2 | Entrys that pass this filter will be associated with this profile id.
[Filter input type](#filter-input-type) | inputType | 1 | Determines how this filter interprets incoming entries.
[Filter flags](#filter-flags) | flags | 1 | bitmask for future use. Must remain 0 for now.


### Cuckoo filter data

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | number of buckets log2 | 1 | Cuckoofilter will contain 2^(num buckets) buckets.
uint8_t | Keys per bucket | 1 | Each bucket will contain at most this amount of fingerprints. 
uint16_t | victim fingerprint | 2 | Fingerprint of last item that failed to be inserted, 0 if none.
uint8_t | victim bucket index A | 1 | Bucket A of victim, 0 if none.
uint8_t | victim bucket index B | 1 | Bucket V of victim, 0 if none.
uint8_t[] | fingerprint array | 2^N*K | Fingerprint array. Here N = number of buckets log 2, K is number of fingerprints per bucket


### Filter input type
A `uint8_t` value to determine which part of an advertisment is fed to the filter and how it is interpreted. 

Value | Name | Description
--- | --- | ---
0 | MAC filter | Mac address is fed into the filter and determines the trackable id.
1 | AD data identity filter | AD data is fed into the filter and determines the trackable id.
2 | AD data category filter | AD data is fed into the filter but MAC address determines the trackable id.

### Filter version
A `uint16_t` "lollipop" value used for determining if a filter is up-to-date. 


Value | Description
--- | --- 
0 | Unknown or unset version. Always inferior to non-zero versions.
other | For non zero versions `v` and all `1 <= n < 2^16`, `v` is inferior to `v+n` including lollipop roll over.

### Filter flags
A `uint8_t` bitmask.

Bits | Name | Description
--- | --- | ---
0 | isActive | Determines if the filter is currently active or not.
1-7 | - | Reserved for future use. Must be 0.

### Filter summary
A short summary of a filters state.

Type | Name | Length
--- | --- | --- 
uint8_t | filterId | 1 
uint8_t | flags | 1 
uint16_t | filterVersion | 2
uint16_t | filterCrc | 2
