# Installation

This is a step-by-step instruction to install the bluenet build system. If you prefer a simple install script, you can use the `install.sh` script provided in the [crownstone-sdk](https://github.com/crownstone/crownstone-sdk#bluenet_lib_configs) repository.

The installation has been tested on Ubuntu 14.04 and assumes you use the J-Link Lite CortexM-9 programmer.

## Prerequisites

To compile and run the bluenet firmware, the following prerequisites are needed:

1. Nordic SDK
2. Cross-compiler for ARM
3. JLink utilities from Segger

### Nordic SDK

IMPORTANT NOTE: THIS VERSION OF BLUENET IS ONLY COMPATIBLE WITH THE NEWER CHIP NRF52. USE THE OLDER BRANCH SDK_8 FOR THE NRF51 CHIPS!

Download Nordic's SDK and unzip:

- [Nordic nRF5 SDK 11](https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v11.x.x/)

By default, we use the Softdevices provided in the Nordic SDK. For SDK 11, this is the [Nordic S132 Softdevice 2.0](https://www.nordicsemi.com/eng/Products/S132-SoftDevice), so you don't need to download any softdevices separately. However, if you want to use a different Softdevice version, you can download it and later adapt the config to use the new softdevice.

There is a bug in Nordic's SDK code, search for files named `nrf_svc.h`.
In those files, replace the assembly line:

    #define GCC_CAST_CPP (uint8_t)

With:

    #define GCC_CAST_CPP (uint16_t)

You can do that manually, or use

    perl -p -i -e 's/#define GCC_CAST_CPP \(uint8_t\)/#define GCC_CAST_CPP \(uint16_t\)/g' `grep -ril "#define GCC_CAST_CPP (uint8_t)" *`

from the root folder of the Nordic SDK to do it for you.

In SDK 11, there is another bug that has to be fixed. In the file `nrf_drv_saadc.h`, replace the define of high limit disabled with:

    #define NRF_DRV_SAADC_LIMITH_DISABLED (4095)



### J-Link

Download and install J-Link Segger’s [software](https://www.segger.com/downloads/jlink). The current version is V6.34f (direct link to [64 bit .deb](https://www.segger.com/downloads/jlink/JLink_Linux_x86_64.deb)) and can be found at the "J-Link Software and Documentation Pack" section. 

Then install it:

    sudo dpkg -i JLink_Linux_x86_64.deb

Check the [Segger forums](http://forum.segger.com/index.php?page=Thread&threadID=4089) for some help with the udev rules (there is one that comes with the Segger software). One thing that you might want to do is disable or deinstall the modemmanager on Ubuntu: `sudo systemctl disable ModemManager.service`.
Note that it might be the case that UART is not enabled (idProduct `0101` rather than `0105`). You can enable this by typing `vcom enable` on the JLink command line.

### Cross compiler

The GCC cross-compiler for ARM can be found at the [ARM website](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). It's now at version 7, in Q2 2018: [64 bit tar ball](https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2?revision=bc2c96c0-14b5-4bb4-9f18-bceb4050fee7?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,7-2018-q2-update). You might also just use the cross-compiler if it's in a PPA. It's nice if you have one with Python support enabled so you can use this [GDB dashboard](https://github.com/cyrus-and/gdb-dashboard), but this is optional.

Assuming you have a 64 bit system, you might have to install 32 bit packages:

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install libstdc++6:i386 libncurses5:i386

If the cross-compiler does not work, make sure you check if all its dependencies are met with `ldd arm-none-eabi-gcc`.

### Misc.

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

Next make a dir for your config file(s), by default, this should be called `config` and be placed inside the workspace.

    mkdir ~/bluenet-workspace/config

Now we need to set up the environment variables to keep track of the different folders required to build bluenet

    cd ~/bluenet_workspace/source
    cp conf/cmake/env.config.template env.config

Open the file then uncomment and assign the variable `BLUENET_WORKSPACE_DIR` to your workspace path, e.g.

    BLUENET_WORKSPACE_DIR=~/bluenet_workspace

And you can - if you like to - point to all folders independently:

    BLUENET_DIR=~/bluenet_workspace/source
    BLUENET_CONFIG_DIR=~/bluenet_workspace/config
    BLUENET_BIN_DIR=~/bluenet_workspace/bin
    BLUENET_BUILD_DIR=~/bluenet_workspace/build
    BLUENET_RELEASE_DIR=~/bluenet_workspace/release

Last we want to load the environments by default for every terminal session with the following command:

    echo "source ~/bluenet_workspace/source/scripts/env.sh" >> ~/.bashrc

Apply the environment variables:

    source ~/.bashrc

If you have another shell, please do the above for your own shell.

## Get the mesh code

The mesh code we use is a modification of Nordics OpenMesh.

    cd $BLUENET_WORKSPACE_DIR
    git clone -b bluenet https://github.com/crownstone/nRF51-ble-bcast-mesh

## Configuration

Copy the template config file to your config directory:

    cp $BLUENET_DIR/conf/cmake/CMakeBuild.config.template $BLUENET_CONFIG_DIR/CMakeBuild.config

then open it to customize

    gedit $BLUENET_CONFIG_DIR/CMakeBuild.config &

The following variables have to be set before you can build the code:

- Set `BLUETOOTH_NAME` to something you like, but make sure it's short (<9).
- Set `HARDWARE_BOARD` to the board you're using. This determines the pin layout.
- Set `COMPILER_PATH` to the path where the compiler can be found (it should contain the `/bin` subdir).
- Set `NRF51822_DIR` to wherever you installed the Nordic SDK. It should have the following subdirectories:
    - components
    - documentation
    - examples
    - external
    - SVD

Last, copy any lines that you want to adjust over from the default configuration.

E.g.

- enable meshing by setting `BUILD_MESHING=1` and `MESHING=1`
- enable device scanner by setting `INTERVAL_SCANNER_ENABLED=1`
- enable the power service by setting `POWER_SERVICE=1`
- set serial verbosity to debug by setting `SERIAL_VERBOSITY=DEBUG`

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

First build and upload the SoftDevice:

    cd $BLUENET_DIR/scripts
    ./softdevice.sh build
    ./softdevice.sh upload

Now we can build our own software:

    cd $BLUENET_DIR/scripts
    ./firmware.sh --command=build --target=default

By default, the code is built inside the `$BLUENET_WORKSPACE_DIR/build` folder and if successful, the compiled binaries (\*.hex, \*.elf, \*.bin) are copied to `$BLUENET_WORKSPACE_DIR/bin`. If you want to change either folder, you can uncomment and assign the following environment variables in `$BLUENET_DIR/env.config`

    - `BLUENET_BUILD_DIR`. Set this variable to the path where the build files should be stored.
    - `BLUENET_BIN_DIR`. Set this variable to the path where the compiled binaries should be stored

If the compilation fails due to misconfiguration, you might have to remove the build dir and then try again.

To upload the application with the JLink you can use:

    ./firmware.sh --command=upload --target=default

To debug with `gdb` you can use:

    ./firmware.sh --command=debug --target=default

There is a shortcut to build and upload you can use:

    ./firmware.sh --command=run --target=default

And another shortcut to build, upload, and debug:

    ./firmware.sh --command=all --target=default

### Uploading combined files

You can also upload a binary that combines the application, the softdevice, and the bootloader. Let us explain these with short options (`-c` rather than `--command` for example). The following uploads the softdevice (`-s`), and the bootloader (`-b`) as well:

    ./firmware.sh -c upload -t default -s -b

This uploads all these binaries separately. It is also possible to combine everything into one binary (`-u`, or `--use_combined`):

    ./firmware.sh -c upload -t default -u
    
In the combined binary the hardware version is set as well in a `UICR` register. It also writes an app valid bit.

### Advanced Usage

The above scripts and configuration are sufficient if you work with one device and one configuration file. However, if you want to quickly switch between different configurations or have more than one device connected, you might want to try out the support for multiple targets.

#### Local config file

There are options which are the same for all configurations, such as the paths (`NRF51822_DIR`, `COMPILER_PATH`, etc.). Instead of defining them in every configuration file, you can create a file `$BLUENET_DIR/CMakeBuild.config.local` and store common configuration values there.
This file is optional and not required. If it is available, it will overwrite the default values from $BLUENET_DIR/CMakeBuild.config.default`. The order of precedence of the configuration values in the 3 files is as follows:

1. Values are loaded from `$BLUENET_DIR/conf/cmake/CMakeBuild.config.default`
2. Values are loaded from `$BLUENET_DIR/conf/cmake/CMakeBuild.config.local`. This file is only read if it is present. It overwrites the values loaded at step 1.
3. Values are loaded from `$BLUENET_CONFIG_DIR/conf/cmake/CMakeBuild.config`. Values read here will overwrite values loaded at step 1 and 2.

#### Different Configurations

If you quickly want to switch between different configurations, you can create sub folders in the `$BLUENET_CONFIG_DIR` with different names. E.g. let's create two additional config files, one called BLUE and one called RED (We call these henceforth targets, eg. target BLUE and target RED). You can create two subdirectories

- `$BLUENET_CONFIG_DIR/BLUE`
- `$BLUENET_CONFIG_DIR/RED`

Then create in each subdirectory a file called CMakeBuild.config (or copy it over from an existing file, the template or the default).

The new structure in the `$BLUENET_CONFIG_DIR` would now be:

- `$BLUENET_CONFIG_DIR`
    - `BLUE`
        - `CMakeBuild.config`
    - `RED`
        - `CMakeBuild.config`
    - `CMakeBuild.config`

Last, execute the following command to enable multi target / configurations:

    cp $BLUENET_DIR/scripts/_targets_template.sh $BLUENET_CONFIG_DIR/_targets.sh

Now you can call the scripts above together with the target at the end of the call.

E.g. to build and upload target `BLUE` execute:

    ./firmware.sh --command=run --target=BLUE

and to build, upload and debug target `RED` call:

    ./firmware.sh --command=all --target=RED

The following scripts support multi targets:

- `firmware.sh`. Usage `./firmware.sh --command=<COMMAND> --target=<TARGET>`
- `softdevice.sh`. Usage `./softdevice.sh <COMMAND> <TARGET>`
- `hardware_version.sh`. Usage `./hardware_version.sh <TARGET>`
- 'all.sh'. Usage `./all.sh <TARGET>`

### Different Devices

To take the case of the different configurations a bit further, we can also assign the different configurations to different devices. E.g. if you are using 2 PCA10000 boards, each one has a different serial number. If you try to upload firmware to one of the devices, while having both plugged in, JLink will return with an error because it doesn't know to which device to upload the firmware. To solve this issue, follow the steps aboth to create two new configurations. (target `BLUE` and target `RED`)

After completing the steps above, open the file `$BLUENET_CONFIG_DIR/_targets.sh` and between the lines

    case "$target" in
    ...
    esac

add your new targets as

    BLUE)
      serial_num=<SERIAL NUMBER DEVICE1>
      ;;
    RED)
      serial_num=<SERIAL NUMBER DEVICE2>
      ;;

Note: It's also a good practice to define one of the devices as the default, which will be used if no target is supplied to the scripts, or if you want to add different configurations, without adding a new entry every time. So let's say the device with the configuration `RED` is the default, the code above will be adapted to

    BLUE)
      serial_num=<SERIAL NUMBER DEVICE1>
      ;;
    RED)
    *)
      serial_num=<SERIAL NUMBER DEVICE2>
      ;;

To obtain the serial number of the JLink devices, run

    JLinkExe

and then enter `ShowEmuList`, which gives a list of connected devices, e.g.

    JLink> ShowEmuList
    J-Link[0]: Connection: USB, Serial number: 680565191, ProductName: J-Link OB-SAM3U128-V2-NordicSem
    J-Link[1]: Connection: USB, Serial number: 518005793, ProductName: J-Link Lite Cortex-M-9
    JLink>

Now if you call one of the scripts, with target `BLUE`,  e.g. `./firmware.sh run BLUE` it will compile config `BLUE` at `$BLUENET_CONFIG_DIR/BLUE/CMakeBuild.config` and upload it to `DEVICE1`. While calling `./firmware.sh run RED` will compile config `RED` at `$BLUENET_CONFIG_DIR/RED/CMakeBuild.config` and upload it to `DEVICE2`.

And if you call `./firmware.sh run` it will compile the default config at `$BLUENET_CONFIG_DIR/CMakeBuild.config` and upload it to device `DEVICE2`.

## Unit tests

You can also run unit tests for host or the target board through:

    ./firmware.sh --command=unit-test-host --target=test
    ./firmware.sh --command=unit-test-nrf5 --target=test

Set `TEST_TARGET=`nrf5` in your `CMakeBuild.config` file in `test` to actually run the unit tests.

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
