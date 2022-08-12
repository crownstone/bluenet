/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 15, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Values in GPREGRET used to remember things after a reboot.
 *
 * We don't use the NRF_BL_DFU_ENTER_METHOD_GPREGRET method, instead we use our own implementation.
 *
 * However, we do use NRF_BL_DFU_ENTER_METHOD_GPREGRET in the bootloader once it decided to go into DFU mode,
 * to avoid modifications to nrf_bootloader.c.
 * This means that if GPREGRET == 1011 0001 (counter=17, brownout=true, storage_recovered=true), the bootloader will
 * also go into DFU mode.
 *
 * We use 5 bits of GPREGRET as counter. Each boot, the bootloader increases the count by 1.
 * Once the counter is at its max, the bootloader will go into DFU mode.
 * A minute or so after boot, the app will set GPREGRET to 0.
 * This means that if the app crashes quickly after boot, it will end up in DFU mode.
 *
 * In case the bootloader is updated, but the app not:
 * The old app will set GPREGRET to CS_GPREGRET_LEGACY_DFU_RESET when it wants to got to DFU mode.
 * This case should be handled by the bootloader for now.
 *
 * The remaining bits are used as flags.
 */
#define CS_GPREGRET_COUNTER_MASK 31                       // Bits used to count reboots.
#define CS_GPREGRET_COUNTER_MAX CS_GPREGRET_COUNTER_MASK  // Max value of the counter.
#define CS_GPREGRET_COUNTER_SOFT_RESET 1        // Value to set the counter to when we want to reboot normally.
#define CS_GPREGRET_LEGACY_DFU_RESET 66         // Value that used to be the value of the counter to enter dfu mode.
#define CS_GPREGRET_FLAG_BROWNOUT 32            // Bit used to indicate a brownout happened.
#define CS_GPREGRET_FLAG_DFU_RESET 64           // Bit used to indicate we want to go to dfu mode.
#define CS_GPREGRET_FLAG_STORAGE_RECOVERED 128  // Bit used to indicate storage recovered.

/**
 * Values in GPREGRET2 used to remember things after a reboot.
 *
 * Make sure this doesn't interfere with the nrf bootloader values that are used. Like:
 * - BOOTLOADER_DFU_GPREGRET2_MASK
 * - BOOTLOADER_DFU_GPREGRET2
 * - BOOTLOADER_DFU_SKIP_CRC_BIT_MASK
 *
 * Currently the nrf bootloader uses:
 * - bit 3-7 are used as "magic pattern" to recognize the dfu communication.
 * - bit 0 is used for "skip crc"
 *
 * We use:
 * -
 */
