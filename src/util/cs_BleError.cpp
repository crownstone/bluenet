/**
 * Author: Crownstone Team
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <util/cs_BleError.h>
#include <drivers/cs_Serial.h>

//! Called by BluetoothLE.h classes when exceptions are disabled.
void ble_error_handler (const char * msg, uint32_t line_num, const char * p_file_name) {
	volatile const char* message __attribute__((unused)) = msg;
	volatile uint16_t line __attribute__((unused)) = line_num;
	volatile const char* file __attribute__((unused)) = p_file_name;

	LOGf("FATAL ERROR %s, at %s:%d", message, file, line);

	__asm("BKPT");
	while(1) {}
}

/**
 * Check the following website for the error codes. They change with every SoftDevice release, so check them 
 * regularly!
 *
 * https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.0.0/group___b_l_e___c_o_m_m_o_n.html
 */
void pretty_print(uint32_t error) {
	switch (error) {
		case NRF_ERROR_SVC_HANDLER_MISSING:
			LOGe("NRF_ERROR_SVC_HANDLER_MISSING");
			break;
		case NRF_ERROR_SOFTDEVICE_NOT_ENABLED:
			LOGe("NRF_ERROR_SOFTDEVICE_NOT_ENABLED");
			break;
		case NRF_ERROR_INTERNAL:
			LOGe("NRF_ERROR_INTERNAL");
			break;
		case NRF_ERROR_NO_MEM:
			LOGe("NRF_ERROR_NO_MEM");
			break;
		case NRF_ERROR_NOT_FOUND:
			LOGe("NRF_ERROR_NOT_FOUND");
			break;
		case NRF_ERROR_NOT_SUPPORTED:
			LOGe("NRF_ERROR_NOT_SUPPORTED");
			break;
		case NRF_ERROR_INVALID_PARAM:
			LOGe("NRF_ERROR_INVALID_PARAM");
			break;
		case NRF_ERROR_INVALID_STATE:
			LOGe("NRF_ERROR_INVALID_STATE");
			break;
		case NRF_ERROR_INVALID_LENGTH:
			LOGe("NRF_ERROR_INVALID_LENGTH");
			break;
		case NRF_ERROR_INVALID_FLAGS:
			LOGe("NRF_ERROR_INVALID_FLAGS");
			break;
		case NRF_ERROR_INVALID_DATA:
			LOGe("NRF_ERROR_INVALID_DATA");
			break;
		case NRF_ERROR_DATA_SIZE:
			LOGe("NRF_ERROR_DATA_SIZE");
			break;
		case NRF_ERROR_TIMEOUT:
			LOGe("NRF_ERROR_TIMEOUT");
			break;
		case NRF_ERROR_NULL:
			LOGe("NRF_ERROR_NULL");
			break;
		case NRF_ERROR_FORBIDDEN:
			LOGe("NRF_ERROR_FORBIDDEN");
			break;
		case NRF_ERROR_INVALID_ADDR:
			LOGe("NRF_ERROR_INVALID_ADDR");
			break;
		case NRF_ERROR_BUSY:
			LOGe("NRF_ERROR_BUSY");
			break;
		case NRF_ERROR_CONN_COUNT:
			LOGe("NRF_ERROR_CONN_COUNT");
			break;
		case NRF_ERROR_RESOURCES:
			LOGe("NRF_ERROR_RESOURCES");
			break;
		case NRF_ERROR_SDM_LFCLK_SOURCE_UNKNOWN:
			LOGe("NRF_ERROR_SDM_LFCLK_SOURCE_UNKNOWN");
			break;
		case NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION:
			LOGe("NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION");
			break;
		case NRF_ERROR_SDM_INCORRECT_CLENR0:
			LOGe("NRF_ERROR_SDM_INCORRECT_CLENR0");
			break;
		case NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN:
			LOGe("NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN");
			break;
		case NRF_ERROR_SOC_NVIC_INTERRUPT_NOT_AVAILABLE:
			LOGe("NRF_ERROR_SOC_NVIC_INTERRUPT_NOT_AVAILABLE");
			break;
		case NRF_ERROR_SOC_NVIC_INTERRUPT_PRIORITY_NOT_ALLOWED:
			LOGe("NRF_ERROR_SOC_NVIC_INTERRUPT_PRIORITY_NOT_ALLOWED");
			break;
		case NRF_ERROR_SOC_NVIC_SHOULD_NOT_RETURN:
			LOGe("NRF_ERROR_SOC_NVIC_SHOULD_NOT_RETURN");
			break;
		case NRF_ERROR_SOC_POWER_MODE_UNKNOWN:
			LOGe("NRF_ERROR_SOC_POWER_MODE_UNKNOWN");
			break;
		case NRF_ERROR_SOC_POWER_POF_THRESHOLD_UNKNOWN:
			LOGe("NRF_ERROR_SOC_POWER_POF_THRESHOLD_UNKNOWN");
			break;
		case NRF_ERROR_SOC_POWER_OFF_SHOULD_NOT_RETURN:
			LOGe("NRF_ERROR_SOC_POWER_OFF_SHOULD_NOT_RETURN");
			break;
		case NRF_ERROR_SOC_RAND_NOT_ENOUGH_VALUES:
			LOGe("NRF_ERROR_SOC_RAND_NOT_ENOUGH_VALUES");
			break;
		case NRF_ERROR_SOC_PPI_INVALID_CHANNEL:
			LOGe("NRF_ERROR_SOC_PPI_INVALID_CHANNEL");
			break;
		case NRF_ERROR_SOC_PPI_INVALID_GROUP:
			LOGe("NRF_ERROR_SOC_PPI_INVALID_GROUP");
			break;
		case BLE_ERROR_NOT_ENABLED: // 0x3001
			LOGe("BLE_ERROR_NOT_ENABLED");
			break;
		case BLE_ERROR_INVALID_CONN_HANDLE: // 0x3002
			LOGe("BLE_ERROR_INALID_CONN_HANDLE");
			break;
		case BLE_ERROR_INVALID_ATTR_HANDLE: // 0x3003
			LOGe("BLE_ERROR_INVALID_ATTR_HANDLE");
			break;
		case BLE_ERROR_INVALID_ADV_HANDLE: // 0x3004
			LOGe("BLE_ERROR_INVALID_ADV_HANDLE");
			break;
		case BLE_ERROR_INVALID_ROLE: // 0x3005
			LOGe("BLE_ERROR_INVALID_ROLE");
			break;
		case BLE_ERROR_BLOCKED_BY_OTHER_LINKS: // 0x3006
			LOGe("BLE_ERROR_BLOCKED_BY_OTHER_LINKS");
			break;
		default:
			LOGe("Unknown");
	}
}

