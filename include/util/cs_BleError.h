/**
 * Author: Crownstone Team
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <drivers/cs_Serial.h>
#include <util/cs_Error.h>

#ifdef __cplusplus
extern "C"
{
#endif


//! Called by BluetoothLE.h classes when exceptions are disabled.
void ble_error_handler (const char * msg, uint32_t line_num, const char * p_file_name);

//! called by soft device when you pass bad parameters, etc.
//void app_error_handler (uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name);

//called by NRF SDK when it has an internal error.
void assert_nrf_callback (uint16_t line_num, const uint8_t *file_name);

//called by soft device when it has an internal error.
void softdevice_assertion_handler(uint32_t pc, uint16_t line_num, const uint8_t * file_name);

/**
 * Check the following website for the error codes. They change with every SoftDevice release, so check them 
 * regularly!
 *
 * https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.0.0/group___b_l_e___c_o_m_m_o_n.html
 */
constexpr const char* NordicTypeName(uint32_t nordic_type) {
	switch (nordic_type) {
		case NRF_ERROR_SVC_HANDLER_MISSING: // 0x0001
			return "NRF_ERROR_SVC_HANDLER_MISSING";
		case NRF_ERROR_SOFTDEVICE_NOT_ENABLED: // 0x0002
			return "NRF_ERROR_SOFTDEVICE_NOT_ENABLED";
		case NRF_ERROR_INTERNAL: // 0x0003
			return "NRF_ERROR_INTERNAL";
		case NRF_ERROR_NO_MEM: // 0x0004
			return "NRF_ERROR_NO_MEM";
		case NRF_ERROR_NOT_FOUND: // 0x0005
			return "NRF_ERROR_NOT_FOUND";
		case NRF_ERROR_NOT_SUPPORTED: // 0x0006
			return "NRF_ERROR_NOT_SUPPORTED";
		case NRF_ERROR_INVALID_PARAM: // 0x0007
			return "NRF_ERROR_INVALID_PARAM";
		case NRF_ERROR_INVALID_STATE: // 0x0008
			return "NRF_ERROR_INVALID_STATE";
		case NRF_ERROR_INVALID_LENGTH: // 0x0009
			return "NRF_ERROR_INVALID_LENGTH";
		case NRF_ERROR_INVALID_FLAGS: // 0x0010
			return "NRF_ERROR_INVALID_FLAGS";
		case NRF_ERROR_INVALID_DATA: // 0x0011
			return "NRF_ERROR_INVALID_DATA";
		case NRF_ERROR_DATA_SIZE: // 0x0012
			return "NRF_ERROR_DATA_SIZE";
		case NRF_ERROR_TIMEOUT: // 0x0013
			return "NRF_ERROR_TIMEOUT";
		case NRF_ERROR_NULL: // 0x0014
			return "NRF_ERROR_NULL";
		case NRF_ERROR_FORBIDDEN: // 0x0015
			return "NRF_ERROR_FORBIDDEN";
		case NRF_ERROR_INVALID_ADDR: // 0x0016
			return "NRF_ERROR_INVALID_ADDR";
		case NRF_ERROR_BUSY: // 0x0017
			return "NRF_ERROR_BUSY";
		case NRF_ERROR_CONN_COUNT:
			return "NRF_ERROR_CONN_COUNT";
		case NRF_ERROR_RESOURCES:
			return "NRF_ERROR_RESOURCES";
		case NRF_ERROR_SDM_LFCLK_SOURCE_UNKNOWN:
			return "NRF_ERROR_SDM_LFCLK_SOURCE_UNKNOWN";
		case NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION:
			return "NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION";
		case NRF_ERROR_SDM_INCORRECT_CLENR0:
			return "NRF_ERROR_SDM_INCORRECT_CLENR0";
		case NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN:
			return "NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN";
		case NRF_ERROR_SOC_NVIC_INTERRUPT_NOT_AVAILABLE:
			return "NRF_ERROR_SOC_NVIC_INTERRUPT_NOT_AVAILABLE";
		case NRF_ERROR_SOC_NVIC_INTERRUPT_PRIORITY_NOT_ALLOWED:
			return "NRF_ERROR_SOC_NVIC_INTERRUPT_PRIORITY_NOT_ALLOWED";
		case NRF_ERROR_SOC_NVIC_SHOULD_NOT_RETURN:
			return "NRF_ERROR_SOC_NVIC_SHOULD_NOT_RETURN";
		case NRF_ERROR_SOC_POWER_MODE_UNKNOWN:
			return "NRF_ERROR_SOC_POWER_MODE_UNKNOWN";
		case NRF_ERROR_SOC_POWER_POF_THRESHOLD_UNKNOWN:
			return "NRF_ERROR_SOC_POWER_POF_THRESHOLD_UNKNOWN";
		case NRF_ERROR_SOC_POWER_OFF_SHOULD_NOT_RETURN:
			return "NRF_ERROR_SOC_POWER_OFF_SHOULD_NOT_RETURN";
		case NRF_ERROR_SOC_RAND_NOT_ENOUGH_VALUES:
			return "NRF_ERROR_SOC_RAND_NOT_ENOUGH_VALUES";
		case NRF_ERROR_SOC_PPI_INVALID_CHANNEL:
			return "NRF_ERROR_SOC_PPI_INVALID_CHANNEL";
		case NRF_ERROR_SOC_PPI_INVALID_GROUP:
			return "NRF_ERROR_SOC_PPI_INVALID_GROUP";
		case BLE_ERROR_NOT_ENABLED: // 0x3001
			return "BLE_ERROR_NOT_ENABLED";
		case BLE_ERROR_INVALID_CONN_HANDLE: // 0x3002
			return "BLE_ERROR_INALID_CONN_HANDLE";
		case BLE_ERROR_INVALID_ATTR_HANDLE: // 0x3003
			return "BLE_ERROR_INVALID_ATTR_HANDLE";
		case BLE_ERROR_INVALID_ADV_HANDLE: // 0x3004
			return "BLE_ERROR_INVALID_ADV_HANDLE";
		case BLE_ERROR_INVALID_ROLE: // 0x3005
			return "BLE_ERROR_INVALID_ROLE";
		case BLE_ERROR_BLOCKED_BY_OTHER_LINKS: // 0x3006
			return "BLE_ERROR_BLOCKED_BY_OTHER_LINKS";
		default:
			return "Unknown";
	}
}

