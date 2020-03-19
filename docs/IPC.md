# Inter-process communication
-----------------------------
Bluenet has a way for different processes (bootloader, application, arduino programs) to communicate with eachother.

This is done by allocating a section of RAM for IPC. This is done at the **end** of the RAM region.
In the RAM section, multiple items can be set and get by the different processes.
Each index will have a certain purpose.


## Types

Index | Type | Set by | Value
----- | ---- | ------ | -----
0     | Bootloader version | Bootloader | Char array, that forms the bootloader version as string.



## Relavant files

#### Shared
- Config file `CMakeBuild.config.default` determines the size of the IPC RAM.
- Files `cs_IpcRamData.h` and `cs_IpcRamData.cpp` implement functions to set and get items.

#### Application
- Linker file `generic_gcc_nrf52.ld` reserves the RAM, and creates a section.

#### Bootloader
- Linker file `secure_bootloader_gcc_nrf52.ld` reserves the RAM, and creates a section.