void fds_pretty_print(ret_code_t ret_code) {
	switch(ret_code) {
		case FDS_SUCCESS:
			//LOGe("FDS_SUCCESS");
			break;
		case FDS_ERR_OPERATION_TIMEOUT:
			LOGe("FDS_ERR_OPERATION_TIMEOUT");
			break;
		case FDS_ERR_NOT_INITIALIZED:
			LOGe("FDS_ERR_NOT_INITIALIZED");
			break;
		case FDS_ERR_UNALIGNED_ADDR:
			LOGe("FDS_ERR_UNALIGNED_ADDR");
			break;
		case FDS_ERR_INVALID_ARG:
			LOGe("FDS_ERR_INVALID_ARG");
			break;
		case FDS_ERR_NULL_ARG:
			LOGe("FDS_ERR_NULL_ARG");
			break;
		case FDS_ERR_NO_OPEN_RECORDS:
			LOGe("FDS_ERR_NO_OPEN_RECORDS");
			break;
		case FDS_ERR_NO_SPACE_IN_FLASH:
			LOGe("FDS_ERR_NO_SPACE_IN_FLASH");
			break;
		case FDS_ERR_NO_SPACE_IN_QUEUES:
			LOGe("FDS_ERR_NO_SPACE_IN_QUEUES");
			break;
		case FDS_ERR_RECORD_TOO_LARGE:
			LOGe("FDS_ERR_RECORD_TOO_LARGE");
			break;
		case FDS_ERR_NOT_FOUND:
			LOGe("FDS_ERR_NOT_FOUND");
			break;
		case FDS_ERR_NO_PAGES:
			LOGe("FDS_ERR_NO_PAGES");
			break;
		case FDS_ERR_USER_LIMIT_REACHED:
			LOGe("FDS_ERR_USER_LIMIT_REACHED");
			break;
		case FDS_ERR_CRC_CHECK_FAILED:
			LOGe("FDS_ERR_CRC_CHECK_FAILED");
			break;
		case FDS_ERR_BUSY:
			LOGe("FDS_ERR_BUSY");
			break;
		case FDS_ERR_INTERNAL:
			LOGe("FDS_ERR_INTERNAL");
			break;
		default:
			LOGe("Unknown");
	}
}

//! called by soft device when you pass bad parameters, etc.
void app_error_handler (uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
	volatile uint32_t error __attribute__((unused)) = error_code;
	volatile uint16_t line __attribute__((unused)) = line_num;
	volatile const uint8_t* file __attribute__((unused)) = p_file_name;

	pretty_print(error);
	LOGf("FATAL ERROR 0x%x, at %s:%d", error, file, line);

	__asm("BKPT");
	while(1) {}
}

//! Called by softdevice
void app_error_handler_bare(ret_code_t error_code)
{
	volatile uint32_t error __attribute__((unused)) = error_code;
	volatile uint16_t line __attribute__((unused)) = 0;
	volatile const uint8_t* file __attribute__((unused)) = NULL;

	pretty_print(error);
	LOGf("FATAL ERROR 0x%x", error);

	__asm("BKPT");
	while(1) {}
}

//called by NRF SDK when it has an internal error.
void assert_nrf_callback (uint16_t line_num, const uint8_t *file_name) {
	volatile uint16_t line __attribute__((unused)) = line_num;
	volatile const uint8_t* file __attribute__((unused)) = file_name;

	LOGf("FATAL ERROR at %s:%d", file, line);

	__asm("BKPT");
	while(1) {}
}

/*

//called by soft device when it has an internal error.
void softdevice_assertion_handler(uint32_t pc, uint16_t line_num, const uint8_t * file_name){
	volatile uint16_t line __attribute__((unused)) = line_num;
	volatile const uint8_t* file __attribute__((unused)) = file_name;
	__asm("BKPT");
	while(1) {}
}

*/
