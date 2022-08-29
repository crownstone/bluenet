/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Characteristic.h>
#include <storage/cs_State.h>

CharacteristicBase::CharacteristicBase() : _handles({}), _status({}) {}

/**
 * Set the default attributes of every characteristic
 *
 * There are two settings for the location of the memory of the buffer that is used to communicate with the SoftDevice.
 *
 * + BLE_GATTS_VLOC_STACK
 * + BLE_GATTS_VLOC_USER
 *
 * The former makes the SoftDevice allocate the memory (the quantity of which is defined by the user). The latter
 * leaves it to the user. It is essential that with BLE_GATTS_VLOC_USER the calls to sd_ble_gatts_value_set() are
 * handed persistent pointers to a buffer. If a temporary value is used, this will screw up the data read by the user.
 * The same is true if this buffer is reused across characteristics. If it is meant as data for one characteristic
 * and is read through another, this will resolve to nonsense to the user.
 */
void CharacteristicBase::init(Service* svc) {
	if (_status.initialized) {
		return;
	}

	_service = svc;

	// Attribute metadata for client characteristic configuration
	ble_gatts_attr_md_t cccdMetadata;
	memset(&cccdMetadata, 0, sizeof(cccdMetadata));
	cccdMetadata.vloc = BLE_GATTS_VLOC_STACK;
	cccdMetadata.vlen = 1;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdMetadata.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdMetadata.write_perm);

	ble_gatts_char_md_t characteristicMetadata;
	memset(&characteristicMetadata, 0, sizeof(characteristicMetadata));
	characteristicMetadata.char_props.read      = 1;  // allow read
	characteristicMetadata.char_props.broadcast = 0;  // don't allow broadcast
	characteristicMetadata.char_props.write     = _status.writable ? 1 : 0;
	characteristicMetadata.char_props.notify    = _status.notifies ? 1 : 0;
	characteristicMetadata.char_props.indicate  = _status.indicates ? 1 : 0;
	characteristicMetadata.p_cccd_md            = &cccdMetadata;

	// The user description is optional.
	characteristicMetadata.p_char_user_desc     = nullptr;
	characteristicMetadata.p_user_desc_md       = nullptr;

	// Presentation format is optional.
	characteristicMetadata.p_char_pf            = nullptr;

	ble_gatts_attr_md_t attributeMetadata;
	memset(&attributeMetadata, 0, sizeof(attributeMetadata));
	attributeMetadata.vloc = BLE_GATTS_VLOC_USER;
	attributeMetadata.vlen = 1;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attributeMetadata.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attributeMetadata.write_perm);

	// Attribute value
	const ble_uuid_t& uuid = _uuid.getUuid();

	ble_gatts_attr_t characteristicValue;
	memset(&characteristicValue, 0, sizeof(characteristicValue));
	characteristicValue.p_uuid    = &uuid;
	characteristicValue.init_offs = 0;
	characteristicValue.init_len  = getGattValueLength();
	characteristicValue.max_len   = getGattValueMaxLength();
	characteristicValue.p_value   = getGattValuePtr();
	characteristicValue.p_attr_md = &attributeMetadata;

	LOGCharacteristicDebug("init with buffer=%p of len=%u", getGattValuePtr(), getGattValueMaxLength());

	// Add to softdevice.
	uint32_t nrfCode =
			sd_ble_gatts_characteristic_add(svc->getHandle(), &characteristicMetadata, &characteristicValue, &_handles);
	switch (nrfCode) {
		case NRF_SUCCESS: break;
		case NRF_ERROR_INVALID_ADDR:
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
		case NRF_ERROR_INVALID_PARAM:
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied, service handle, Vendor Specific UUIDs,
			// lengths, and permissions need to adhere to the constraints. This shouldn't happen: crash.
		case NRF_ERROR_INVALID_STATE:
			// * @retval ::NRF_ERROR_INVALID_STATE Invalid state to perform operation, a service context is required.
			// This shouldn't happen: crash.
		case NRF_ERROR_FORBIDDEN:
			// * @retval ::NRF_ERROR_FORBIDDEN Forbidden value supplied, certain UUIDs are reserved for the stack.
			// This shouldn't happen: crash.
		case NRF_ERROR_NO_MEM:
			// * @retval ::NRF_ERROR_NO_MEM Not enough memory to complete operation.
			// This shouldn't happen: crash.
		case NRF_ERROR_DATA_SIZE:
			// * @retval ::NRF_ERROR_DATA_SIZE Invalid data size(s) supplied, attribute lengths are restricted by @ref
			// BLE_GATTS_ATTR_LENS_MAX. This shouldn't happen: crash.
		default:
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}

	// Set initial value (default value)
	updateValue();
	_status.initialized = true;
}

