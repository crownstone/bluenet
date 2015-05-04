/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2015
 * License: LGPLv3+
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <ble/cs_Nordic.h>

/* introduces wrapper functions for the softdevice functions sd_ble_gatts_value_get and
 * sd_ble_gatts_value_set because their parameters changed from softdevice 130 v0.5.0 to
 * s130 v0.9.0
 */

/**Get the value of a given attribute.
 *
 * @conn_handle[in]   Connection handle. If there is no connection (and value is not a CCCD), then BLE_CONN_HANDLE_INVALID can be used.
 * @handle[in]        Attribute handle.
 * @p_len[in,out]     Length in bytes to be read, total length of attribute value (in bytes, starting from offset) after successful return.
 * @p_data[in,out]    Pointer to a buffer (at least len bytes long) where to store the attribute value. Set to NULL to obtain the complete length of attribute value.
 *
 * @note                 If the attribute value is longer than the size of the supplied buffer,
 *                       p_len will return the total attribute value length (excluding offset),
 *                       and not the number of bytes actually returned in p_data.
 *                       The application may use this information to allocate a suitable buffer size.
 *
 * @return NRF_SUCCESS Successfully retrieved the value of the attribute.
 * @return NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
 * @return NRF_ERROR_NOT_FOUND Attribute not found.
 */
uint32_t cs_sd_ble_gatts_value_get(uint16_t conn_handle, uint16_t handle, uint16_t * p_len, uint8_t* const p_data);

/**Set the value of a given attribute.
 *
 * @conn_handle[in]   Connection handle. If there is no connection (and value is not a CCCD), then BLE_CONN_HANDLE_INVALID can be used.
 * @handle[in]        Attribute handle.
 * @p_len[in,out]     Length in bytes to be written, length in bytes written after successful return.
 * @p_value[in]       Pointer to a buffer (at least len bytes long) containing the desired attribute value. If value is stored in user memory, only the attribute length is updated when p_value == NULL.
 *
 * @return NRF_SUCCESS Successfully set the value of the attribute.
 * @return NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
 * @return NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
 * @return NRF_ERROR_NOT_FOUND Attribute not found.
 * @return NRF_ERROR_FORBIDDEN Forbidden handle supplied, certain attributes are not modifiable by the application.
 * @return NRF_ERROR_DATA_SIZE Invalid data size(s) supplied, attribute lengths are restricted by BLE_GATTS_ATTR_LENS_MAX.
 */
uint32_t cs_sd_ble_gatts_value_set(uint16_t conn_handle, uint16_t handle, uint16_t* p_len, uint8_t * const p_data);

#ifdef __cplusplus
}
#endif

