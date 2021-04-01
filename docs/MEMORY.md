# Memory

Information and decisions with respect to memory on the Crownstone (both flash and ram).

## Memory layout

### Flash

This document describes the flash memory layout, and how to add more data to store. Bluenet makes use of the Persistent storage manager from the Nordic SDK.

The global layout of the the flash is shown below:


| Start address | What | Nr of pages
| ------------- |:-------------:| -----:|
| 0x00000000 | MBR | 1
| 0x00001000 | SD | 37
| 0x00026000 | App / Bluenet | 43
| 0x00051000 | Free | 24
| 0x00069000 | Microapp | 4
| 0x0006D000 | IPC page | 1
| 0x0006E000 | Reserved for FDS expansion | 4
| 0x00072000 | FDS | 4
| 0x00076000 | Bootloader | 7
| 0x0007D000 | Reserved for bootloader expansion | 1
| 0x0007E000 | MBR settings | 1
| 0x0007F000 | Bootloader settings | 1
| | **Total** | 128


The bootloader start address is defined in the _CMakeBuild.config_ as `BOOTLOADER_START_ADDRESS`.

The application start address is defined in the _CMakeBuild.config_ as `APPLICATION_START_ADDRESS`.

The firmware size + free size is 352kB. For the dual bank bootloader, this means that the firmware can be 176kB max.

### RAM

The amount of RAM in the nRF52832 is 64kB. See the [config file](https://github.com/crownstone/bluenet/blob/master/source/conf/cmake/CMakeBuild.config.default) and the [bluenet linker file](https://github.com/crownstone/bluenet/blob/master/source/include/third/nrf/generic_gcc_nrf52.ld).

| Start address | What | Size | Size | Size (kB)
| ------------- |:----:| ----:| ----:| --------:|
| 0x20000000 | SD | 0x2A00 | 10752 | 10.5
| 0x20002A00 | App heap and stack | 0xCD00 | 52480 | 51.25
| 0x2000F700 | Microapp | 0x800 | 2048 | 2
| 0x2000FF00 | IPC | 0x100 | 256 | 0.25
| | **Total** | 0x10000 | 65536 | 64

### RAM start and end addresses

| Start address | End address | What
| ------------- | ----------- | ----
| 0x20000000    | 0x20002A00  | Soft device.
| 0x20002A00    |             | App heap start, grows up to stack.
| 0x2000F6FF    |             | App stack start, grows down to heap.
| 0x2000F700    | 0x2000FEFF  | Microapp stack, grows from `end` down to `start`.
| 0x2000FF00    | 0x20010000  | IPC, fixed layout.


* TODO: There is a reference to `CORE_BL_RAM`. I suppose this is not actually used... If RAM has to be preserved for the bootloader, we have to move the IPC section below this part.

In bootloader mode, RAM is (see the [bootloader linker file](https://github.com/crownstone/bluenet/blob/master/source/bootloader/secure_bootloader_gcc_nrf52.ld)):

| Start address | What | Size | Size | Size (kB)
| ------------- |:-------------:| -----:| -----:| -----:|
| 0x20000000 | SD + offset | 0x3118 | 12568 | 12.273
| 0x20002A00 | Bootloader heap/stack | 0xCDE8 | 52712 | 51.477
| 0x0000FE00 | IPC | 0x100 | 256 | 0.250
| | **Total** | 0x10000 | 65536 | 64

## App data

The app data is divided into pages of 0x1000 bytes large. The first page is used by the persistent storage manager, as swap. Bluenet has 3 pages: configuration, general, and state.
Furthermore, one page can be used for the device manager.

![App data layout](../docs/diagrams/flash-memory-layout-app-data.png)

The order is defined _cs_Storage.h_ (`ps_storage_id`) and _cs_Storage.cpp_ (`storage_config_t`).

The app data can be read out with a script:
```
./scripts/printAppData.sh
```

### Adding a page

Since the first entry in the `ps_storage_id` and `storage_config_t` gets the lowest address on flash, and since the app data grows downwards, **a new page should be added as first entry**.
More specifically: the order is determined by the order in which `pstorage_register` is called. The first call gets the lowest address.

The number of pages should be increased in the following places:

- `PSTORAGE_NUM_OF_PAGES` in _pstorage_platform.h_
- `DFU_APP_DATA_CURRENT` in _dfu_types.h_ (in the bluenet bootloader code).

Also add a new entry in _dfu_types.h_ of the bootloader: `DFU_APP_DATA_RESERVED_VX_X_X` and add code to handle this in _dfu_dual_bank.c_.

To make sure that no data is already at the new page, a value  should be added to an existing page. If this value is the default value (0xFFFFFFFF), then the new page should be cleared. Afterwards, the value can be set, to mark that the new page can be used.


### Swap page

The swap page is used for clear and update commands.
Since for a write, a pstorage_update is used, the following happens for each write:
- swap page is cleared
- data page is copied to swap
- data page is cleared
- swap is copied to data page, except with the new value

This makes the current cyclic storage implementation useless for its purpose.

### Blocks

The page can be divided in several blocks. Only a whole block can be read or written at a time.

> For example, if a module has a table with 10 entries, and each entry is 64 bytes in size, it can request 10 blocks with a block size of 64 bytes. The module can also request one block that is 640 bytes depending on how it would like to access or alter the memory in persistent memory. The first option is preferred when it is a single entry that needs to be updated often and doesn't impact the other entries. The second option is preferred when table entries are not changed individually but have a common point of loading and storing data. 

Bluenet uses 1 big block per page, for flexibility of the stored data.

A better way could be to use smaller blocks and have functions to deal with values that are larger than 1 block. But whether this is actually better than the big block is unclear. Since methods `pstorage_store()`, and `pstorage_update()` can write less than a block.
Maybe a call to `pstorage_store()` uses the swap when size is less than the block size? --> Doesn't seem so from the diagram (on the _persistent storage manager_ page).

### Queuing

PStorage functions are asynchronous, but writes to flash can not happen simultaneously. This is why cs_Storage has a queue. This queue is also used to wait for high priority events to be done, like scanning and meshing.

# Decisions

Currently there is support for 255 mesh devices (this in contrast with 40 devices that we supported first).
This increases RAM uses by around 1500 bytes.
