/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 17, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cs_SharedConfig.h>
#include <drivers/cs_Uicr.h>
#include <nrf_nvmc.h>
#include <nrf_sdh.h>
#include <protocol/cs_ErrorCodes.h>
#include <stdbool.h>
#include <stdint.h>

bool _canEditUicr() {
	return nrf_sdh_is_enabled() == false;
}

uint32_t getHardwareBoard() {
	uint32_t hardwareBoard = *((uint32_t*)g_HARDWARE_BOARD_ADDRESS);
	if (hardwareBoard == 0xFFFFFFFF) {
		hardwareBoard = g_DEFAULT_HARDWARE_BOARD;
	}
	return hardwareBoard;
}

cs_ret_code_t writeHardwareBoard() {
	if (!_canEditUicr()) {
		return ERR_WRONG_STATE;
	}
	uint32_t hardwareBoard = *((uint32_t*)g_HARDWARE_BOARD_ADDRESS);
	if (hardwareBoard == 0xFFFFFFFF) {
		nrf_nvmc_write_word(g_HARDWARE_BOARD_ADDRESS, g_DEFAULT_HARDWARE_BOARD);
	}
	return ERR_SUCCESS;
}

cs_ret_code_t enableNfcPinsAsGpio() {
	if (!_canEditUicr()) {
		return ERR_WRONG_STATE;
	}
	if (NRF_UICR->NFCPINS & 1) {
		nrf_nvmc_write_word((uint32_t) & (NRF_UICR->NFCPINS), 0xFFFFFFFE);
	}
	return ERR_SUCCESS;
}

bool canUseNfcPinsAsGpio() {
	return (NRF_UICR->NFCPINS & 1) == 0;
}

/**
 * Checks whether a value can be written to UICR, but does not check if UICR is writable at all.
 *
 * If not, you will need to clear the whole UICR first.
 */
bool _canSetUicrField(uint32_t value, uint32_t address) {
	uint32_t currentValue = *((uint32_t*)address);

	// With a write, You can only turn a bit from 1 into a 0.
	// The only way to turn a 0 into a 1, is to erase the whole UICR.
	if ((currentValue & value) == value) {
		return true;
	}
	return false;
}

/**
 * Writes value to UICR if not already done so.
 *
 * Make sure to first check whether this value can be written.
 */
void _setUicrField(uint32_t value, uint32_t address) {
	// Avoid unnecessary writes, you can only write UICR a few times before you have to erase the UICR.
	// So only write to UICR if it's not already written.
	if (*((uint32_t*)address) != value) {
		nrf_nvmc_write_word(address, value);
	}
}

cs_ret_code_t _clearUicr() {
	if (!_canEditUicr()) {
		return ERR_WRONG_STATE;
	}

	// In order to clear the UICR, we can only clear the whole UICR, including fields used by nordic.
	// So we have to first copy the nordic contents to RAM, clear UICR, and copy back.

	// Based on the following post, but using nrf_nvmc functions where we can.
	// https://devzone.nordicsemi.com/f/nordic-q-a/18199/dfu---updating-from-legacy-sdk-v11-0-0-bootloader-to-secure-sdk-v12-x-0-bootloader

	// First block, contains all UICR->NRFFW[] and UICR->NRFHW.
	const uint32_t startAddress = 0x10001014;
	const uint32_t endAddress   = 0x10001080;
	const int bufSize           = (endAddress - startAddress) / sizeof(uint32_t);
	uint32_t buffer[bufSize];

	// Second block, contains all UICR fields after the UICR->CUSTOMER[].
	const uint32_t startAddress2 = 0x10001200;
	const uint32_t endAddress2   = 0x10001210;
	const int bufSize2           = (endAddress2 - startAddress2) / sizeof(uint32_t);
	uint32_t buffer2[bufSize];

	CRITICAL_REGION_ENTER();

	// Copy UICR to RAM.
	uint32_t* address = (uint32_t*)startAddress;
	for (int i = 0; i < bufSize; ++i) {
		buffer[i] = *address;
		address++;
	}

	address = (uint32_t*)startAddress2;
	for (int i = 0; i < bufSize2; ++i) {
		buffer2[i] = *address;
		address++;
	}

	// Enable erase.
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
	__ISB();
	__DSB();

	// Erase the page
	NRF_NVMC->ERASEUICR = NVMC_ERASEUICR_ERASEUICR_Erase << NVMC_ERASEUICR_ERASEUICR_Pos;
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
	}

	// Unlike nrf_nvmc_page_erase() we don't have to enable read here,
	// as the next thing we do is go into write mode.

	// Write the cached UICR contents back to the UICR.
	nrf_nvmc_write_words(startAddress, buffer, bufSize);
	nrf_nvmc_write_words(startAddress2, buffer2, bufSize2);

	CRITICAL_REGION_EXIT();

	return ERR_SUCCESS;
}

cs_ret_code_t getUicr(cs_uicr_data_t* uicrData) {
	uicrData->board                       = *((uint32_t*)g_HARDWARE_BOARD_ADDRESS);
	uicrData->productRegionFamily.asInt   = *((uint32_t*)g_FAMILY_MARKET_TYPE_ADDRESS);
	uicrData->majorMinorPatch.asInt       = *((uint32_t*)g_MAJOR_MINOR_PATCH_ADDRESS);
	uicrData->productionDateHousing.asInt = *((uint32_t*)g_PROD_DATE_HOUSING_ADDRESS);
	return ERR_SUCCESS;
}

cs_ret_code_t setUicr(const cs_uicr_data_t* uicrData, bool overwrite) {
	// First check if every field can be written to UICR, we don't want a partial write.
	if (!_canSetUicrField(uicrData->board, g_HARDWARE_BOARD_ADDRESS)
		|| !_canSetUicrField(uicrData->productRegionFamily.asInt, g_FAMILY_MARKET_TYPE_ADDRESS)
		|| !_canSetUicrField(uicrData->majorMinorPatch.asInt, g_MAJOR_MINOR_PATCH_ADDRESS)
		|| !_canSetUicrField(uicrData->productionDateHousing.asInt, g_PROD_DATE_HOUSING_ADDRESS)) {
		if (!overwrite) {
			return ERR_ALREADY_EXISTS;
		}
		if (!_canEditUicr()) {
			return ERR_WRONG_STATE;
		}
		cs_ret_code_t retCode = _clearUicr();
		if (retCode != ERR_SUCCESS) {
			return retCode;
		}
	}

	// Write all fields.
	_setUicrField(uicrData->board, g_HARDWARE_BOARD_ADDRESS);
	_setUicrField(uicrData->productRegionFamily.asInt, g_FAMILY_MARKET_TYPE_ADDRESS);
	_setUicrField(uicrData->majorMinorPatch.asInt, g_MAJOR_MINOR_PATCH_ADDRESS);
	_setUicrField(uicrData->productionDateHousing.asInt, g_PROD_DATE_HOUSING_ADDRESS);
	return ERR_SUCCESS;
}
