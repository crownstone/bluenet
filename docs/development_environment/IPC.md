# Inter-process communication

Bluenet has a way for different processes (bootloader, application, arduino programs) to communicate with each other.

This is done by allocating a section of RAM for IPC. This is done at the **end** of the RAM region.
In the RAM section, multiple items can be set and get by the different processes.
Each index will have a certain purpose.

## Relevant files


### Shared

- Config file `CMakeBuild.config.default` determines the size of the IPC RAM.
- Files `cs_IpcRamData.h` and `cs_IpcRamData.cpp` implement functions to set and get items.


### Application

- Linker file `generic_gcc_nrf52.ld` reserves the RAM, and creates a section.


### Bootloader

- Linker file `secure_bootloader_gcc_nrf52.ld` reserves the RAM, and creates a section.


## General purpose retention register

The general purpose retention register (GPREGRET) is used for communication between bootloader and application, as well as to store data that survives a reboot.

How this is done can be read in `cs_GpRegRetConfig.h`.
