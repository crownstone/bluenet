# Installation


## Prerequisites

Bluenet uses a cmake build system, so you will need it:

    sudo apt-get install cmake


## Getting the Bluenet code

The best way is to first [fork](https://github.com/dobots/bluenet/fork) the bluenet repository. Then create a workspace folder where all the necessary files and folders will be stored, e.g.

    mkdir ~/bluenet-workspace

and download the code (We recommend to download the code into the workspace, but you can place it anywhere you want):

    cd ~/bluenet-workspace
    git clone https://github.com/YOUR_GITHUB_USERNAME/bluenet source

and set the correct upstream:

    cd source
    git remote add upstream git@github.com:crownstone/bluenet.git


## Setup

The installation is changing to use more CMake instead of bash scripts. To start, you can simply use:

    mkdir build && cd build
    cmake .. & make

This will download the required programs and libraries. The installation has been tested on Ubuntu 18.04 (should work on newer ones as well) and assumes you use a J-Link programmer.


## Configuration

We allow for multiple configurations. This means that the config dir can contain multiple directories. A config directory `default` is already included, with a `CMakeBuild.config` that is a copy of `source/conf/cmake/CMakeBuild.config.template`. Open it to customize:

    xdg-open config/default/CMakeBuild.config

The following variables have to be set before you can build the code:

- Set `BLUETOOTH_NAME` to something you like, but make sure it's short.
- Set `HARDWARE_BOARD` to the board you're using. This determines the pin layout.

Last, copy any lines that you want to adjust over from the default configuration: `source/conf/cmake/CMakeBuild.config.default`.


### Advanced Configuration

If you have problems with the path of the JLink tool, you can adjust the paths of the JLinkExe and JLinkGdbServer:

- Set `JLINK` to the correct JLink Exe file. (Default: `/usr/bin/JLinkExe`)
- Set `JLINK_GDB_SERVER` to the correct file. (Default: `/usr/bin/JLinkGDBServer`)

If you installed a different compiler than the one we recommended above, adjust the compiler type:

- Set `COMPILER_TYPE` to the correct compiler type  (Default: `arm-none-eabi`)

If you want to download and use a different softdevice, you will need to adjust the values above:

- Set `SOFTDEVICE` (basename of file without `_softdevice.hex`)
- Set `SOFTDEVICE_SERIES`, `SOFTDEVICE_MAJOR`, and `SOFTDEVICE_MINOR` accordingly.
- Set `APPLICATION_START_ADDRESS`, `APPLICATION_LENGTH`, `RAM_R1_BASE` and `RAM_APPLICATION_AMOUNT` to the correct values (can be found in the Softdevice Specification Document)

And set the following variables to the correct paths:

- Set `SOFTDEVICE_DIR` to wherever you unzipped the SoftDevice dir from Nordic.
- Set `SOFTDEVICE_DIR_API` to the directory with the SoftDevice include dir. This is relative, `${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}/` will be the used path.
- Set `SOFTDEVICE_DIR_HEX` to the directory with the SoftDevice .hex file. This is relative, `${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/` will be the used path. (use `/` if the .hex file is in the same folder as `SOFTDEVICE_DIR`)

- If you run into memory issues, you can play around with `HEAP_SIZE`. Increasing the heap size (dynamic memory), will reduce the stack size (static memory).


## Usage

Once everything is installed and configured, the code can be compiled and uploaded.
You will have to attach a programmer/debugger, like the JLink. Towards that you only need four pins: `GND`, `3V`, `SWDIO / RESET`, and `SWCLK / FACTORY`. The pin layout of the JLink connector is written out on the [Crownstone blog](https://crownstone.rocks/2015/01/23/programming-the-nrf51822-with-the-st-link).


### Compiling, uploading and debugging

These commands use the `nrfjprog` binary under the hood. It writes the firmware to a Crownstone or a development board. The configuration is obtained from the `$BLUENET_CONFIG/default/CMakeBuild.config` file. Hence, the target here is `default`.

First erase (`-e`) the flash memory:

    cd $BLUENET_DIR/scripts
    ./bluenet.sh -e -t default

After erasing, you have to upload (`-u`) the hardware board version in a `UICR` register (`-H`):

    cd $BLUENET_DIR/scripts
    ./bluenet.sh -u -H -t default

Then build (`-b`) and upload (`-u`) the SoftDevice (`-S`):

    cd $BLUENET_DIR/scripts
    ./bluenet.sh -b -S -t default
    ./bluenet.sh -u -S -t default

Now we can build (`-b`) our own application/firmware (`-F`):

    cd $BLUENET_DIR/scripts
    ./bluenet.sh -b -F -t default

By default, the code is built inside the `$BLUENET_WORKSPACE_DIR/build` folder and if successful, the compiled binaries (\*.hex, \*.elf, \*.bin) are copied to `$BLUENET_WORKSPACE_DIR/bin`. If you want to change either folder, you can uncomment and assign the following environment variables in `$BLUENET_DIR/env.config`

    - `BLUENET_BUILD_DIR`. Set this variable to the path where the build files should be stored.
    - `BLUENET_BIN_DIR`. Set this variable to the path where the compiled binaries should be stored

If the compilation fails due to misconfiguration, you might have to remove the build dir and then try again.

To upload (`-u`) the application/firwmare (`-F`) with the JLink you can use:

    ./bluenet.sh -u -F -t default

To debug (`-d`) the firmware (`-F`) with the cross-compiled gdb tool you can use:

    ./bluenet.sh -d -F -t default

### Combinations

You can use multiple commands in one go. For example, to erase the flash, build and upload the board version, firmware and softdevice, and lastly debug the firmware, you can use the following command:

    ./bluenet.sh -bud -FSH -t default

You can also build and upload a binary that combines the application, the softdevice, the bootloader, and board version.

    ./bluenet.sh -bu -FSBHC -t default

### Advanced Usage

The above scripts and configuration are sufficient if you work with one device and one configuration file. However, if you want to quickly switch between different configurations or have more than one device connected, you might want to try out the support for multiple targets.

#### Local config file

There are options which are the same for all configurations, such as the paths (`NRF5_DIR`, `COMPILER_PATH`, etc.). Instead of defining them in every configuration file, you can create a file `$BLUENET_DIR/CMakeBuild.config.local` and store common configuration values there.
This file is optional and not required. If it is available, it will overwrite the default values from `$BLUENET_DIR/CMakeBuild.config.default`. The order of precedence of the configuration values in the 3 files is as follows:

1. Values are loaded from `$BLUENET_DIR/conf/cmake/CMakeBuild.config.default`
2. Values are loaded from `$BLUENET_DIR/conf/cmake/CMakeBuild.config.local`. This file is only read if it is present. It overwrites the values loaded at step 1.
3. Values are loaded from `$BLUENET_CONFIG_DIR/conf/cmake/CMakeBuild.config`. Values read here will overwrite values loaded at step 1 and 2.

#### Different Configurations

If you quickly want to switch between different configurations, you can create sub folders in the `$BLUENET_CONFIG_DIR` with different names. E.g. let's create two additional config files, one called BLUE and one called RED (We call these henceforth targets, eg. target `crownstone-blue` and target `crownstone-red`). You can create two subdirectories:

    mkdir -p $BLUENET_CONFIG_DIR/crownstone-blue
    mkdir -p $BLUENET_CONFIG_DIR/crownstone-red

Then create in each subdirectory a `CMakeBuild.config` file (or copy it over from an existing file, the template or the default).

The new structure in the `$BLUENET_CONFIG_DIR` would now be:

    $BLUENET_CONFIG_DIR
      crownstone-blue
        CMakeBuild.config
      crownstone-red
        CMakeBuild.config

If you require different debug sessions per target, you will need to specify this in a `_target.sh` file. It allows you to set non-default `serial_num` and `gdb_port` values. See, next section.

    cp $BLUENET_DIR/scripts/_targets_template.sh $BLUENET_CONFIG_DIR/_targets.sh

You can call the scripts with the target (`-t target`) just as indicated above as well.

E.g. to build and upload target `crownstone-blue`, execute:

    ./bluenet.sh -bu -F -t crownstone-blue

and to build, upload and debug target `crownstone-red` call:

    ./bluenet.sh -bud -F -t crownstone-red

### Different Devices

To take the case of the different configurations a bit further, we can also assign the different configurations to different devices. E.g. if you are using 2 PCA10000 boards, each one has a different serial number. If you try to upload firmware to one of the devices, while having both plugged in, JLink will return with an error because it doesn't know to which device to upload the firmware. To solve this issue, follow the steps aboth to create two new configurations. (target `crownstone-blue` and target `crownstone-red`)

After completing the steps above, open the file `$BLUENET_CONFIG_DIR/_targets.sh` and between the lines

    case "$target" in
    ...
    esac

add your new targets as

    crownstone-blue)
      serial_num=<SERIAL NUMBER DEVICE1>
      ;;
    crownstone-red)
      serial_num=<SERIAL NUMBER DEVICE2>
      ;;