void CharacteristicBase::setName(const char* const name) {
	_name = name;
}

/**
 * Update characteristic value.
 * This is also required when switching to/from encryption.
 */
uint32_t CharacteristicBase::updateValue(ConnectionEncryptionType encryptionType) {

	// get the data length of the value (unencrypted)
	uint16_t valueLength      = getValueLength();

	// get the address where the value should be stored so that the gatt server can access it
	uint8_t* valueGattAddress = getGattValuePtr();

	if (_status.aesEncrypted && _minAccessLevel < ENCRYPTION_DISABLED) {
		// GATT is public facing, getValue is internal
		// getValuePtr is not padded, it's the size of an int, or string or whatever is required.
		// the valueGattAddress can be used as buffer for encryption
		_log(LogLevelCharacteristicDebug, false, "data: ");
		_logArray(LogLevelCharacteristicDebug, true, getValuePtr(), valueLength);

		// we calculate what size buffer we need
		cs_ret_code_t retVal            = ERR_SUCCESS;
		uint16_t encryptionBufferLength = ConnectionEncryption::getEncryptedBufferSize(valueLength, encryptionType);
		if (encryptionBufferLength > getGattValueMaxLength()) {
			retVal = ERR_BUFFER_TOO_SMALL;
		}
		else {
			retVal = ConnectionEncryption::getInstance().encrypt(
					cs_data_t(getValuePtr(), valueLength),
					cs_data_t(valueGattAddress, encryptionBufferLength),
					_minAccessLevel,
					encryptionType);
			_log(LogLevelCharacteristicDebug, false, "encrypted: ");
			_logArray(LogLevelCharacteristicDebug, true, valueGattAddress, encryptionBufferLength);
		}
		if (!SUCCESS(retVal)) {
			// clear the partially encrypted buffer.
			memset(valueGattAddress, 0x00, encryptionBufferLength);

			// make sure nothing will be read
			setGattValueLength(0);

			// disconnect from the device.
			Stack::getInstance().disconnect();
			LOGe("error encrypting data.");
			return retVal;
		}

		// on success, set the readable buffer length to the encryption package.
		setGattValueLength(encryptionBufferLength);
	}
	else {
		// set the data length of the gatt value (when not using encryption)
		setGattValueLength(valueLength);
	}

	uint16_t gattValueLength = getGattValueLength();
	LOGCharacteristicDebug(
			"gattValueLength=%u gattValueAddress=%p, gattValueMaxSize=%u",
			gattValueLength,
			valueGattAddress,
			getGattValueMaxLength());

	ble_gatts_value_t gatts_value;
	gatts_value.len     = gattValueLength;
	gatts_value.offset  = 0;
	gatts_value.p_value = valueGattAddress;

	uint32_t nrfCode =
			sd_ble_gatts_value_set(_service->getStack()->getConnectionHandle(), _handles.value_handle, &gatts_value);
	switch (nrfCode) {
		case NRF_SUCCESS: break;
		case BLE_ERROR_INVALID_CONN_HANDLE:
			// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied on a system attribute.
			// This shouldn't happen, as the connection handle is only set in the main thread.
			LOGe("Invalid handle");
			break;
		case NRF_ERROR_INVALID_ADDR:
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
		case NRF_ERROR_INVALID_PARAM:
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
			// This shouldn't happen: crash.
		case NRF_ERROR_NOT_FOUND:
			// * @retval ::NRF_ERROR_NOT_FOUND Attribute not found.
			// This shouldn't happen: crash.
		case NRF_ERROR_FORBIDDEN:
			// * @retval ::NRF_ERROR_FORBIDDEN Forbidden handle supplied, certain attributes are not modifiable by the
			// application. This shouldn't happen: crash.
		case NRF_ERROR_DATA_SIZE:
			// * @retval ::NRF_ERROR_DATA_SIZE Invalid data size(s) supplied, attribute lengths are restricted by @ref
			// BLE_GATTS_ATTR_LENS_MAX. This shouldn't happen: crash.
		default:
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}

	// Stop here if we are not in notifying state
	if ((!_status.notifies) || (!_service->getStack()->isConnectedPeripheral()) || !_status.notifyingEnabled) {
		return ERR_SUCCESS;
	}
	else {
		return notify();
	}
}

