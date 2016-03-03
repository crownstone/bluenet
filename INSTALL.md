# Installation

The installation has been tested on Ubuntu 14.04 and assumes you use the J-Link programmer.

## Prerequisites

The installation should not be hard when you have the Nordic SDK. Get this from their website after buying a development kit. You also need a cross-compiler for ARM. You need the JLink utilities from Segger. And you need cmake for the build process.

Download Nordic's SDK and unzip:

* [Nordic nRF51822 SDK 8.1](https://developer.nordicsemi.com/nRF5_SDK/nRF51_SDK_v8.x.x/)
* [Nordic S110 Softdevice 8.0](http://www.nordicsemi.com/eng/nordic/Products/nRF51822/S110-SD-v8/45846) or [Nordic S1130 Softdevice 1.0](https://www.nordicsemi.com/eng/nordic/Products/nRF51822/S130-SD/46579)

Download and install J-Link Segger’s [software](http://www.segger.com/jlink-software.html) (4.96.2 works, some other version had issues at the time):

* Download the [.deb file 64bit version 4.96.2](https://www.segger.com/jlink-software.html?step=1&file=JLinkLinuxDEB64_4.96.2)
* Install it:

    sudo dpkg -i jlink_4.96.2_x86_64.deb

A cross-compiler for ARM is the `GCC` cross-compiler which is maintained by the ARM folks on [Launchpad](https://launchpad.net/gcc-arm-embedded).

* Download and extract version [2014-q3](https://launchpad.net/gcc-arm-embedded/4.8/4.8-2014-q3-update/+download/gcc-arm-none-eabi-4_8-2014q3-20140805-linux.tar.bz2).

* Install some 32bit packages, assuming you have a 64bit systems:

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install libstdc++6:i386 libncurses5:i386

* If the cross-compiler does not work, make sure you check if all its dependencies are met:

    ldd /opt/gcc-arm-none-eabi-4_8-2014q3/bin/arm-none-eabi-gcc

Bluenet uses a `cmake` build system, so you will need it:

    sudo apt-get install cmake

### Nordic SDK Bugs

There is a bug in Nordics SDK code, search for files named `nrf_svc.h`.
In those files, replace the assembly line:

    "bx r14" : : "I" (number) : "r0" \

With:

    "bx r14" : : "I" ((uint16_t)number) : "r0" \

### Getting the Bluenet code

The best way is to first [fork](https://github.com/dobots/bluenet/fork) the bluenet repository.
Then download the code:

    git clone https://github.com/YOUR_GITHUB_USERNAME/bluenet
    
Set the correct upsteam and set environment variables:

    cd bluenet
    git remote add upstream git@github.com:dobots/bluenet.git
    echo "export BLUENET_DIR=$PWD" >> ~/.bashrc

Make a new dir for your config file(s), for example `~/bluenet-config`

    mkdir ~/bluenet-config
    echo "export BLUENET_CONFIG_DIR=~/bluenet-config" >> ~/.bashrc

Apply the environmental variables:

    source ~/.bashrc
    

### Configuration

Start writing your personal config file:

    gedit $BLUENET_CONFIG_DIR/CMakeBuild.config &

Copy the lines that you want to adjust from the default configuration: `$BLUENET_DIR/CMakeBuild.config.default`.

Installation dependent configurations:

* Set `COMPILER_PATH` to the path where the compiler can be found (it should contain the `/bin` subdir).
* Set `COMPILER_TYPE` to the correct compiler type
* Set `JLINK` to the correct JLink Exe file.
* Set `JLINK_GDB_SERVER` to the correct file.
* Set `NRF51822_DIR` to wherever you installed the Nordic SDK (it should contain the `/Include` and `/Source` subdirs).

Softdevice dependent configurations:

* Set `SOFTDEVICE_DIR` to wherever you unzipped the SoftDevice dir from Nordic (it should contain the .hex file).
* Set `SOFTDEVICE_DIR_API` to the directory with the SoftDevice include dir. This is relative, `${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}/` will be the used path.
* Set `SOFTDEVICE accordingly` (basename of file without `_softdevice.hex`)
* Set `SOFTDEVICE_SERIES`, `SOFTDEVICE_MAJOR`, and `SOFTDEVICE_MINOR` accordingly.

Device dependent configuration:

* Set `BLUETOOTH_NAME` to something you like, but make sure it's short (<10).
* Set `INDOOR_SERVICE` to `1` if you want to enable it, the same is true for the other services. The indoor service only works for the s130.
* Set `CHAR_SAMPLE_CURRENT` to `1` if you want to enable the sample current characteristics, the same is true for other characteristics.
* Set `CHAR_MESHING` to `1` if you want to enable meshing functionality.
* You can adjust `SERIAL_VERBOSITY` if you want to see more or less output over the UART
* Set `HARDWARE_BOARD` to the correct number for your board. This determines the pin layout.
* Set `HARDWARE_VERSION` to the correct version of the NRF51 chip you have. Use `bluenet/scripts/hardware_version.sh` to check your version.
* Set `IBEACON` to `1`, if you want to advertise like an iBeacon. You will want to set the correct values for the `BEACON_*` configs as well.
* You can adjust the `MASTER_BUFFER_SIZE` when needed, this buffer is used on several different places.
* If you run into memory issues, you can play around with `HEAP_SIZE`. Increasing the heap size (dynamic memory), will reduce the stack size (static memory).


## Usage

Once everything is installed and configured, the code can be compiled and uploaded.
You will have to attach a programmer/debugger, like the JLink. Towards that you only need four pins: `GND`, `3V`, `SWDIO / RESET`, and `SWCLK / FACTORY`. The pin layout of the JLink connector is written out on the [DoBots blog](http://dobots.nl/2014/03/05/rfduino-without-rfduino-code/).

### Compiling, uploading and debugging

First build and upload the SoftDevice:

    cd $BLUENET_DIR/scripts
    ./softdevice.sh build
    ./softdevice.sh upload

Now we can build our own software:

    cd $BLUENET_DIR/scripts
    ./firmware.sh build

If this fails due to misconfiguration, you might have to remove the `$BLUENET_DIR/build` dir.

To upload the application with the JLink you can use:

    ./firmware.sh upload
    
To debug with `gdb` you can use:

    ./firmware.sh debug

There is a shortcut to build and upload you can use:

    ./firmware.sh run

And another shortcut to build, upload, and debug:

    ./firmware.sh all


## Flashing with the ST-Link

The above assumes you have the J-Link programmer from Nordic. If you do not have that device, you can still program something like the RFduino or the Crownstone, by using an ST-Link. A full explanation can be found on <https://dobots.nl/2015/01/23/programming-the-nrf51822-with-the-st-link/>.

Disclaimer: no work has been done on the ST-Link scripts for quite some time, so things might not work anymore.

### Combine softdevice and firmware

First of all you should combine all the required binaries into one big binary. This is done by the script combine.sh. Before you use it, you will need to install srec_cat on your system.

    sudo apt-get install srecord

If you call the script it basically just runs srec_cat:

    ./combine.sh

And you will see that it runs something like this:

    srec_cat /opt/softdevices/s110_nrf51822_7.0.0_softdevice.hex -intel crownstone.bin -binary -offset 0x00016000 -o combined.hex -intel

You have to adjust that file on the moment manually to switch between softdevices or to add/remove the bootloader, sorry! Note that the result is a `.hex` file. Such a file does haveinformation across multiple memory sections. If you upload a `.bin` file often configuration bits/bytes will not be set!

### Upload with OpenOCD

Rather than downloading `openocd` from the Ubuntu repositories, it is recommended to get the newest software from the source:

    cd /opt
    git clone https://github.com/ntfreak/openocd
    sudo aptitude install libtool automake libusb-1.0-0-dev expect
    cd openocd
    ./bootstrap
    ./configure --enable-stlink
    make
    sudo make install

Also, make sure you can use the USB ST-Link device without sudo rights:

    sudo cp scripts/openocd/49-stlinkv2.rules /etc/udev/rules.d
    sudo restart udev

You can now use the `flash_openocd.sh` script in the `scripts` directory:

    ./flash_openocd.sh init

And in another console:

    ./flash_openocd.sh upload combined.bin

Here the binary `combined.bin` is the softdevice and application combined.

## Meshing

The mesh code is relatively memory intensive, so you will need a 32kB version to run it. Set `CHAR_MESHING` to `1` if you want to use it.

For the meshing functionality we use https://github.com/NordicSemiconductor/nRF51-ble-bcast-mesh written by Trond Einar Snekvik, department of Engineering Cybernetics at Norwegian University of Science and Technology (and Nordic Semiconductors).

## UART

Currently UART is used for debugging. The TX pin is defined by the board type you configured (`HARDWARE_BOARD`), for each board type the TX pin nr can be found in the `cs_Boards.h` file.

In case you happen to have the nRFgo Motherboard (nRF6310, strongly recommended) you can easily connect the pins at P2.0 and P2.1 to respectively the pins RXD and TXD on P15 on the board. Do not forget to switch on the RS232 switch. Subsequently you will need some RS232 to USB cable if you haven't an extremely old laptop.

The current set baudrate you can find in `cs_Serial.cpp` and is `38400` baud. To read from serial you can for example use `minicom` (replace ttyUSB0 with the correct device):

    minicom -b 38400 -c on -D /dev/ttyUSB0

If this requires sudo rights, you may want to add yourself to the dialout group:

    sudo adduser $USER dialout

You will have to logout and login for it to have effect.

If all goes well, you should see text appearing whenever the crownstone is rebooted/reset. This also shows you which services and characteristics are enabled.