constexpr const char* NordicFDSTypeName(uint32_t nordic_type) {
	switch(nordic_type) {
		case FDS_SUCCESS:
			return "FDS_SUCCESS";
		case FDS_ERR_OPERATION_TIMEOUT:
			return "FDS_ERR_OPERATION_TIMEOUT";
		case FDS_ERR_NOT_INITIALIZED:
			return "FDS_ERR_NOT_INITIALIZED";
		case FDS_ERR_UNALIGNED_ADDR:
			return "FDS_ERR_UNALIGNED_ADDR";
		case FDS_ERR_INVALID_ARG:
			return "FDS_ERR_INVALID_ARG";
		case FDS_ERR_NULL_ARG:
			return "FDS_ERR_NULL_ARG";
		case FDS_ERR_NO_OPEN_RECORDS:
			return "FDS_ERR_NO_OPEN_RECORDS";
		case FDS_ERR_NO_SPACE_IN_FLASH:
			return "FDS_ERR_NO_SPACE_IN_FLASH";
		case FDS_ERR_NO_SPACE_IN_QUEUES:
			return "FDS_ERR_NO_SPACE_IN_QUEUES";
		case FDS_ERR_RECORD_TOO_LARGE:
			return "FDS_ERR_RECORD_TOO_LARGE";
		case FDS_ERR_NOT_FOUND:
			return "FDS_ERR_NOT_FOUND";
		case FDS_ERR_NO_PAGES:
			return "FDS_ERR_NO_PAGES";
		case FDS_ERR_USER_LIMIT_REACHED:
			return "FDS_ERR_USER_LIMIT_REACHED";
		case FDS_ERR_CRC_CHECK_FAILED:
			return "FDS_ERR_CRC_CHECK_FAILED";
		case FDS_ERR_BUSY:
			return "FDS_ERR_BUSY";
		case FDS_ERR_INTERNAL:
			return "FDS_ERR_INTERNAL";
		default:
			return "Unknown";
	}
}

constexpr const char* NordicFDSEventTypeName(uint32_t nordic_type) {
	switch(nordic_type) {
		case FDS_EVT_INIT:
			return "FDS_EVT_INIT";
		case FDS_EVT_WRITE:
			return "FDS_EVT_WRITE";
		case FDS_EVT_UPDATE:
			return "FDS_EVT_UPDATE";
		case FDS_EVT_DEL_RECORD:
			return "FDS_EVT_DEL_RECORD";
		case FDS_EVT_DEL_FILE:
			return "FDS_EVT_DEL_FILE";
		case FDS_EVT_GC:
			return "FDS_EVT_GC";
		default:
			return "Unknown";
	}
}

constexpr const char* NordicEventTypeName(uint32_t nordic_type) {
	switch(nordic_type) {
		case BLE_GAP_EVT_CONNECTED:
			return "BLE_GAP_EVT_CONNECTED";
		case BLE_EVT_USER_MEM_REQUEST:
			return "BLE_EVT_USER_MEM_REQUEST";
		case BLE_EVT_USER_MEM_RELEASE:
			return "BLE_EVT_USER_MEM_RELEASE";
		case BLE_GAP_EVT_DISCONNECTED:
			return "BLE_GAP_EVT_DISCONNECTED";
		case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
			return "BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST";
		case BLE_GATTS_EVT_TIMEOUT:
			return "BLE_GATTS_EVT_TIMEOUT";
		case BLE_GAP_EVT_RSSI_CHANGED:
			return "BLE_GAP_EVT_RSSI_CHANGED";
		case BLE_GATTS_EVT_WRITE:
			return "BLE_GATTS_EVT_WRITE";
		case BLE_GATTS_EVT_HVC:
			return "BLE_GATTS_EVT_HVC";
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
			return "BLE_GATTS_EVT_SYS_ATTR_MISSING";
		case BLE_GAP_EVT_PASSKEY_DISPLAY:
			return "BLE_GAP_EVT_PASSKEY_DISPLAY";
		case BLE_GAP_EVT_ADV_REPORT:
			return "BLE_GAP_EVT_ADV_REPORT";
		case BLE_GAP_EVT_TIMEOUT:
			return "BLE_GAP_EVT_TIMEOUT";
		case BLE_GATTS_EVT_HVN_TX_COMPLETE:
			return "BLE_GATTS_EVT_HVN_TX_COMPLETE";
		default:
			return "Unknown";
	}
}