Note: It's also a good practice to define one of the devices as the default, which will be used if no target is supplied to the scripts, or if you want to add different configurations, without adding a new entry every time:

    crownstone-blue)
      serial_num=<SERIAL NUMBER DEVICE1>
      ;;
    crownstone-red)
      serial_num=<SERIAL NUMBER DEVICE2>
      ;;
    *)
      serial_num=<DEFAULT SERIAL NUMBER>
      ;;


To obtain the serial number of the JLink devices, run

    JLinkExe

and then enter `ShowEmuList`, which gives a list of connected devices, e.g.

    JLink> ShowEmuList
    J-Link[0]: Connection: USB, Serial number: 680565191, ProductName: J-Link OB-SAM3U128-V2-NordicSem
    J-Link[1]: Connection: USB, Serial number: 518005793, ProductName: J-Link Lite Cortex-M-9
    JLink>

Now if you call one of the scripts, with target `crownstone-blue`,  e.g. `./bluenet.sh -bu -F -t crownstone-blue` it will compile using config `crownstone-blue` at `$BLUENET_CONFIG_DIR/crownstone-blue/CMakeBuild.config` and upload it to `DEVICE1`. While calling `./bluenet.sh -bu -F -t crownstone-red` will compile using config `crownstone-red` at `$BLUENET_CONFIG_DIR/crownstone-red/CMakeBuild.config` and upload it to `DEVICE2`.

