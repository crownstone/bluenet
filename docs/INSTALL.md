# Installation


## Prerequisites

Bluenet uses the `cmake` build system, `git` as versioning system, and `wget` to retrieve other files.

    sudo apt install cmake git wget python3 python3-pip


## Getting the Bluenet code

The best way is to first [fork](https://github.com/crownstone/bluenet/fork) the bluenet repository.

<p align="center">
  <a href="https://github.com/crownstone/bluenet/fork">
    <img src="https://img.shields.io/badge/fork-bluenet-blue" alt="Fork">
  </a>
</p>

Create a workspace folder where all the necessary files and folders will be stored, e.g.

    mkdir -p ~/workspace

and download the code:

    git clone https://github.com/YOUR_GITHUB_USERNAME/bluenet

and set the correct upstream:

    cd bluenet
    git remote add upstream git@github.com:crownstone/bluenet.git

## Setup

The installation uses CMake. To start, you can simply use:

    cd bluenet
    mkdir build && cd build
    cmake .. && make

This will download the required programs and libraries. The installation has been tested on Ubuntu 18.04 (should work on newer ones as well) and assumes you use a J-Link programmer.

By default a `default` target will be build. It is possible to build for that target by navigating into its build
directory:

    cd build/default
    make

## Flashing

Different commands to write to a board (flashing) use `nrfjprog` under the hood.

To clear the target device completely:

    cd build
    make erase

To write the softdevice:

    cd build
    make write_softdevice

This will also erase the pages if there is already code on this.

To check the version of a softdevice:

    make read_softdevice_version

If you type `make ` and then TAB there should be tab completion that shows the possible targets.

The softdevice makes use of a so called master boot record (MBR). This provides the softdevice, bootloader, and firmware some functions. It requires a parameter page, which it finds by looking at the UICR. We write the address with:

    make write_mbr_param_address

Before writing the application or bootloader, the board version should be written. It allows Crownstone to run the same binary on all our devices and decide per device what type of hardware is actually present. For example, on a Guidestone there will be no switch functionality available.
It has to be written before the application runs, else the application will write a default.

    cd build/default
    make write_board_version

The bootloader is build automatically when you build the firmware, it can also be flashed to the target board. The value of its start address is written to UICR separately.

To write the bootloader and its address:

    cd build/default/bootloader
    make write_bootloader
    make write_bootloader_address

Then write the application:

    cd build/default
    make write_application

The bootloader requires a bootloader settings page that contains information about the current DFU process. It also contains information about the installed application and its version. The bootloader verifies if the application is correct by checking it against the bootloader settings, so each time the application changes, you the bootloader settings also change.

To build and write the bootloader settings:

    cd build/default
    make build_bootloader_settings
    make write_bootloader_settings

Now that everything is written, the board has to be reset:

    cd build
    make reset

## DFU (over the air updates)

In order to create DFU packages, the firmware has to be signed. For this, [pass](https://www.passwordstore.org/) is used to store the signing keys.

TODO: make target to generate signing key.

Now it becomes possible to create a dfu package to update the application through:

    cd build/default
    make generate_dfu_package_application

Dependencies between these steps might need to be included.

It is possible to set what entry in `pass` is used:

    PASS_FILE=your_pass_store/dfu_pkg_signing_key

The contents of this file when executing on the command-line:

    pass your_pass_store/dfu_pkg_signing_key

should be something like:

    -----BEGIN EC PRIVATE KEY------
    ...
    ...
    -----END EC PRIVATE KEY------


## Summary

All build specific commands should be run from the target directory, while non-build specific commands can be run from the build directory.
To write everything, for now you will need the following series of commands.

    cd build
    make erase
    make write_softdevice
    cd default
    make write_board_version
    make write_application
    make write_bootloader_settings
    cd bootloader
    make write_bootloader
    make write_bootloader_address

If the dust settles we might have one single `make write_all` command as well. Keep tight. :-)

## Logs

You can find more on logs (over UART) in [this document](LOGGING.md).

## Debug

To start debugging the target, run first in a separate console:

    cd build/default
    make debug_server

Then run the debug session in another console:

    make debug_application

In a third console, you can also run an RTT Client

    make rtt_client

## Configuration

If you want to configure for a different target, go to the `config` directory in the workspace and copy the default configuration files to a new directory, say `board0`.

    cd config
    cp -r default board0

Go to the build

    cd build
    cmake .. -DBOARD_TARGET=board0
    make

The other commands are as in the usual setting.

Note, now, if you change a configuration setting you will also need to run `CMake` again.
It picks up the configuration settings at configuration time and then passes them as defines to `CMake` in the bluenet directory.

    cd build
    cmake ..
    make

Only after this you can assume that the `make` targets in `build/default` or any other target are up to date.

## Advanced

### Overwrites and runtime configs

The following is convenient if you have multiple boards attached to your system.

It is possible to have a second file in your target directory that overwrites values in your `CMakeBuild.config`.
The file is called `CMakeBuild.overwrite.config`.

For example to flash to two different boards, both attached to a USB port.

    cd build
    make list_jlinks

Note the serial number of your JLink device and add it to for example `config/default/CMakeBuild.overwrite.config`.

    SERIAL_NUM=682450212

On the moment there is no rebuild triggered when the values in this file change.

Another convenient variable to set there is `GDB_PORT`. To have both running in parallel:

    cd build
    cmake .. -DBOARD_TARGET=board0 && make
    cd build/board0
    make debug_server
    cmake .. -DBOARD_TARGET=board1 && make
    cd build/board1
    make debug_server

For in particular the above information you perhaps do not want to have everything recompiled just because you use
a particular JLINK device. The file that is used for runtime only is `CMakeBuild.runtime.config`. If you write
something like `SERIAL_NUM=682450212` in that file this can be altered without causing `cmake` to run again.

### Wireless

Information about the MAC address can be obtained through the make system as well:

    make read_mac_address

The address is stored by Nordic as 48 bits in two registers, `DEVICEADDR[0]` and `DEVICEADDR[1]`. Depending on the last two bits, this is a static or a private address (resolvable or non-resolvable). Those bits are normally set to 00, but due to the fact that the Crownstones are static addresses, we set them to 11. This means we can use the result to connect to the Crownstone (for the setup process for instance).

Note that reading out UICR/FICR registers can cause the firmware to halt. Run `make reset` afterwards.

### Setup

It is possible to make use of the `csutil` utility which can be sourced through a cmake flag:

    cmake .. -DDOWNLOAD_CSUTIL=ON      # and other flags

You can prepare the configuration used to set up a Crownstone by

    make write_config

Or individually as

    BLUENET_CONFIG=MAC_ADDRESS make write_config
    BLUENET_CONFIG=IBEACON_MINOR make write_config
    ...

This will write a `CMakeBuild.dynamic.config` file locally. To perform the setup, run

    make setup

This will generate a `config.json` file with the proper information and use the `csutil` tool to perform the setup. You can also do a factory reset.

    make factory_reset

The `csutil` tool uses the Crownstone BLE library under the hood. That means that you can also setup hardware that is not attached to your machine. In that case make sure you have the right config values set (e.g. in `CMakeBuild.runtime.config`).

## Common issues

On older ubuntu versions, python2.7 is the default version. This gives issues with nrfutil.

This can be fixed by installing pip2, removing nrfutil from pip3, and installing nrfutil with pip2.


## Maintainers

Not for everyday users, just for maintainers of this code base, there are more options.
A few of the options are described in the [Build System](BUILD_SYSTEM.md) document.

There are a few other options which can be found as well by typing `make`, a space and then TAB. Some examples:

For a linter (cppcheck):

    make check

For documentation:

    make generate_documentation
    make view_documentation

By the way, to remove dependency checking, `CMake` introduces for each target also a fast variant. For example, the `debug` target depends on the `${PROJECT_NAME}.elf` target

    make debug

To actually only run `debug`, just type:

    make debug/fast

This option exists for almost all targets.

## Feedback

If there are bugs in the build process, please indicate so.

The default installation procedure is done through continuous integration. If it is successful you will see a proper building button:

[![Build Status](https://travis-ci.org/crownstone/bluenet.svg?branch=master)](https://travis-ci.org/crownstone/bluenet)

In that case, check your system to see if you've done something peculiar. :-)


