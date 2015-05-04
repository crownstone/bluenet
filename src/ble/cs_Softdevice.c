/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2015
 * License: LGPLv3+
 */

#include <ble/cs_Softdevice.h>

uint32_t cs_sd_ble_gatts_value_get(uint16_t conn_handle, uint16_t handle, uint16_t * p_len, uint8_t* const p_data) {
	uint32_t error_code;

#if (SOFTDEVICE_SERIES == 130) && (SOFTDEVICE_MAJOR == 0) && (SOFTDEVICE_MINOR == 9)
	ble_gatts_value_t p_value;
	p_value.len = *p_len;
	p_value.offset = 0;
	p_value.p_value = p_data;
	error_code = sd_ble_gatts_value_get(conn_handle, handle, &p_value);
	*p_len = p_value.len;
#else
	error_code = sd_ble_gatts_value_get(handle, 0, p_len, p_data);
#endif

	return error_code;
}

uint32_t cs_sd_ble_gatts_value_set(uint16_t conn_handle, uint16_t handle, uint16_t* p_len, uint8_t * const p_data) {
	uint32_t error_code;

#if (SOFTDEVICE_SERIES == 130) && (SOFTDEVICE_MAJOR == 0) && (SOFTDEVICE_MINOR == 9)
	ble_gatts_value_t p_value;
	p_value.len = *p_len;
	p_value.offset = 0;
	p_value.p_value = p_data;
	error_code = sd_ble_gatts_value_set(conn_handle, handle, &p_value);
	*p_len = p_value.len;
#else
	error_code = sd_ble_gatts_value_set(handle,	0, p_len, p_data);
#endif

	return error_code;
}