## Unit tests

You can also run unit tests for host or the target board through:

    ./bluenet.sh --unit_test_host -t test
    ./bluenet.sh --unit_test_nrf5 -t test

Set `TEST_TARGET=nrf5` in your `CMakeBuild.config` file in `test` to actually run the unit tests.

## UART

The main way to debug your application is through UART. The TX pin is defined by the board type you configured (`HARDWARE_BOARD`), for each board type the TX pin nr can be found in the `include/cfg/cs_Boards.h` file.

In case you happen to have the nRFgo Motherboard (nRF6310, strongly recommended) you can easily connect the pins at P2.0 and P2.1 to respectively the pins RXD and TXD on P15 on the board. Do not forget to switch on the RS232 switch. Subsequently you will need some RS232 to USB cable if you haven't an extremely old laptop.

The current set baudrate you can find in `cs_Serial.cpp` and is `230400` baud. To read from serial you can for example use `minicom` (replace ttyUSB0 with the correct device):

    minicom -b 230400 -c on -D /dev/ttyUSB0

If this requires sudo rights, you may want to add yourself to the dialout group:

    sudo adduser $USER dialout

You will have to logout and login for it to have effect.

If all goes well, you should see text appearing whenever the crownstone is rebooted/reset. This also shows you which services and characteristics are enabled.

The verbosity of the debug ouptut can be set in the CMakeBuild.conifg through the variable `SERIAL_VERBOSITY`. Highest level is `DEBUG`, lowest level is `FATAL`. If you want to disable logging totally, set verbosity to `NONE`.

For verbosity level `DEBUG`, you can add even more verbosity per class files by enabling the flags in `/include/cfg/cs_Debug.h`.

## Troubleshooting

Suppose, there's the following error in the build process:

    arm-none-eabi-g++: error trying to exec 'cc1plus': execvp: No such file or directory
    CMakeFiles/crownstone.dir/build.make:144: recipe for target 'CMakeFiles/crownstone.dir/src/ble/cs_ServiceData.cpp.obj' failed
    make[3]: *** [CMakeFiles/crownstone.dir/src/ble/cs_ServiceData.cpp.obj] Error 1

This very likely means that although the cross-compiler itself has been found, the shared libraries are not. Make sure that the configuration file, say `$BLUENET_CONFIG_DIR/default/CMakeBuild.config`, contains the right `COMPILER_PATH`, for example:

    COMPILER_PATH=/opt/compilers/gcc-arm-none-eabi-7-2018-q2-update
    
Both the directories `bin` and `lib` should be present in this directory. Check with `ldd` if the right `.so` files are linked.
