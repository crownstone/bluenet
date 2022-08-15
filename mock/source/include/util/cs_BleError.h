#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name);

void app_error_handler_bare(uint32_t error_code);

#ifdef __cplusplus
}
#endif
