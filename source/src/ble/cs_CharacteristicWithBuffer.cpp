/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 30, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_CharacteristicWithBuffer.h>

struct __attribute__((__packed__)) notification_t {
	uint8_t partNr;
	uint8_t data[MAX_NOTIFICATION_LEN];
};

uint32_t Characteristic<buffer_ptr_t>::notify() {

	if (!CharacteristicBase::_state.notifies || !_service->getStack()->isConnectedPeripheral()
		|| !_state.notifyingEnabled) {
		return NRF_ERROR_INVALID_STATE;
	}

	uint32_t nrfCode          = NRF_SUCCESS;

	// get the data length of the value (encrypted)
	uint16_t valueLength      = getGattValueLength();
	uint8_t* valueGattAddress = getGattValuePtr();

	ble_gatts_hvx_params_t hvx_params;

	uint16_t offset;
	if (_state.notificationPending) {
		offset = _notificationPendingOffset;
	}
	else {
		offset = 0;
	}

	while (offset < valueLength) {
		uint16_t dataLen            = MIN(valueLength - offset, MAX_NOTIFICATION_LEN);

		notification_t notification = {};

		// TODO: [Alex 22.08] verify if the oldVal is required. I do not think we use this.
		uint8_t oldVal[sizeof(notification_t)];

		if (valueLength - offset > MAX_NOTIFICATION_LEN) {
			notification.partNr = offset / MAX_NOTIFICATION_LEN;
		}
		else {
			notification.partNr = CS_CHARACTERISTIC_NOTIFICATION_PART_LAST;
		}

		//		LOGi("dataLen: %d ", dataLen);
		//		CsUtils::printArray(valueGattAddress, valueLength);

		memcpy(&notification.data, valueGattAddress + offset, dataLen);

		uint16_t packageLen = dataLen + sizeof(notification.partNr);
		uint8_t* p_data     = (uint8_t*)&notification;

		//		LOGi("address: %p", valueGattAddress + offset);
		//		LOGi("offset: %d", offset);
		//		CsUtils::printArray((uint8_t*)valueGattAddress + offset, dataLen);
		//		CsUtils::printArray((uint8_t*)&notification, packageLen);

		memcpy(oldVal, valueGattAddress + offset, packageLen);

		hvx_params.handle = _handles.value_handle;
		hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
		hvx_params.offset = 0;
		hvx_params.p_len  = &packageLen;
		hvx_params.p_data = p_data;

		nrfCode           = sd_ble_gatts_hvx(_service->getStack()->getConnectionHandle(), &hvx_params);
		switch (nrfCode) {
			case NRF_SUCCESS: {
				memcpy(valueGattAddress + offset, oldVal, packageLen);
				offset += dataLen;
				break;
			}

			case NRF_ERROR_RESOURCES: {
				// * @retval ::NRF_ERROR_RESOURCES Too many notifications queued.
				// *                               Wait for a @ref BLE_GATTS_EVT_HVN_TX_COMPLETE event and retry.
				// Mark that there is a pending notification, we can continue later.
				onNotifyTxError();
				_notificationPendingOffset = offset;
				return nrfCode;
			}

			case NRF_ERROR_TIMEOUT:
				// * @retval ::NRF_ERROR_TIMEOUT There has been a GATT procedure timeout. No new GATT procedure can be
				// performed without reestablishing the connection. It can happen there was a timeout in the meantime.
			case NRF_ERROR_INVALID_STATE: {
				// * @retval ::NRF_ERROR_INVALID_STATE One or more of the following is true:
				// *                                   - Invalid Connection State
				// *                                   - Notifications and/or indications not enabled in the CCCD
				// *                                   - An ATT_MTU exchange is ongoing
				// It can happen that the phone disconnected or disabled notification in the meantime.
				LOGw("nrfCode=%u (0x%X)", nrfCode, nrfCode);

				// Reset the notification state, we can't continue later.
				_notificationPendingOffset  = 0;
				_state.notificationPending = false;
				return nrfCode;
			}

			case BLE_ERROR_GATTS_SYS_ATTR_MISSING: {
				// * @retval ::BLE_ERROR_GATTS_SYS_ATTR_MISSING System attributes missing, use @ref
				// sd_ble_gatts_sys_attr_set to set them to a known value. Anne: do not complain for now... (meshing)
				LOGe("nrfCode=%u (0x%X)", nrfCode, nrfCode);
				break;
			}

			case BLE_ERROR_INVALID_CONN_HANDLE: {
				// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid Connection Handle.
				// This shouldn't happen, as the connection handle is only set in the main thread.
				LOGe("Invalid handle");
				break;
			}

			case NRF_ERROR_BUSY:
				// * @retval ::NRF_ERROR_BUSY For @ref BLE_GATT_HVX_INDICATION Procedure already in progress. Wait for a
				// @ref BLE_GATTS_EVT_HVC event and retry. This shouldn't happen, as we don't use INDICATION, but
				// NOTIFICATION.
			case NRF_ERROR_INVALID_ADDR:
				// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
				// This shouldn't happen: crash.
			case NRF_ERROR_INVALID_PARAM:
				// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
				// This shouldn't happen: crash.
			case BLE_ERROR_INVALID_ATTR_HANDLE:
				// * @retval ::BLE_ERROR_INVALID_ATTR_HANDLE Invalid attribute handle(s) supplied. Only attributes added
				// directly by the application are available to notify and indicate. This shouldn't happen: crash.
			case BLE_ERROR_GATTS_INVALID_ATTR_TYPE:
				// * @retval ::BLE_ERROR_GATTS_INVALID_ATTR_TYPE Invalid attribute type(s) supplied, only characteristic
				// values may be notified and indicated. This shouldn't happen: crash.
			case NRF_ERROR_NOT_FOUND:
				// * @retval ::NRF_ERROR_NOT_FOUND Attribute not found.
				// This shouldn't happen: crash.
			case NRF_ERROR_FORBIDDEN:
				// * @retval ::NRF_ERROR_FORBIDDEN The connection's current security level is lower than the one
				// required by the write permissions of the CCCD associated with this characteristic. This shouldn't
				// happen: crash.
			case NRF_ERROR_DATA_SIZE:
				// * @retval ::NRF_ERROR_DATA_SIZE Invalid data size(s) supplied.
				// This shouldn't happen: crash.
			default:
				// Crash
				APP_ERROR_HANDLER(nrfCode);
		}
	}
	return nrfCode;
}

void Characteristic<buffer_ptr_t>::onTxComplete(__attribute__((unused)) const ble_common_evt_t* p_ble_evt) {
	// if we have a notification pending, try to send it
	if (_state.notificationPending) {
		// this-> is necessary so that the call of notify depends on the template
		// parameter T
		uint32_t err_code = this->notify();
		if (err_code == NRF_SUCCESS) {
			//				LOGi("ontx success");
			_state.notificationPending = false;
			_notificationPendingOffset  = 0;
		}
	}
}
