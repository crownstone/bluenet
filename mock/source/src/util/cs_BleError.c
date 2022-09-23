#include <stdint.h>
#include <stdlib.h>

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
	exit(error_code);
}

void app_error_handler_bare(uint32_t error_code) {
	exit(error_code);
}
