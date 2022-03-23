# Asset filtering

When an asset broadcasts an advertisement, this component will decide whether it is of interest (whether it passes the filters), and what to do with it (the output).
Each asset advertisement that passes the filters, will be assigned an [asset ID](../protocol/ASSET_FILTERING.md#asset-id), this is used for output throttling, and can be used as output format.

This component is configured by setting filters, each with their own [filter ID](../protocol/ASSET_FILTERING.md#filter-id). Once new filters have been successfully set at a Crownstone, it will set these filters at neighbouring Crownstones in the mesh network. The progress of this process can be monitored by looking at the [service data](../protocol/SERVICE_DATA.md#alternative-state-packet) of the Crownstones.

The typical workflow for setting the filters is:
1. [Get summaries](../protocol/ASSET_FILTERING.md#get-filter-summaries) to find out what [protocol](../protocol/ASSET_FILTERING.md#asset-filter-store-protocol-version) the firmware uses and which filters are currently in store.
2. [Upload](../protocol/ASSET_FILTERING.md#upload-filter) new filters and [remove](../protocol/ASSET_FILTERING.md#remove-filter) outdated ones.
3. [Commit](../protocol/ASSET_FILTERING.md#commit-filter-changes) to complete the changes.

