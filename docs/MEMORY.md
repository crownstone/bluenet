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
| 0x0005C000 | Free | 15 | implicit
| 0x0006B000 | P2P DFU | 1 | location: implicit<br> size: fixed
| 0x0006C000 | Microapp | 4 | FLASH_MICROAPP_BASE <br> FLASH_MICROAPP_PAGES
| 0x00070000 | Reserved for FDS expansion | 2 | location: implicit <br> CS_FDS_VIRTUAL_PAGES_RESERVED_BEFORE
| 0x00072000 | FDS | 4 | location: implicit <br> CS_FDS_VIRTUAL_PAGES
| 0x00076000 | Bootloader | 7 | BOOTLOADER_START_ADDRESS <br> BOOTLOADER_LENGTH
| 0x0007D000 | Reserved for bootloader expansion | 1 | location: implicit<br> size: fixed
| 0x0007E000 | MBR settings | 1 | CS_MBR_PARAMS_PAGE_ADDRESS <br> size: fixed
| 0x0007F000 | Bootloader settings | 1 | CS_BOOTLOADER_SETTINGS_ADDRESS <br> size: fixed
| | **Total** | 128


The maximum size of the app is available as `APPLICATION_MAX_LENGTH`, and is the sum of app + free.
For a dual bank bootloader, the application size should be smaller than the free space.

The build will fail if the app binary will go beyond the P2P DFU start address.

### RAM

The layout for RAM memory is shown below for the nRF52832:

| Start address | What | Size | Size | Size (kB)
| ------------- |:----:| ----:| ----:| --------:|
| 0x20000000 | SD | 0x2A60 | 10848 | 10.6
| 0x20002A60 | Application | 0xC4A0 | 50336 | 49.2
| 0x2000EF00 | Microapp | 0x1000 | 4086 | 4
| 0x2000FF00 | IPC | 0x100 | 256 | 0.25
| | **Total** | 0x10000 | 65536 | 64

The bluenet heap starts at the start of application ram and grows up to the bluenet stack.
The stack starts at the end of application ram and grows down to the heap.
The microapp stack grows down from the end of microapp ram.

In bootloader mode, RAM is (see the [bootloader linker file](https://github.com/crownstone/bluenet/blob/master/source/bootloader/secure_bootloader_gcc_nrf52.ld)):

| Start address | What | Size | Size | Size (kB)
| ------------- |:-------------:| -----:| -----:| -----:|
| 0x20000000 | SD | 0x3178 | 12664 | 12.4
| 0x20003178 | Bootloader | 0xCD88 | 52616 | 51.4
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
| 0x0005C000 | Free | 118 | implicit
| 0x000D2000 | P2P DFU | 1 | location: implicit<br> size: fixed
| 0x000D3000 | Microapp | 4 | FLASH_MICROAPP_BASE <br> FLASH_MICROAPP_PAGES
| 0x000D7000 | Reserved for more microapps | 12 | implicit <br>
| 0x000E3000 | Reserved for FDS expansion | 8 | location: implicit <br> CS_FDS_VIRTUAL_PAGES_RESERVED_BEFORE
| 0x000EB000 | FDS | 4 | location: implicit <br> CS_FDS_VIRTUAL_PAGES
| 0x000EF000 | Bootloader | 7 | BOOTLOADER_START_ADDRESS <br> BOOTLOADER_LENGTH
| 0x000F6000 | Reserved for bootloader expansion | 8 | location: implicit<br> size: fixed
| 0x000FE000 | MBR settings | 1 | CS_MBR_PARAMS_PAGE_ADDRESS <br> size: fixed
| 0x000FF000 | Bootloader settings | 1 | CS_BOOTLOADER_SETTINGS_ADDRESS <br> size: fixed
| | **Total** | 256

The microapp section is larger than on the nRF52832.

The maximum size of the app is available as `APPLICATION_MAX_LENGTH`, and is the sum of app + free.
For a dual bank bootloader, the application size should be smaller than the free space.
The build will fail if the app binary will go beyond the P2P DFU start address.

### RAM

The layout for RAM memory is shown below for the nRF52840:

| Start address | What | Size | Size | Size (kB)
| ------------- |:----:| ----:| ----:| --------:|
| 0x20000000 | SD | 0x2AA8 | 10920 | 10.7
| 0x20002AA8 | Application | 0x3C458 | 246872 | 241.1
| 0x2003EF00 | Microapp | 0x1000 | 4096 | 4
| 0x2003FF00 | IPC | 0x100 | 256 | 0.25
| | **Total** | 0x40000 | 262144 | 256

# Miscellaneous

There is support for 255 mesh devices (this in contrast with 40 devices that we supported first).
This increases RAM usage by around 1500 bytes.
