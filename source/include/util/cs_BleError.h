/**
 * Author: Crownstone Team
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <logging/cs_CLogger.h>
#include <util/cs_Error.h>

// These macros can end up in C code, so use the C logger.

#ifdef __cplusplus
extern "C" {
#endif

//! Called by BluetoothLE.h classes when exceptions are disabled.
void ble_error_handler(const char* msg, uint32_t line_num, const char* p_file_name);

//! called by soft device when you pass bad parameters, etc.
// void app_error_handler (uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name);

// called by NRF SDK when it has an internal error.
void assert_nrf_callback(uint16_t line_num, const uint8_t* file_name);

// called by soft device when it has an internal error.
void softdevice_assertion_handler(uint32_t pc, uint16_t line_num, const uint8_t* file_name);

void mesh_assertion_handler(uint32_t pc);

/** @brief Macro for FDS error handling (will not abort)
 */
#define FDS_ERROR_CHECK(ret_code_t)                                                                 \
	do {                                                                                            \
		const uint32_t LOCAL_ret_code_t = (ret_code_t);                                             \
		if (LOCAL_ret_code_t != NRF_SUCCESS) {                                                      \
			CLOGe("ret_code_t: $nordicFDSTypeName(%u) (0x%X)", LOCAL_ret_code_t, LOCAL_ret_code_t); \
		}                                                                                           \
	} while (0)

/** @brief Macro for calling error handler function.
 *
 * @param[in] cs_ret_code_t Error code supplied to the error handler.
 */
#ifdef APP_ERROR_HANDLER
#undef APP_ERROR_HANDLER
#endif
#define APP_ERROR_HANDLER(cs_ret_code_t)                                  \
	do {                                                                  \
		app_error_handler((cs_ret_code_t), __LINE__, (uint8_t*)__FILE__); \
	} while (0)

/** @brief Macro for calling error handler function if supplied error code any other than NRF_SUCCESS.
 *
 * @param[in] cs_ret_code_t Error code supplied to the error handler.
 */
#ifdef APP_ERROR_CHECK
#undef APP_ERROR_CHECK
#endif
#define APP_ERROR_CHECK(cs_ret_code_t)                                                                    \
	do {                                                                                                  \
		const uint32_t LOCAL_cs_ret_code_t = (cs_ret_code_t);                                             \
		if (LOCAL_cs_ret_code_t != NRF_SUCCESS) {                                                         \
			CLOGe("cs_ret_code_t: $nordicTypeName(%u) (0x%X)", LOCAL_cs_ret_code_t, LOCAL_cs_ret_code_t); \
			APP_ERROR_HANDLER(LOCAL_cs_ret_code_t);                                                       \
		}                                                                                                 \
	} while (0)

#define APP_ERROR_CHECK_EXCEPT(cs_ret_code_t, EXCEPTION) \
	if (cs_ret_code_t == EXCEPTION) {                    \
		CLOGw(STRINGIFY((int)EXCEPTION));                \
	}                                                    \
	else {                                               \
		APP_ERROR_CHECK(cs_ret_code_t);                  \
	}

#ifdef __EXCEPTIONS
#include <exception>

class ble_exception : public std::exception {
public:
	char* _message;
	ble_exception(char* message, char* file = "<unknown>", int line = 0) : _message(message) {}
	~ble_exception() throw() {}

	virtual char const* what() const throw() { return _message; }
};

//! A macro to throw an std::exception if the given function does not have the result NRF_SUCCESS
//#define BLE_CALL(function, args) do { uint32_t result = function args; if (result != NRF_SUCCESS) throw
// ble_exception(#function, __FILE__, __LINE__); } while(0)

#define BLE_CALL(function, args)         \
	do {                                 \
		uint32_t result = function args; \
		APP_ERROR_CHECK(result);         \
	} while (0)

#define BLE_THROW_IF(result, message)                                                \
	do {                                                                             \
		if (result != NRF_SUCCESS) throw ble_exception(message, __FILE__, __LINE__); \
	} while (0)

#define BLE_THROW(message) throw ble_exception(message, __FILE__, __LINE__)

#else /** __EXCEPTIONS */

//#define BLE_CALL(function, args) do {volatile uint32_t result = function args; if (result != NRF_SUCCESS) {std::string
// ble_error_message(# function ); ble_error_handler(ble_error_message, __LINE__, __FILE__); } } while(0)
#define BLE_CALL(function, args)         \
	do {                                 \
		uint32_t result = function args; \
		APP_ERROR_CHECK(result);         \
	} while (0)

#define BLE_THROW_IF(result, message)                       \
	do {                                                    \
		if (result != NRF_SUCCESS) {                        \
			CLOGd("BLE_THROW: %s", message);                \
			ble_error_handler(message, __LINE__, __FILE__); \
		}                                                   \
	} while (0)

#define BLE_THROW(message)                              \
	do {                                                \
		CLOGd("BLE_THROW: %s", message);                \
		ble_error_handler(message, __LINE__, __FILE__); \
	} while (0)

#endif /** __EXCEPTIONS */

#ifdef __cplusplus
}
#endif
