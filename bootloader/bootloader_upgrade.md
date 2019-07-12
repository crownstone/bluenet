# Upgrading the existing Bootloader to a secure Bootloader

### Problem Statement

### High-level Plan

A new bootloader has been created based on the legacy bootloader. This retains all the functionality present previously with an additional update to pick-up the bootloader upgradation. This intermediate bootloader can be written using the existing BL-DFU method.

In the intermediate bootloader, along with the existing bootloader commands, one additional command would be introduced which can handle the bootloader extension. The process is detailed below:

* Once the UPGRADE_BL (a new command) is picked up by the DFU, it runs the normal BL-update routine of copying the incoming new BL into the "free-space" (process part of Dual-bank update).

* With the Upgraded Bootloader in the free-space (at an address lower than the previous NEW_BL_ADDRESS), the APP_DATA is moved to a new location, moving into the unoccupied free-space.

* UICR.NRFFW[0] register is updated with the new upgraded bootloader address (NEW_BL_ADDRESS).

* The new bootloader present in the free-space is copied to the NEW_BL_ADDRESS.

* A reset is performed.