#include <drivers/cs_Uicr.h>
#include <protocol/cs_ErrorCodes.h>

static bool useNfcPinsAsGpio = false;
static cs_uicr_data_t staticUicrData = {
		.board = 0xFFFFFFFF,
		.productRegionFamily.asInt = 0xFFFFFFFF,
		.majorMinorPatch.asInt = 0xFFFFFFFF,
		.productionDateHousing.asInt = 0xFFFFFFFF,
};
static uint32_t board = 0xFFFFFFFF;

uint32_t getHardwareBoard() {
	return board;
}

cs_ret_code_t writeHardwareBoard() {
	return ERR_SUCCESS;
}

cs_ret_code_t enableNfcPinsAsGpio() {
	useNfcPinsAsGpio = true;
	return ERR_SUCCESS;
}

bool canUseNfcPinsAsGpio() {
	return useNfcPinsAsGpio;
}

cs_ret_code_t getUicr(cs_uicr_data_t* uicrData) {
	*uicrData = staticUicrData;
	return ERR_SUCCESS;
}

cs_ret_code_t setUicr(const cs_uicr_data_t* uicrData, bool overwrite) {
	staticUicrData = *uicrData;
	return ERR_SUCCESS;
}
