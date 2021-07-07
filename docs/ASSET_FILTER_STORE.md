# Asset Filter Store

This page describes the commands and packets that affect Bluenets Asset Filter Store component public API.
Typical workflow for updating the filters of the parser:
1. send a command [get summaries](#get-filter-summaries) to find out what [protocol](#asset-filter-store-protocol-version) the firmware uses and which filters are currently used
2. send commands to [upload](#upload-filter) new filters and [remove](#remove-filter) outdated ones.
3. send a [commit command](#commit-filter-changes) to complete the changes.

When a crownstone reboots it loads any filters stored in its flash module into RAM. After a consistency check it will start parsing bluetooth advertisements.
If an upload or remove command is received, it will change its run-time filters accordingly and set a flag `filterModificationInProgress` to true.
During modifications the parser blocks all advertisements to prevent inconsistent behaviour. The progress flag stays active until a succesfully executed 
commit command or a timeout occurs.

Filters are identified by a `uint8` called a filterId. These ids are free to be chosen by the device that uploads a filter. There is a maximum number of filters on the
Crownstone, which is implementation defined. (`MAX_FILTER_IDS`)

## Table of contents

Version
- [Asset Filter Store protocol version](#asset-filter-store-protocol-version)

Commands packets
- [Upload filter](#upload-filter)
- [Remove filter](#remove-filter)
- [Commit filter changes](#commit-filter-changes)
- [Get filter summaries](#get-filter-summaries)

Filter format packets
- [Tracking filter data](#tracking-filter-data)
- [Tracking filter meta data](#tracking-filter-meta-data)
- [Filter type](#filter-type)

Filter IO packets
- [Filter input type](#filter-input-description)
- [Filter output type](#filter-output-description)
- [Filter output format](#filter-output-format)
- [Advertisement subdata](#advertisement-subdata)
- [Advertisement subdata type](#advertisement-subdata-type)
- [Masked AD data type selector](#masked-ad-data-type-selector)

Filter description packets
- [Filter summary](#filter-summary)
- [Filter master CRC](#filter-master-crc)
- [Filter version](#filter-master-version)

*************************************************************************

## Version

### Asset Filter Store protocol version
All commands that the Asset Filter Store accepts contain a `uint8` command protocol version.
This version will be incremented when making breaking changes to the protocol.

Name | type | Description
--- | --- | ---
commandProtocolVersion | uint8 | Version identifier that describes the further format of the command.

Value | Change Description
--- | ---
0 | **Current version**  Initial protocol definitions. 

## Commands

### Upload filter

Command to upload a filter in chunks. All chunks will be merged by the Crownstone. If a previously committed filter with 
the given filterId is already present on the Crownstone, it will be removed prior to handling the chunk.

This command sets the `filterModificationInProgress` flag to true.

#### Upload filter packet

Type | Name | Length | Description
--- | --- | --- | ---
[CommandProtocolVersion](#asset-filter-store-protocol-version) | protocol | 1 | 
uint8_t | filterId | 1 | Which filter to add the entry to.
uint16_t | chunkStartIndex | 2 |  Offset in bytes of this chunk.
uint16_t | totalSize | 2 | Total size of the chunked data. Practical upper limit depends on command buffer size, prior allocated memory for filters in firmware etc. 
uint16_t | chunkSize | 2 |
uint8_t[] | chunk | `chunkSize` | Contiguous subspan of a [tracking filter data](#tracking-filter-data) packet starting from the byte at `chunkStartIndex` and `chunkSize` bytes in total.


#### Upload filter result

- `SUCCESS`: Chunk has been copied into the desired filter.
- `INVALID_MESSAGE`: Chunk would overflow total size of the filter. Message dropped.
- `WRONG_STATE`: Total size changed between upload commands. Filter deallocated. 
- `NO_SPACE`: Message dropped.


*************************************************************************

### Remove filter

Removes the filter with given filter ID.

This command sets the `filterModificationInProgress` flag to true.

#### Remove filter packet

Type | Name | Length | Description
--- | --- | --- | ---
[CommandProtocolVersion](#asset-filter-store-protocol-version) | protocol | 1 | 
uint8_t | filterId | 1 | Id of the filter to remove.

#### Remove filter result

- `SUCCESS`: filter was found and deallocated. Progress was started.
- `SUCCESS_NO_CHANGE`: filter wasn't found. Progress was started.

*************************************************************************

### Commit filter changes

A commit filter changes command signifies the end of a sequence of changes. When this is received the firmware
will perform several consistency checks:
- CRC values are recomputed where necessary
- Filters are checked for size consistency (e.g. allocated space for a tracking filter must match the cuckoo filter size definition)

Any malformed filters may immediately be deallocated to save resources and prevent firmware crashes. When return value is not `SUCCESS`, query the status with a [get filter summaries](#get-filter-summaries) command for more information.

#### Commit filter packet

Type | Name | Length | Description
--- | --- | --- | ---
[CommandProtocolVersion](#asset-filter-store-protocol-version) | protocol | 1 |
uint16_t | [MasterVersion](#master-version) | 2 | Value of the synchronization version of the Asset Filter Store that should be set to if this command is succesfully handled.
uint32_t | [MasterCrc](#master-crc) | 4 | Master CRC at time of constructing this result.


#### Commit filter result

- `SUCCESS`: all filters passed the consistency checks and the master CRC matches. `MasterVersion` is updated to the value in the received command.
- `MISMATCH`:the master CRC did not match but no consistency checks failed. `MasterVersion` is not changed.
- `WRONG_STATE`: some filters have been deleted due to failed consistency checks. `MasterVersion` is not changed.


*************************************************************************

### Get filter summaries

Obtain a summary of the state of all filters currently on the device and selected metadata about synchronization, memory usage and protocol.

#### Get filter summaries packet

Empty.

#### Get filter summaries result packet

Type | Name | Length | Description
--- | --- | --- | ---
[CommandProtocolVersion](#asset-filter-store-protocol-version) | protocol | 1 | Asset Filter Store protocol version implemented in the firmware
uint16_t | [MasterVersion](#master-version) | 2 | Synchronization version of the Asset Filter Store at time of constructing this result packet.
uint32_t | [MasterCrc](#master-crc) | 4 | Master CRC at time of constructing this result packet.
uint16_t | freeSpace | 2 | The number of bytes that the Asset Filter Store is still allowed to allocate. (Does not take into account free heap space on the firmware.)
[Filter summary](#filter-summary) | summaries | < MAX_FILTERS_IDS * 4 | Summaries of all filters currently on the Crownstone. 

Result:
- `SUCCESS`: No filter modifications in progress.
- `BUSY`: There was a filter modification in progress while handling this command.


*************************************************************************

# Packets

## Filter upload packets

### Tracking filter data

Type | Name | Length | Description
--- | --- | --- | ---
[Tracking filter meta data](#tracking-filter-meta-data) | metadata | | Metadata determining how the filter behaves
uint8[] | filterdata | | Byte representation of the filter, format depends on the `metadata`


### Tracking filter meta data

Type | Name | Length | Description
--- | --- | --- | ---
[Filter type](#filter-type) | filterType | 1 | Filter protocol of this filter. Describes a `filterdata` format.
[Filter flags](#filter-flags) | flags | 1 | Flags bitmask.
uint8_t | profileId | 1 | Entries that pass this filter will be associated with this profile id.
[Filter input type](#filter-input-description) | inputDescription |  | Determines how this filter interprets incoming entries.
[Filter output type](#filter-output-description) | outputDescription |  | Determines how advertisements that pass this filter are handled by the system.


### Filter type

A `uint8` defining the format and implementation of the filter datastructure.

Value | Name | Description 
--- | --- | ---
0 | CuckooFilter | Filter data is interpreted as [Cuckoo filter data](./CUCKOO_FILTER.md#cuckoo-filter-data)


### Filter flags

Bit | Name |  Description
--- | --- | ---
0 | Exclude | If set, the assets that pass this filter will be ignored. Note: this does not mean that assets that do not pass this filter are handled.
1-7 | Reserved | Reserved for future usage, must be 0 for now.


## Filter IO packets

### Filter input description
The input type metadata field of a filter defines what data is put into the filter when an advertisement is received
in order to determine wether or not the advertisement 'passed' the filter or is 'rejected'.

Type | Name | Description
--- | --- | ---
[Advertisement subdata description](#advertisement-subdata-description) | format | What part of the advertisment is used to filter on.

### Filter output description
When a filter accepts an advertisement, 'output data' needs to be constructed. Depending on the use case this construction 
may or may not need to use parts of the advertisement data. The output_t metadata field of a filter defines which part
of the advertisement is used to produce the output data, and how this output data is formatted.

Type | Name | Description 
--- | --- | ---
[Filter output format](#filter-output-format) | out_format | Determines how the output is formatted. 
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
1 | ShortAssetId | [Advertisement subdata](#advertisement-subdata) |  |  [Short asset id](#short-asset-id)



### Advertisement subdata description
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


## Filter description packets

### Filter summary
A short summary of a filters state.

Type | Name | Length
--- | --- | ---
uint8_t | filterId | 1
uint32_t | filterCrc | 4

### Filter master CRC

Given a list of filters `f[0], ... , f[k]`, in [tracking filter data](#tracking-filter-data) format, the `masterCrc` is computed as follows:
- Sort the filters according to ascending `filterId` to obtain a new list: `f'[0], ... , f'[k]`
- compute the CRC-32 values of this list: `c[0], ... , c[k]`.
- create a new list by zipping the filterIds with these crcs: `id(f'[0]), c[0], ... , id(f'[k]), c[k]`.
  Here `id(f'[i])` is the `filterId` of `f'[i]`. Note that these id's are `uint8`, while the CRCs are `uint32`.
- the `masterCrc` the CRC-32 of this zipped list.

The if the firmware needs to recompute its master CRC, it uses the filters it has in RAM,
not the filters that are stored on flash.


### Short asset id

A **3-byte** identifier that is used in Crownstone mesh communication to differentiate between advertisements (or the entities broadcasting them). 
For example, a filter may be configured with [input type](#input-type) `MAC`, and filter type [CuckooFilter](#filter-type). The short asset id
can then be a [compressed filter entry](CUCKOO_FILTER.md#compressed-cuckoo-filter-entry-data).

Each [filter type](#filter-type) is allowed to implement their own algorithm to construct a short asset id based on their input data. They are 
required to make a _reasonable effort_ to avoid collisions. Currently implemented:

[Filter type](#filter-type) | Short asset id type | Depends on input type
--- | --- | ---
CuckooFilter | [compressed filter entry](CUCKOO_FILTER.md#compressed-cuckoo-filter-entry-data) | false


### Filter master version
A `uint16_t` "lollipop" value used for determining if a filter set is up-to-date.

Value | Description
--- | --- 
0 | Unknown or unset version. Always inferior to non-zero versions.
other | For non zero versions `v` and all `1 <= n < 2^(16-1)`, `v` is inferior to `v+n`, including lollipop roll over.

