# Asset Filtering

This page describes the Asset Filtering component of Bluenet which is our defence wall against mesh overload 
in densely inhabited bluetooth environments.

When an asset broadcasts an advertisement, it is this class that performs the necessary computation to decide
whether or not the asset is relevant to the Sphere.


### Filters
There are several types of filters. Each of them is set for inclusion or exclusion filtering. Asset advertisements
pass the Asset Filtering if they are not found in any exclusion filters and in at least one inclusion filter.
In that case an `EVT_ASSET_ACCEPTED` event is emitted on the event bus which any interested component can inspect for 
further actions.

See /docs/ASSET_FILTER_STORE.md#tracking-filter-meta-data for more configuration options.

### Filter commands
...

### Short Asset Ids
Accepted assets may be transformed into (short asset ids)[/ASSET_FILTER_STORE.md#short-asset-id]. These Id's are designed to as unique as possible given the fact they are only 3 bytes long. As advertisements and mac addresses may change, it is not possible to define a deterministic algortihm to compute such SID without knowing the current filter configuration. A consequence of this is that SIDs are invalidated after updating filters. (In some cases this can be avoided.)

An SID will be generated when at least one of the accepting filters `outputDescription.out_format` is `ShortAssetId`. If there is more than one such filter, the one with the lowest `filterId` is used. The SID is determined by passing the advertisement data `outputDescription.in_format` to the particular filter's SID generator function.

Each `filterType` has their own implementation. This is because the ExactMatchFilter is able to guarantee being collision free while CuckooFilters work optimally when using hashes.


#### Default SIDs
Hash (crc32) of the data selected by the accepting filters' `outputDescription.in_format`, trunkated to the lowest 3 bytes.


#### CuckooFilter SIDs
Generates default SIDs.

Note: this may be _more_ bytes than are stored in the CuckooFilter for that asset, which means that eventhough two assets collide in the filter, they may be distinguished in other parts of the system.

#### ExactMatchFilter SIDs
If the data selected by the `outputDescription.in_format` matches an entry in the array with index `i`, the SID will be `{(i>>0) & 0xff, (i>>8) & 0xff,(i>>16) & 0xff}`. When the selected data is not found, default SIDs are generated.
