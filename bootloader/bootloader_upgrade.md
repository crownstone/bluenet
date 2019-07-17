# Upgrading the existing Bootloader to a secure Bootloader

### Problem Statement

### High-level Plan

A new intermediate bootloader has to be created based on the legacy bootloader. This should retain all the functionality present previously with an additional update to pick-up the bootloader upgrade command. In principle, this intermediate bootloader can be written using the existing BL-DFU method as this doesn't increase the size of the bootloader by a significant amount.

In the intermediate bootloader, along with the existing bootloader commands, one additional command would be introduced which can handle the bootloader extension/upgrade. The process is detailed below:

* Once the UPGRADE_BL (a new command) is picked up by the DFU, it runs the normal BL-update routine of copying the incoming new BL into the "free-space" (process part of Dual-bank update).

* With the Upgraded Bootloader in the free-space (at an address lower than the previous NEW_BL_ADDRESS), the APP_DATA is moved to a new location, moving into the unoccupied free-space.

* UICR.NRFFW[0] register is updated with the new upgraded bootloader address (NEW_BL_ADDRESS).

* The new bootloader present in the free-space is copied to the NEW_BL_ADDRESS.

* A reset is performed.

#### Challenge
The current implementation done by Nordic is to use dual bank mode to obtain the bootloader into the flash first (into bank-1). Once, the bootloader verifies its presence and validity, it is copied using MBR commands, which doesn't allow the change in the destination/target address. Making the bootloader's address fixed.

* **Solution 1:** ~~Not sure if this works~~. This method should work according the one of the post in the devzone. This solution is based on the assumption that the MBR_BL_COPY command uses the UICR.NRFFW[0] value as the destination address. If this is true, the copy would place the bootloader already in the right place. This is the method which is being used currently.

* **Solution 2:** The bootloader after receiving the new bootloader into bank-1, it should hand-off the control to firmware, but before it does so, it writes into a persistent register a unique trigger value. The firmware after reading this trigger value, copies the image of bootloader which is present in bank-1 to the new location safely and later resetting itself. This seems like a most reliable solution.

* Solution 3: Use the image copy function provided by the nRF SDK to copy the BL image from Bank-1 to the new Bootloader location. As long as the image copy function is part of Soft-Device, it's perfectly fine. Otherwise, we are shooting in our foot. **Update** image copy function belongs to the bootloader. So this solution is *infeasible*.

#### Additional updates
It has been verified that Bank-0 (Application) is about 155 kB long. Which is about half the size of the total available application area. (328 kB). Therefore, the new secure bootloader can safely fit into the bank-1 size.