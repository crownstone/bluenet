# Memory

Information and decisions with respect to memory on the Crownstone (both flash and ram).

To manage flash, bluenet makes use of the persistent storage manager (`fstorage` and `fds`) from the Nordic SDK.

## Memory layout on the nRF52832

The nRF52832 QFAA variant has 512 kB flash and 64 kB RAM. An individual page is 4 kB (`0x1000` in hex), hence there is space for 128 pages.

See the [default config file](https://github.com/crownstone/bluenet/blob/master/source/conf/cmake/CMakeBuild.config.default), the [default target file](https://github.com/crownstone/bluenet/blob/master/config/default/CMakeBuild.config), and the [bluenet linker file](https://github.com/crownstone/bluenet/blob/master/source/include/third/nrf/generic_gcc_nrf52.ld).

### Flash

The global layout of the flash memory is shown below for the nRF52832:

| Start address | What | Nr of pages | Config
| ------------- |:-------------:| -----:|:-----|
| 0x00000000 | MBR | 1 | fixed
| 0x00001000 | SD | 37 | location: fixed <br> size: implicit
| 0x00026000 | App / Bluenet | 54 | APPLICATION_START_ADDRESS<br> implicit
| 0x0005C000 | Free | 13 | implicit
| 0x00069000 | Microapp | 4 | FLASH_MICROAPP_BASE <br> FLASH_MICROAPP_PAGES
| 0x0006D000 | P2P DFU | 1 | location: implicit<br> size: fixed
| 0x0006E000 | Reserved for FDS expansion | 4 | location: implicit <br> CS_FDS_VIRTUAL_PAGES_RESERVED_BEFORE
| 0x00072000 | FDS | 4 | location: implicit <br> CS_FDS_VIRTUAL_PAGES
| 0x00076000 | Bootloader | 7 | BOOTLOADER_START_ADDRESS <br> BOOTLOADER_LENGTH
| 0x0007D000 | Reserved for bootloader expansion | 1 | location: implicit<br> size: fixed
| 0x0007E000 | MBR settings | 1 | CS_MBR_PARAMS_PAGE_ADDRESS <br> size: fixed
| 0x0007F000 | Bootloader settings | 1 | CS_BOOTLOADER_SETTINGS_ADDRESS <br> size: fixed
| | **Total** | 128


The maximum size of the app is available as `APPLICATION_MAX_LENGTH`.
For 54 + 13 pages this amounts to `(54 + 13) * 0x1000 = 0x43000`.
This is about 256 kB. For a dual bank bootloader the firmware should than be 132 kB max so the old firmware can exist
temporarily alongside the new firmware. This has not been achievable for us since a while, so no dual bank setup is in
use.
The build will fail if the app binary will go beyond the address `FLASH_MICROAPP_BASE` (`0x69000`).

### RAM

The layout for RAM memory is shown below for the nRF52832:

| Start address | What | Size | Size | Size (kB)
| ------------- |:----:| ----:| ----:| --------:|
| 0x20000000 | SD | 0x2A00 | 10752 | 10.5
| 0x20002A00 | App heap and stack | 0xCD00 | 52480 | 51.25
| 0x2000F700 | Microapp | 0x800 | 2048 | 2
| 0x2000FF00 | IPC | 0x100 | 256 | 0.25
| | **Total** | 0x10000 | 65536 | 64

The start and end addresses are like this:

| Start address | End address | What
| ------------- | ----------- | ----
| 0x20000000    | 0x20002A00  | Soft device.
| 0x20002A00    |             | App heap start, grows up to stack.
| 0x2000F6FF    |             | App stack start, grows down to heap.
| 0x2000F700    | 0x2000FEFF  | Microapp stack, grows from `end` down to `start`.
| 0x2000FF00    | 0x20010000  | IPC, fixed layout.

In bootloader mode, RAM is (see the [bootloader linker file](https://github.com/crownstone/bluenet/blob/master/source/bootloader/secure_bootloader_gcc_nrf52.ld)):

| Start address | What | Size | Size | Size (kB)
| ------------- |:-------------:| -----:| -----:| -----:|
| 0x20000000 | SD + offset | 0x3118 | 12568 | 12.273
| 0x20003118 | Bootloader heap/stack | 0xCDE8 | 52712 | 51.477
| 0x0000FF00 | IPC | 0x100 | 256 | 0.250
| | **Total** | 0x10000 | 65536 | 64

The offset is called `RAM_BOOTLOADER_START_OFFSET` and is of size `0x718`.
When the bootloader is running there are no microapps, hence that part of RAM can be safely used.

## Memory layout on the nRF52840

The nRF52840 has 1 MB flash and 256 kB RAM. An individual page is 4 kB (`0x1000` in hex), hence there is space for 256 pages.

See the [default config file](https://github.com/crownstone/bluenet/blob/master/source/conf/cmake/CMakeBuild.config.default), the [nrf52840 target file](https://github.com/crownstone/bluenet/blob/master/config/nrf52840/CMakeBuild.config), and the [bluenet linker file](https://github.com/crownstone/bluenet/blob/master/source/include/third/nrf/generic_gcc_nrf52.ld).

### Flash

The global layout of the flash memory is shown below for the nRF52840:

| Start address | What | Nr of pages | Config
| ------------- |:-------------:| -----:|:-----|
| 0x00000000 | MBR | 1 | fixed
| 0x00001000 | SD | 37 | location: fixed <br> size: implicit
| 0x00026000 | App / Bluenet | 54 | APPLICATION_START_ADDRESS<br> implicit
| 0x0005C000 | Free | 129 | implicit
| 0x00069000 | Microapp | 16 | FLASH_MICROAPP_BASE <br> FLASH_MICROAPP_PAGES
| 0x0006D000 | P2P DFU | 1 | location: implicit<br> size: fixed
| 0x0006E000 | Reserved for FDS expansion | 4 | location: implicit <br> CS_FDS_VIRTUAL_PAGES_RESERVED_BEFORE
| 0x00072000 | FDS | 4 | location: implicit <br> CS_FDS_VIRTUAL_PAGES
| 0x00076000 | Bootloader | 7 | BOOTLOADER_START_ADDRESS <br> BOOTLOADER_LENGTH
| 0x0007D000 | Reserved for bootloader expansion | 1 | location: implicit<br> size: fixed
| 0x0007E000 | MBR settings | 1 | CS_MBR_PARAMS_PAGE_ADDRESS <br> size: fixed
| 0x0007F000 | Bootloader settings | 1 | CS_BOOTLOADER_SETTINGS_ADDRESS <br> size: fixed
| | **Total** | 256

The microapp section is larger than on the nRF52832.

The maximum size of the app is available as `APPLICATION_MAX_LENGTH`.
For 54 + 141 pages this amounts to `(54 + 129) * 0x1000 = 0xB7000`.
This is about 732 kB. For a dual bank bootloader the firmware should than be 366 kB max so the old firmware can exist
temporarily alongside the new firmware. This is possible on the nRF52840, but that doesn't mean that it's in use.
The build will fail if the app binary will go beyond the address `FLASH_MICROAPP_BASE` (`0xDD000`).

### RAM

The layout for RAM memory is shown below for the nRF52840:

| Start address | What | Size | Size | Size (kB)
| ------------- |:----:| ----:| ----:| --------:|
| 0x20000000 | SD | 0x2A00 | 10752 | 10.5
| 0x20002A00 | App heap and stack | 0x3CD00 | 249088 | 243.25
| 0x2000F700 | Microapp | 0x800 | 2048 | 2
| 0x2000FF00 | IPC | 0x100 | 256 | 0.25
| | **Total** | 0x40000 | 262144 | 256

# Miscellaneous

There is support for 255 mesh devices (this in contrast with 40 devices that we supported first).
This increases RAM usage by around 1500 bytes.
