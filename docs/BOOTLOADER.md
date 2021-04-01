# Bootloader

There are two possible bootloaders that can be used. The first one is the so-called "secure bootloader" and is part of the Nordic SDK. The other one is the "mesh bootloader" and is part of the Mesh SDK.

## Secure bootloader

The secure bootloader code in this repository is based on the `secure_bootloader/pca10040_ble` example in SDK 15.3.

You can find the information at the [Infocenter - Bootloader and DFU modules](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.3.0%2Flib_bootloader.html). For dual bank see [Infocenter - Bootloader and DFU modules - Device Firmware Update process](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.3.0%2Flib_bootloader_dfu_banks.html).

### Installation

The code for the bootloader is already there if you have downloaded the Nordic SDK. In the `CMakeList` file in the bootloader directory you see that the main file is set to `bootloader/main.c`. The `cs_Boards.h` file is included so the right pins for serial are used. Including the logging functionalities by Nordic increases the binary size of the bootloader a lot. For release, make sure debugging output over serial is disabled.

A prerequisite to building the bootloader is a compiled encryption library (`micro-eec`):

Set the compiler (again, sorry!):

    xdg-open ${NRF5_DIR}/components/toolchain/gcc/Makefile.posix

Build the lib:

    ${NRF5_DIR}/external/micro-ecc/build_all.sh

If the script is not executable, set its exec bit:

    chmod u+x ${NRF5_DIR}/external/micro-ecc/build_all.sh

If it contains windows line-endings, remove them ([stackoverflow will do](https://stackoverflow.com/questions/11680815/removing-windows-newlines-on-linux-sed-vs-awk)). Bug report filed [at Nordic](https://devzone.nordicsemi.com/f/nordic-q-a/47429/bug-windows-line-ending-characters).

Read more on the crypto backend at the [Infocenter](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.3.0/lib_crypto_backend_micro_ecc.html). 

To flash the bootloader to the device, use instructions from the [INSTALL](https://github.com/crownstone/bluenet/blob/master/docs/INSTALL.md) document.

    ./bluenet.sh -buB -t default

Note. If you set general breakpoints like `b main`, note that `gdb` will set the breakpoint both in the main of the bootloader and that of the firmware.

    ./bluenet.sh -dB -t default

It has no way to distinguish them. If you debug the firmware, `gdb` will start at the application address and breakpoints in the bootloader are skipped.

    ./bluenet.sh -dF -t default

To debug use the option `CS_SERIAL_NRF_LOG_ENABLED`. Set to `1` it will use `RTT`. Set to `2` it will use `UART`.

### Start address and bootloader size

The start address and size of the bootloader are defined in the linker script `secure_bootloader_gcc_nrf52.ld`. It is based on the script with the same name in `examples/dfu/secure_bootloader/pca10040_ble/armgcc`).

```
FLASH (rx) : ORIGIN = 0x76000, LENGTH = 0x8000
```

When the device boots, it will look at `MBR_BOOTLOADER_ADDR` or `UICR.BOOTLOADERADDR` (`UICR.NRFFW[1]`, or `0x10001014`) for the start address of the bootloader. The size of the bootloader is fixed for the lifetime of the device. This is because the location (`MBR_BOOTLOADER_ADDR`) that stores the start address of the bootloader is not (safely) updateable.

If you enable debugging, you will need more space for the bootloader. You will get an overflow error at link time if there has not been enough space reserved.

```
FLASH (rx) : ORIGIN = 0x71000, LENGTH = 0xd000
```

There are currently actually **two** locations where you have to define the start of the bootloader.

1. The [secure_bootloader_gcc_nrf52.ld](https://github.com/crownstone/bluenet/blob/master/bootloader/secure_bootloader_gcc_nrf52.ld) script in the `bootloader` directory (see above).
2. The `CMakeBuild.config` file for the target you are building for. The default comes from [CMakeBuild.config.default](https://github.com/crownstone/bluenet/blob/master/conf/cmake/CMakeBuild.config.default).

To allow for the additional space for debugging, starting at 0x71000 up to (almost!) the end of FLASH should be sufficient (adds up to `0x7E000`). Note, it's not up to `0x7FFFF`.

```
BOOTLOADER_START_ADDRESS=0x00071000
BOOTLOADER_LENGTH=0xD000
```

### Challenge

The devices in the field have an older bootloader. They need to be upgraded to the new bootloader which uses more space.

* A delicate upgrade process is required to update the current bootloader to the new bigger bootloader. This needs to be done in such a way that the chance of bricking a device in the field is neglectable. The process is stipulated at [this forum post](https://devzone.nordicsemi.com/f/nordic-q-a/18199/dfu---updating-from-legacy-sdk-v11-0-0-bootloader-to-secure-sdk-v12-x-0-bootloader).
* Diligently optimize for size and memory of the bootloader. For example, enable link time optimization and analyse other ways to decrease the footprint of the bootloader.
* Implement a secure setup procedure. Currently the setup is done by reducing the TX power such that someone eavesdropping needs to be physically close. However, there are software implementations that allow for 1) a secure key exchange to prevent eavesdropping and 2) authentication to prevent a MITM attack. This will improve the experience of the user, because there is no need to be physically close to the Crownstone to configure it (or reconfigure it after an update).

## Mesh bootloader

The mesh bootloader can be compiled by setting `BUILD_MESH_BOOTLOADER`. The documentation can be found in the [Infocenter - SDK - nRF5 SDK for Mesh 3.1.0 - Libraries - Mesh DFU Protocol](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.meshsdk.v3.1.0/md_doc_libraries_dfu_dfu_protocol.html) and subsections like [Configuring DFU over Mesh](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.meshsdk.v3.1.0/md_doc_libraries_dfu_dfu_quick_start.html). This uses the proprietary OpenMesh protocol under the hood (not the Bluetooth Mesh). Moreover, you'll need one of the devices hooked up over a serial connection. Only then an update can be propagated over the mesh.

### Challenge

This challenge should only be tackled when the DFU process directly from a phone is working perfectly.

* Implement device firmware updates over the mesh. This makes use of a hub that has a USB dongle ([link](https://shop.crownstone.rocks/products/crownstone-usb-dongle)). From this hub firmware updates can be propagated to nearby Crownstones. However, those Crownstones can further propagate their firmware updates if they will make use of the mesh. To implement this properly it might be necessary to modularize the Crownstone software itself.

This will require a hub and further in-depth studying of how to break up the software on the Crownstone. An analysis on how large the binaries are a very strong prerequisite.
