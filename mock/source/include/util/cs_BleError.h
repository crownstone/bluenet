#pragma once

#include <stdint.h>
#include <cassert>

#ifdef __cplusplus
extern "C" {
#endif

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t* p_file_name);

void app_error_handler_bare(uint32_t error_code);

#define APP_ERROR_CHECK(RES)        \
    do {                            \
        assert(RES == NRF_SUCCESS); \
    } while (0)

#define BLE_CALL(function, args)         \
    do {                                 \
        uint32_t result = function args; \
        APP_ERROR_CHECK(result);         \
    } while (0)


#define APP_ERROR_HANDLER(RET_CODE)                                  \
    do {                                                             \
        app_error_handler((RET_CODE), __LINE__, (uint8_t*)__FILE__); \
    } while (0)

#ifdef __cplusplus
}
#endif
