# Asset filtering

When an asset broadcasts an advertisement, this component will decide whether it is of interest (whether it passes the filters), and what to do with it (the output).
Each asset advertisement that passes the filters, will be assigned an [asset ID](#asset-id), this is used for output throttling, and can be used as output format.

This component is configured by setting filters, each with their own [filter ID](#filter-id). Once new filters have been successfully set at a Crownstone, it will set these filters at neighbouring Crownstones in the mesh network. The progress of this process can be monitored by looking at the [service data](SERVICE_DATA.md#alternative-state-packet) of the Crownstones.

The typical workflow for setting the filters is:
1. [Get summaries](#get-filter-summaries) to find out what [protocol](#asset-filter-store-protocol-version) the firmware uses and which filters are currently in store.
2. [Upload](#upload-filter) new filters and [remove](#remove-filter) outdated ones.
3. [Commit](#commit-filter-changes) to complete the changes.