/** @brief Macro for FDS error handling (will not abort)
 */
#define FDS_ERROR_CHECK(ret_code_t)                                                                                    \
		do                                                                                                             \
		{                                                                                                              \
			const uint32_t LOCAL_ret_code_t = (ret_code_t);                                                            \
			const char* LOCAL_ret_code_str __attribute__((unused)) = NordicFDSTypeName(ret_code_t);                    \
			if (LOCAL_ret_code_t != NRF_SUCCESS)                                                                       \
			{                                                                                                          \
				LOGe("ret_code_t: %d (0x%X) %s", LOCAL_ret_code_t, LOCAL_ret_code_t, LOCAL_ret_code_str);              \
			}                                                                                                          \
		} while (0)

/** @brief Macro for calling error handler function.
 *
 * @param[in] cs_ret_code_t Error code supplied to the error handler.
 */
#define APP_ERROR_HANDLER(cs_ret_code_t)                                                                               \
		do                                                                                                             \
		{                                                                                                              \
			app_error_handler((cs_ret_code_t), __LINE__, (uint8_t*) __FILE__);                                         \
		} while (0)


/** @brief Macro for calling error handler function if supplied error code any other than NRF_SUCCESS.
 *
 * @param[in] cs_ret_code_t Error code supplied to the error handler.
 */
#define APP_ERROR_CHECK(cs_ret_code_t)                                                                                 \
		do                                                                                                             \
		{                                                                                                              \
			const uint32_t LOCAL_cs_ret_code_t = (cs_ret_code_t);                                                      \
			if (LOCAL_cs_ret_code_t != NRF_SUCCESS)                                                                    \
			{                                                                                                          \
				const char* LOCAL_ret_code_str __attribute__((unused)) = NordicTypeName(cs_ret_code_t);                \
				LOGe("cs_ret_code_t: %d (0x%X)", LOCAL_cs_ret_code_t, LOCAL_cs_ret_code_t, LOCAL_ret_code_str);        \
				APP_ERROR_HANDLER(LOCAL_cs_ret_code_t);                                                                \
			}                                                                                                          \
		} while (0)

#define APP_ERROR_CHECK_EXCEPT(cs_ret_code_t, EXCEPTION)                                                               \
		if (cs_ret_code_t == EXCEPTION) {                                                                              \
			LOGw(STRINGIFY(EXCEPTION));                                                                                \
		} else {                                                                                                       \
			APP_ERROR_CHECK(cs_ret_code_t);                                                                            \
		}

#ifdef __EXCEPTIONS

	class ble_exception : public std::exception {
	public:
		char* _message;
		ble_exception(char* message, char* file = "<unknown>", int line = 0) : _message(message) {}
		~ble_exception() throw() {}

		virtual char const *what() const throw() {
			return _message;
		}
	};

	//! A macro to throw an std::exception if the given function does not have the result NRF_SUCCESS
	//#define BLE_CALL(function, args) do { uint32_t result = function args; if (result != NRF_SUCCESS) throw ble_exception(#function, __FILE__, __LINE__); } while(0)
	
	#define BLE_CALL(function, args)                                    \
			do {                                                        \
				uint32_t result = function args;                        \
				APP_ERROR_CHECK(result);                                \
			} while(0)

	#define BLE_THROW_IF(result, message)                               \
			do {                                                        \
				if (result != NRF_SUCCESS)                              \
					throw ble_exception(message, __FILE__, __LINE__);   \
			} while(0)

	#define BLE_THROW(message) throw ble_exception(message, __FILE__, __LINE__)

#else 	/** __EXCEPTIONS */

	//#define BLE_CALL(function, args) do {volatile uint32_t result = function args; if (result != NRF_SUCCESS) {std::string ble_error_message(# function ); ble_error_handler(ble_error_message, __LINE__, __FILE__); } } while(0)
	#define BLE_CALL(function, args)                                    \
			do {                                                        \
				uint32_t result = function args;                        \
				APP_ERROR_CHECK(result);                                \
			} while(0)

	#define BLE_THROW_IF(result, message)                               \
			do {                                                        \
				if (result != NRF_SUCCESS) {                            \
					LOGd("BLE_THROW: %s", message);                     \
					ble_error_handler(message, __LINE__, __FILE__);     \
				}                                                       \
			} while(0)

	#define BLE_THROW(message)                                          \
			do {                                                        \
				LOGd("BLE_THROW: %s", message);                         \
				ble_error_handler(message, __LINE__, __FILE__);         \
			} while(0)

#endif 	/** __EXCEPTIONS */

#ifdef __cplusplus
}
#endif
