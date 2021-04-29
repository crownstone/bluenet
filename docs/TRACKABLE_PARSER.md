# Trackable Parser

This page describes the commands and packets that affect Bluenets Trackable Parser component public API.

Status: *Under active development. Protocol NOT FIXED YET*.


<<Todo: add comments in typical workflow: get summaries, uploads/removes, commit>>
<<Todo: add comment about start/end progress>>
<<Todo: move _internal stuff into this file where necessary>>
<<Todo: define filter set to streamline text about synchronisation etc.>>

## Table of contents

[Commands](#commands)
- [Upload filter](#upload-filter)
- [Remove filter](#remove-filter)
- [Commit filter changes](#commit-filter-changes)
- [Get filter summaries](#get-filter-summaries)

[Packets](#packets)
- [Tracking filter data](#tracking-filter-data)
- [Tracking filter meta data](#tracking-filter-meta-data)
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
uint8_t[] | chunk | ... | <<Todo: explain >> 


#### Upload filter result packet

<<Todo: fix explanation about result packets>>

A [result code](./PROTOCOL.md#result-codes) packet is returned on this command. If result is not SUCCESS, 
no change has been made to the filter or the mesh.
- `SUCCESS`: Chunk has been copied into the desired filter.
- `INVALID_MESSAGE`: Chunk would overflow total size of the filter. Message dropped.
- `WRONG_STATE`: Total size changed between upload commands. Filter deallocated. 
- `NO_SPACE`: Message dropped.


*************************************************************************

### Remove filter

Removes the filter with given filter ID, and starts progress.

#### Remove filter packet

Type | Name | Length | Description
--- | --- | --- | ---
uint8_t | filterId | 1 | Id of the filter to remove.

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

Type | Name | Length | Description
--- | --- | --- | ---
uint16_t | [MasterVersion](#master-version) | 2 | Synchronization version of the TrackableParser at time of constructing this result packet. 
uint16_t | [MasterCrc](#master-crc) | 2 | Master crc at time of constructing this result packet.


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
uint16_t | [MasterVersion](#master-version) | 2 | Synchronization version of the TrackableParser at time of constructing this result packet. 
uint16_t | [MasterCrc](#master-crc) | 2 | Master crc at time of constructing this result packet.
[Filter summary](#filter-summary) | summaries | < MAX_FILTERS_IDS * 6 | Summaries of all filters currently on the Crownstone. <<Todo: MAX_FILTER_IDS should be explained somewhere>>


A [result code](./PROTOCOL.md#result-codes) packet is returned on this command.
- `SUCCESS`: No filter modifications in progress.
- `BUSY`: There was a filter modification in progress while handling this command.


*************************************************************************

## Packets

### Tracking filter data

Type | Name | Length | Description
--- | --- | --- | ---
[Tracking filter meta data](#tracking-filter-meta-data) | metadata | | Metadata determining how the filter behaves
uint8[] | filterdata | | Byte representation of the filter, format depends on the `metadata`


### Tracking filter meta data

Type | Name | Length | Description
--- | --- | --- | ---
[Filter type](#filter-type) | filterType | 1 | Filter protocol of this filter. Describes a `filterdata` format.
uint8_t | profileId | 1 | Entries that pass this filter will be associated with this profile id.
[Filter input type](#filter-input-type) | inputType |  | Determines how this filter interprets incoming entries.
[Filter output type](#filter-output-type) | outputType |  | Determines how advertisements that pass this filter are handled by the system.

### Filter type

Value | Name | Description 
--- | --- | ---
0 | CuckooFilter | Filter data is interpreted as [Cuckoo filter data](./CUCKOO_FILTER.md#cuckoo-filter-data)


### Filter input type
The input type metadata field of a filter defines what data is put into the filter when an advertisement is received
in order to determine wether or not the advertisement 'passed' the filter or is 'rejected'.

Type | Name | Description
--- | --- | ---
advertisement_subdata_t | format | What part of the advertisment is used to filter on.

### Filter output type
When a filter accepts an advertisement, 'output data' needs to be constructed. Depending on the use case this construction 
may or may not need to use parts of the advertisement data. The output_t metadata field of a filter defines which part
of the advertisement is used to produce the output data, and how this output data is formatted.

Type | Name | Description 
--- | --- | ---
[output_format_t](#filter-output-format) | out_format | Determines how the output is formatted. 
uint8[] | in_format | Aux data describing which part of the advertisment is used to construct the output. Type/length of this field depends on `out_format`.



### Filter output format
This output format is a `uint8` used to:
- determines the format of the unique identifyer of the Asset in the Crownstone mesh.
- direct advertisements to the relevant firmware components (E.g. NearestCrownstoneAlgorithm,
MeshTopology, ...) and
- determines `in_format` that describes which data was used to construct the output with.



Value | Name | `in_format` type | `in_format` size |  Output description type
--- | --- | --- | --- | ---
0 | Mac | None | 0 | Mac address as `uint8_t[6]`
1 | ShortAssetId | [Advertisement subdata](#advertisement-subdata) |  | *TODO: formal description* Basically fingerprint+bucket index, but should be allowed to depend on filter implementation type)



### Advertisement subdata
This defines a dataformat/selection of data an advertisement. Given an advertisement_subdata_t value
there is a function that transforms a pair `(macaddres,AD data) -> uint8_t[]` into a byte array.
Effectively that function 'selects' part of that pair which can be used for further processing.

Type | Name | Description
--- | --- | ---
[Advertisement subdata type](#advertisement-subdata-type) | type | See table below
uint8_t[] | auxData | depending on `type`, more descriptive information about the selection method.

### Advertisement subdata type
The advertisement subdata type field can have the following values. 

The AD data is formatted as `(type0, data0[0], ... , data0[l0]), ..., (typeN, dataN[0], ... , dataN[lN])`. Each pair `(typeK, dataK[])` is called an entry. An advertisement consists of a mac address followed by a sequence of AD data entries.

Value | Name  | auxData type | AuxTypeSize | Description
--- | --- | --- | --- | ---
0 | Mac              | - | 0 | The (full) mac address is selected, and nothing more.
1 | AdDataType       | ad_data_type_selector_t  | 1 | The advertisement is truncated to the data part `dataI` of an `(typeI, dataI)` where `typeI` is equal to the given `AdDataType`. If multiple such entries exist, they are treated separately. If that no entry is found with the given type, the advertisement is ignored.
2 | MaskedAdDataType | [masked_ad_data_t](#masked-ad-data-type-selector) | 5 | Same as `AdDataType`, but the data `dataI` is now masked by a byte wise mask.

### Masked AD data type selector
Defines which entries of an advertisement to select based on their type field and defines a mask to blank out irrelevant parts.

The i-th bit of the mask, `AdDataMask & (1 << i)`, is multiplied with `dataI[i]` to obtain the output array.

Type | Name | Description
--- | --- | ---
uint8_t | AdDataType | select an entry with `typeI` equal to `AdDataType`. If multiple such entries exist, they are treated separately.
uint32_t | AdDataMask | bit-to-byte-wise mask for `dataI`.




### Filter summary
A short summary of a filters state.

Type | Name | Length
--- | --- | --- 
uint8_t | filterId | 1 
uint16_t | filterType | 2
uint16_t | filterCrc | 2

### Master Crc

A uint16 that is computed as follows: <<Todo>>
<<Todo: explain master version 0>>


### Master version
A `uint16_t` "lollipop" value used for determining if a filter set is up-to-date. 

<<Todo: This still needs to be implemented>>

Value | Description
--- | --- 
0 | Unknown or unset version. Always inferior to non-zero versions.
other | For non zero versions `v` and all `1 <= n < 2^16`, `v` is inferior to `v+n` including lollipop roll over.

