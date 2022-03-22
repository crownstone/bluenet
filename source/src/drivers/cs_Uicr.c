/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 17, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <nrf52.h>
#include <protocol/cs_ErrorCodes.h>



cs_ret_code_t getUicr(cs_uicr_data_t* uicrData) {
	uicrData->board                       = NRF_UICR->CUSTOMER[UICR_BOARD_INDEX];
	uicrData->productRegionFamily.asInt   = NRF_UICR->CUSTOMER[UICR_INDEX_FAMILY_MARKET_TYPE];
	uicrData->majorMinorPatch.asInt       = NRF_UICR->CUSTOMER[UICR_INDEX_MAJOR_MINOR_PATCH];
	uicrData->productionDateHousing.asInt = NRF_UICR->CUSTOMER[UICR_INDEX_PROD_DATE_HOUSING];
	return ERR_SUCCESS;
}

cs_ret_code_t setUicr(const cs_uicr_data_t* uicrData) {
	nrf_nvmc_write_word(g_HARDWARE_BOARD_ADDRESS, uicrData->board);
	nrf_nvmc_write_word(g_UICR_ADDRESS_FAMILY_MARKET_TYPE, uicrData->productRegionFamily.asInt);
	nrf_nvmc_write_word(g_UICR_ADDRESS_MAJOR_MINOR_PATCH, uicrData->majorMinorPatch.asInt);
	nrf_nvmc_write_word(g_UICR_ADDRESS_PROD_DATE_HOUSING, uicrData->productionDateHousing.asInt);

	if (NRF_UICR->CUSTOMER[UICR_BOARD_INDEX] = 0xFFFFFFFF) {

	}
}

