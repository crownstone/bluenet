#ifndef BLE_ERROR_H
#define BLE_ERROR_H

#include <string> 
#include <stdint.h> 

#include "log.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Called by BluetoothLE.h classes when exceptions are disabled.
void ble_error_handler (std::string msg, uint32_t line_num, const char * p_file_name);

// called by soft device when you pass bad parameters, etc.
void app_error_handler (uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name);

//called by NRF SDK when it has an internal error.
void assert_nrf_callback (uint16_t line_num, const uint8_t *file_name);

//called by soft device when it has an internal error.
void softdevice_assertion_handler(uint32_t pc, uint16_t line_num, const uint8_t * file_name);

/**@brief Macro for calling error handler function. 
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */
#define APP_ERROR_HANDLER(ERR_CODE)                         \
    do                                                      \
    {                                                       \
        app_error_handler((ERR_CODE), __LINE__, (uint8_t*) __FILE__);  \
    } while (0)


/**@brief Macro for calling error handler function if supplied error code any other than NRF_SUCCESS.
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */
#define APP_ERROR_CHECK(ERR_CODE)                           \
    do                                                      \
    {                                                       \
        const uint32_t LOCAL_ERR_CODE = (ERR_CODE);         \
        if (LOCAL_ERR_CODE != NRF_SUCCESS)                  \
        {                                                   \
        	LOG_DEBUG("ERR_CODE: %d", LOCAL_ERR_CODE);		\
            APP_ERROR_HANDLER(LOCAL_ERR_CODE);              \
        }                                                   \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif // BLE_ERROR_H
