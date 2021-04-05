# Whitelist internals

This page describes features of Bluenets whitelist component that are 
considered non-public API. As such, Crownstone might change this protocol
to better suit any desired requirement, possibly without backward compatibility.

# Index
- [Commands](#commands)
  - [Add](#add-entry)
  - [Remove](#remove-entry)
  - [Add compressed](#add-compressed-entry)
  - [Remove compressed](#remove-compressed-entry)
- [Packets](#packets) 
  - entry data
  - compressed entry data

*************************************************************************

## Commands

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

## Compressed commands

The cuckoo filter only stores fingerprints of entries, so it is possible to 
update remote filters by simply communicating these compressed (constant size)
packets rather than communicating the full entry (variable length) to achieve
the same command. This requires the host to be up to date concerning the 
implementation of the filter method and results in less communication.

### Add compressed entry 

When filter version is set to 0, the crownstone always accepts the command and will increment its local version for the given filter id. Else, a crownstone only accepts the command if the received filter version is superior to its current version for the given id.

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

### Remove compressed entry

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

## Packets

### Cuckoo filter entry data
Type | Name | Length | Description
--- | --- | --- | ---
EntryType | Type | 1 | Semantics of this data.
uint8_t | Len | 1 | Length of the data array.
uint8_t | Data | Len | Actual data of the entry 

### Compressed cuckoo filter entry data
Type | Name | Length | Description
--- | --- | --- | ---
uint16_t | Fingerprint | 2 | Fingerprint of the data to add
uint8_t | Bucket index | 1 | Index of the bucket to add the data to (i.e. h1, h2)


