# Installation


## Prerequisites

Bluenet uses a cmake build system, so you will need it:

    sudo apt-get install cmake


## Getting the Bluenet code

The best way is to first [fork](https://github.com/crownstone/bluenet/fork) the bluenet repository. Then create a workspace folder where all the necessary files and folders will be stored, e.g.

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

Different commands to upload to a board (flashing) use nrfjprog under the hood.

To clear the target device completely

    cd build
    make erase

To upload the softdevice

    cd build
    make softdevice

This will also erase the pages if there is already code on this.

To check the version of a softdevice

    make read_softdevice_version

If you type `make ` and then TAB there should be tab completion that shows the possible targets.

To upload the firwmare

    cd build/default
    make upload

It is necessary for the firmware to also write a hardware version

    cd build/default
    make write_hw_version

This can also be read by `make read_hw_version`. It allows Crownstone to run the same binary on all our devices and
decide per device what type of hardware is actually present. For example, on a Guidestone there will be no switch
functionality available.

The bootloader is build automatically when you build the firmware, it can also be flashed to the target board. The
value of its start address is written to UICR separately.

    cd build/default
    make upload_bootloader
    make write_bootloader_address

The bootloader requires a bootloader settings page that contains information about the current DFU process. It also
contains information about the installed application and the firmware version. Generate and upload it through:

    cd build/default
    make build_bootloader_settings
    make upload_bootloader_settings

To actually adjust the bootloader hex file so it contains the settings, run:

    make merge_bootloader_settings

Now it becomes possible to create a dfu package through:

    cd build/default
    make generate_dfu_package

Dependencies between these steps might need to be included.

To generate a DFU package, it is assumed that the key file is encrypted using the utility `pass`. It is possible to
overwrite the pass file per configuration:

    PASS_FILE=your_pass_store/dfu_pkg_signing_key

The contents of this file when executing on the command-line 

    pass your_pass_store/dfu_pkg_signing_key

should be something like:

    -----BEGIN EC PRIVATE KEY------
    ...
    ...
    -----END EC PRIVATE KEY------


## Summary

For your convenience, most commands, can be run from the build directory. To upload everything, you will for now need
the following series of commands. 

    cd build
    make erase
    make write_hw_version
    make softdevice
    make upload_bootloader
    make upload_bootloader_settings
    make write_bootloader_address
    make upload

If the dust settles we might have one single `make upload_all` command as well. Keep tight. :-)

## Debug

To start debugging the target, run first in a separate console:

    cd build/default
    make debug_server

Then run the debug session in another console:

    make debug

In a third console, you can also run an RTT Client

    make rtt_client

## Configuration

If you want to configure for a different target, go to the `config` directory in the workspace and copy the default
configuration files to a new directory, say `board0`.

    cd config
    cp -r default board0

Go to the build

    cd build
    cmake .. -DBOARD_TARGET=board0
    make

The other commands are as in the usual setting.

## Advanced

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

## Feedback

If there are bugs in the build process, please indicate so. 

The default installation procedure is done through continuous integration. If it is successful you will see a proper
building button:

[![Build Status](https://travis-ci.org/crownstone/bluenet.svg?branch=master)](https://travis-ci.org/crownstone/bluenet)

In that case, check your system to see if you've done something peculiar. :-)


