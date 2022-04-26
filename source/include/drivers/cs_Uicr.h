/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 17, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <protocol/cs_UicrPacket.h>
#include <protocol/cs_Typedefs.h>

/**
 * Get the hardware board from UICR, or the default board if none is set in UICR.
 */
uint32_t getHardwareBoard();

/**
 * Write the default hardware board to UICR if none is written yet..
 *
 * In the past, the hardware board was not written to UICR. Instead, the board was compiled into the firmware,
 * and each product had their own different firmware.
 * This function is made to transition from that solution to a single firmware with board in UICR.
 * We only have to make sure that each product once runs a firmware that writes the correct hardware board to UICR..
 */
void writeHardwareBoard();

/**
 * Enable NFC pins to be used as GPIO.
 *
 * This is stored in UICR, so it's persistent.
 * NFC pins leak a bit of current when not at same voltage level.
 */
void enableNfcPinsAsGpio();

/**
 * Returns true when the NFC pins can be used as GPIO.
 */
bool canUseNfcPinsAsGpio();


/**
 * Read the UICR data from UICR.
 *
 * @param[out] uicrData                The struct to read to.
 *
 * @return     ERR_SUCCESS             On success.
 */
cs_ret_code_t getUicr(cs_uicr_data_t* uicrData);

/**
 * Write the UICR data to UICR.
 *
 * @param[in]  uicrData                The data to write to UICR.
 * @param[in]  overwrite               Whether to overwrite existing data in UICR.
 *                                     This is a risky operation, as it erases the whole UICR for a short while.
 *
 * @return     ERR_SUCCESS             On success.
 * @return     ERR_ALREADY_EXISTS      When something is written already. You will have to overwrite.
 */
cs_ret_code_t setUicr(const cs_uicr_data_t* uicrData, bool overwrite);

#ifdef __cplusplus
}
#endif
