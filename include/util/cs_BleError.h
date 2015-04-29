/**
 * Author: Christopher mason
 * Date: 21 Sep., 2013
 * License: TODO
 */
#pragma once

#include <ble/cs_Nordic.h>

//#include <stdint.h>
//#include <common/cs_Types.h>

#include "drivers/cs_Serial.h"

#ifdef __cplusplus
extern "C"
{
#endif


// Called by BluetoothLE.h classes when exceptions are disabled.
void ble_error_handler (const char * msg, uint32_t line_num, const char * p_file_name);

// called by soft device when you pass bad parameters, etc.
//void app_error_handler (uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name);

//called by NRF SDK when it has an internal error.
void assert_nrf_callback (uint16_t line_num, const uint8_t *file_name);

//called by soft device when it has an internal error.
void softdevice_assertion_handler(uint32_t pc, uint16_t line_num, const uint8_t * file_name);

/**@brief Macro for calling error handler function.
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */
#define APP_ERROR_HANDLER(ERR_CODE)                                         \
		do                                                                  \
		{                                                                   \
			app_error_handler((ERR_CODE), __LINE__, (uint8_t*) __FILE__);   \
		} while (0)


/**@brief Macro for calling error handler function if supplied error code any other than NRF_SUCCESS.
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */
#define APP_ERROR_CHECK(ERR_CODE)                                                   \
		do                                                                          \
		{                                                                           \
			const uint32_t LOCAL_ERR_CODE = (ERR_CODE);                             \
			if (LOCAL_ERR_CODE != NRF_SUCCESS)                                      \
			{                                                                       \
				LOGd("ERR_CODE: %d (0x%X)", LOCAL_ERR_CODE, LOCAL_ERR_CODE);		\
				APP_ERROR_HANDLER(LOCAL_ERR_CODE);                                  \
			}                                                                       \
		} while (0)


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

	// A macro to throw an std::exception if the given function does not have the result NRF_SUCCESS
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

#else /* __EXCEPTIONS */

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

#endif /* __EXCEPTIONS */

#ifdef	NDEBUG

// for release version ignore asserts
#define assert(expr, message)

#else

#define assert(expr, message) \
	if (!(expr)) { \
		LOGe("%s", message); \
		exit(1); \
	}

#endif

#ifdef __cplusplus
}
#endif
