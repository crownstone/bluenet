# Bootloader

The bootloader is based on the SDK 15.3 example secure_bootloader/pca10040_ble.

See: Software Development Kit > nRF5 SDK v15.3.0 > Libraries > Bootloader and DFU modules (https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.3.0%2Flib_bootloader.html)
Dual bank, see: Software Development Kit > nRF5 SDK v15.3.0 > Libraries > Bootloader and DFU modules > Device Firmware Update process (https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.3.0%2Flib_bootloader_dfu_banks.html)

## Start address and bootloader size

The start address and size of the bootloader are defined in the linker script (secure_bootloader_gcc_nrf52.ld based on SDK_15_3/examples/dfu/secure_bootloader/pca10040_ble/armgcc/secure_bootloader_gcc_nrf52.ld).
```
FLASH (rx) : ORIGIN = 0x78000, LENGTH = 0x6000
```

When the device boots, it will look at MBR_BOOTLOADER_ADDR or UICR.BOOTLOADERADDR (UICR.NRFFW[1], or 0x10001014) for the start address of the bootloader.
The size of the bootloader is fixed for the lifetime of the device. This is because the location (MBR_BOOTLOADER_ADDR) that stores the start address of the bootloader is not (safely) updateable.