uint32_t CharacteristicBase::notify() {

	if (!_status.notifies || !_service->getStack()->isConnectedPeripheral() || !_status.notifyingEnabled) {
		return NRF_ERROR_INVALID_STATE;
	}

	uint32_t nrfCode;

	// Get the data length of the value (encrypted)
	uint16_t valueLength = getGattValueLength();

	ble_gatts_hvx_params_t hvx_params;

	hvx_params.handle = _handles.value_handle;
	hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
	hvx_params.offset = 0;
	hvx_params.p_len  = &valueLength;
	hvx_params.p_data = NULL;

	nrfCode           = sd_ble_gatts_hvx(_service->getStack()->getConnectionHandle(), &hvx_params);

	switch (nrfCode) {
		case NRF_SUCCESS: break;
		case NRF_ERROR_RESOURCES:
			// * @retval ::NRF_ERROR_RESOURCES Too many notifications queued.
			// *                               Wait for a @ref BLE_GATTS_EVT_HVN_TX_COMPLETE event and retry.
			// Mark that there is a pending notification, we can retry later.
			onNotifyTxError();
			break;
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

			// Reset the notification state, we can't retry later.
			_status.notificationPending = false;
			break;
		}
		case BLE_ERROR_GATTS_SYS_ATTR_MISSING: {
			// * @retval ::BLE_ERROR_GATTS_SYS_ATTR_MISSING System attributes missing, use @ref
			// sd_ble_gatts_sys_attr_set to set them to a known value.
			// TODO: Currently excluded from APP_ERROR_CHECK, seems to originate from MESH code
			LOGe("nrfCode=%u (0x%X)", nrfCode, nrfCode);
			break;
		}
		default:
			// Other return codes, see Characteristic<buffer_ptr_t>::notify().
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}

	return nrfCode;
}

#define MAX_NOTIFICATION_LEN 19

struct __attribute__((__packed__)) notification_t {
	uint8_t partNr;
	uint8_t data[MAX_NOTIFICATION_LEN];
};

uint32_t Characteristic<buffer_ptr_t>::notify() {

	if (!CharacteristicBase::_status.notifies || !_service->getStack()->isConnectedPeripheral()
		|| !_status.notifyingEnabled) {
		return NRF_ERROR_INVALID_STATE;
	}

	uint32_t nrfCode          = NRF_SUCCESS;

	// get the data length of the value (encrypted)
	uint16_t valueLength      = getGattValueLength();
	uint8_t* valueGattAddress = getGattValuePtr();

	ble_gatts_hvx_params_t hvx_params;

	uint16_t offset;
	if (_status.notificationPending) {
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
				_status.notificationPending = false;
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

/** When a previous transmission is successful send the next.
 */
void CharacteristicBase::onTxComplete(__attribute__((unused)) const ble_common_evt_t* p_ble_evt) {
	// if we have a notification pending, try to send it
	if (_status.notificationPending) {
		// this-> is necessary so that the call of notify depends on the template
		// parameter T
		uint32_t err_code = this->notify();
		if (err_code == NRF_SUCCESS) {
			//				LOGi("ontx success");
			_status.notificationPending = false;
		}
	}
}

void Characteristic<buffer_ptr_t>::onTxComplete(__attribute__((unused)) const ble_common_evt_t* p_ble_evt) {
	// if we have a notification pending, try to send it
	if (_status.notificationPending) {
		// this-> is necessary so that the call of notify depends on the template
		// parameter T
		uint32_t err_code = this->notify();
		if (err_code == NRF_SUCCESS) {
			//				LOGi("ontx success");
			_status.notificationPending = false;
			_notificationPendingOffset  = 0;
		}
	}
}
