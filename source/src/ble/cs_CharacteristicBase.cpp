/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_CharacteristicBase.h>
#include <ble/cs_Nordic.h>
#include <ble/cs_Stack.h>
#include <ble/cs_UUID.h>
#include <encryption/cs_KeysAndAccess.h>
#include <logging/cs_Logger.h>
#include <structs/buffer/cs_EncryptedBuffer.h>

#define LOGCharacteristicInfo LOGi
#define LOGCharacteristicDebug LOGd
#define LOGCharacteristicVerbose LOGvv
#define LogLevelCharacteristicVerbose SERIAL_VERY_VERBOSE

CharacteristicBase::CharacteristicBase() {}

cs_ret_code_t CharacteristicBase::setName(const char* const name) {
	_name = name;
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::setUuid(uint16_t uuid) {
	if (_initialized) {
		LOGw("Already initialized");
		return ERR_WRONG_STATE;
	}
	_uuid = uuid;
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::setConfig(const characteristic_config_t& config) {
	if (_initialized) {
		LOGw("Already initialized");
		return ERR_WRONG_STATE;
	}
	_config = config;
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::setEventHandler(const characteristic_callback_t& closure) {
	if (_initialized) {
		LOGw("Already initialized");
		return ERR_WRONG_STATE;
	}
	_callback = closure;
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::setValueBuffer(buffer_ptr_t buffer, cs_buffer_size_t size) {
	if (_initialized) {
		LOGw("Already initialized");
		return ERR_WRONG_STATE;
	}
	if (buffer == nullptr || size == 0) {
		return ERR_BUFFER_UNASSIGNED;
	}
	LOGCharacteristicVerbose("setValueBuffer [%s] buf=%p size=%u", _name, buffer, size);
	_buffer.data = buffer;
	_buffer.len  = size;
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::setInitialValueLength(cs_buffer_size_t size) {
	LOGCharacteristicVerbose("setInitialValueLength [%s] size=%u", _name, size);
	if (_initialized) {
		LOGw("Already initialized");
		return ERR_WRONG_STATE;
	}
	if (_buffer.len < size) {
		LOGw("Buffer too small");
		return ERR_BUFFER_TOO_SMALL;
	}
	_valueLength = size;
	return ERR_SUCCESS;
}

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
cs_ret_code_t CharacteristicBase::init(Service* service) {
	if (_name == nullptr) {
		_name = "";
	}
	LOGCharacteristicDebug("Init [%s] uuid=%u", _name, _uuid);
	if (_initialized) {
		LOGCharacteristicDebug("Already initialized");
		return ERR_SUCCESS;
	}

	_service = service;

	// Attribute metadata for client characteristic configuration
	ble_gatts_attr_md_t cccdMetadata;
	memset(&cccdMetadata, 0, sizeof(cccdMetadata));
	cccdMetadata.vloc = BLE_GATTS_VLOC_STACK;
	cccdMetadata.vlen = 1;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdMetadata.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdMetadata.write_perm);

	ble_gatts_char_md_t characteristicMetadata;
	memset(&characteristicMetadata, 0, sizeof(characteristicMetadata));
	characteristicMetadata.char_props.broadcast     = 0;
	characteristicMetadata.char_props.read          = _config.read ? 1 : 0;
	characteristicMetadata.char_props.write_wo_resp = _config.write ? 1 : 0;
	characteristicMetadata.char_props.write         = _config.write ? 1 : 0;
	characteristicMetadata.char_props.notify        = _config.notify ? 1 : 0;
	characteristicMetadata.char_props.indicate      = _config.notify ? 1 : 0;
	// For some reason it doesn't matter if char_ext_props.reliable_wr = 0
	// We can still perform long writes.
	characteristicMetadata.p_cccd_md                = &cccdMetadata;

	// The user description is optional.
	characteristicMetadata.p_char_user_desc         = nullptr;
	characteristicMetadata.p_user_desc_md           = nullptr;

	// Presentation format is optional.
	characteristicMetadata.p_char_pf                = nullptr;

	ble_gatts_attr_md_t attributeMetadata;
	memset(&attributeMetadata, 0, sizeof(attributeMetadata));
	attributeMetadata.vloc = BLE_GATTS_VLOC_USER;
	attributeMetadata.vlen = 1;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attributeMetadata.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attributeMetadata.write_perm);

	// Attribute value
	UUID fullUuid;
	cs_ret_code_t retCode = fullUuid.fromBaseUuid(_service->getUUID(), _uuid);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	const ble_uuid_t& uuid = fullUuid.getUuid();

	retCode                = initEncryptedBuffer();
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	ble_gatts_attr_t characteristicValue;
	memset(&characteristicValue, 0, sizeof(characteristicValue));
	characteristicValue.p_uuid    = &uuid;
	characteristicValue.init_offs = 0;
	characteristicValue.init_len  = getGattValueLength();
	characteristicValue.max_len   = getGattValueMaxLength();
	characteristicValue.p_value   = getGattValue();
	characteristicValue.p_attr_md = &attributeMetadata;

	LOGCharacteristicVerbose(
			"  add with gatt buffer=%p of size=%u and value length=%u",
			getGattValue(),
			getGattValueMaxLength(),
			getGattValueLength());

	// Add to softdevice.
	uint32_t nrfCode = sd_ble_gatts_characteristic_add(
			_service->getHandle(), &characteristicMetadata, &characteristicValue, &_handles);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			retCode = ERR_SUCCESS;
			break;
		}
		case NRF_ERROR_INVALID_ADDR: {
			retCode = ERR_BUFFER_UNASSIGNED;
			break;
		}
		case NRF_ERROR_INVALID_PARAM:
		case NRF_ERROR_INVALID_STATE:
		case NRF_ERROR_FORBIDDEN:
		case NRF_ERROR_DATA_SIZE: {
			retCode = ERR_WRONG_PARAMETER;
			break;
		}
		case NRF_ERROR_NO_MEM: {
			retCode = ERR_NO_SPACE;
			break;
		}
		default: {
			retCode = ERR_UNSPECIFIED;
			break;
		}
	}

	if (retCode != ERR_SUCCESS) {
		LOGe("Failed to add characteristic: nrfCode=%u", nrfCode);
		// Cleanup
		deinitEncryptedBuffer();
		return retCode;
	}

	LOGCharacteristicVerbose(
			"  added successfully, handles: value=%u cccd=%u", _handles.value_handle, _handles.cccd_handle);

	// Make sure the initial value is encrypted.
	updateValue(_valueLength);

	_initialized = true;
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::initEncryptedBuffer() {
	if (!isEncrypted()) {
		return ERR_SUCCESS;
	}

	if (_encryptedBuffer.data != nullptr) {
		LOGw("Already initialized encryption buffer");
		// Assume the correct buffer of correct size has been set.
		return ERR_SUCCESS;
	}

	// The required size of the encrypted buffer depends on the max value size, and the encryption type.
	uint16_t requiredSize = ConnectionEncryption::getEncryptedBufferSize(_buffer.len, _config.encryptionType);

	if (_config.sharedEncryptionBuffer) {
		EncryptedBuffer::getInstance().getBuffer(
				_encryptedBuffer.data, _encryptedBuffer.len, CS_CHAR_BUFFER_DEFAULT_OFFSET);
		if (requiredSize > _encryptedBuffer.len) {
			LOGw("Encrypted buffer size too small: size=%u required=%u", _encryptedBuffer.len, requiredSize);
			_encryptedBuffer.data = nullptr;
			_encryptedBuffer.len  = 0;
			return ERR_BUFFER_TOO_SMALL;
		}
		LOGCharacteristicVerbose(
				"Use shared encryption [%s] buffer=%p size=%u", _name, _encryptedBuffer.data, _encryptedBuffer.len);
	}
	else {
		_encryptedBuffer.data = (buffer_ptr_t)calloc(requiredSize, sizeof(uint8_t));
		if (_encryptedBuffer.data == nullptr) {
			LOGw("Unable to allocate encryption buffer");
			return ERR_NO_SPACE;
		}
		_encryptedBuffer.len = requiredSize;
		LOGCharacteristicVerbose(
				"Allocated encrypted buffer [%s] buffer=%p size=%u",
				_name,
				_encryptedBuffer.data,
				_encryptedBuffer.len);
	}
	return ERR_SUCCESS;
}

void CharacteristicBase::deinitEncryptedBuffer() {
	if (!_config.sharedEncryptionBuffer && _encryptedBuffer.data != nullptr) {
		free(_encryptedBuffer.data);
		_encryptedBuffer.data = nullptr;
		_encryptedBuffer.len  = 0;
	}
}

cs_ret_code_t CharacteristicBase::updateValue(uint16_t length) {
	if (length > _buffer.len) {
		return ERR_BUFFER_TOO_SMALL;
	}
	_valueLength = length;

	_log(LogLevelCharacteristicVerbose, false, "updateValue [%s] length=%u data=", _name, length);
	_logArray(LogLevelCharacteristicVerbose, true, _buffer.data, _valueLength);

	if (isEncrypted()) {
		// Special case: if value length is 0, we simply want the encrypted value to be 0 as well.
		// Especially for characteristics that use the shared encrypted buffer. Because encrypting
		// a length of 0 still would result in some data, that overwrite the shared encrypted buffer.
		if (length == 0) {
			_encryptedValueLength = 0;
			return setGattValue();
		}

		// Encrypt the value (but only the current length).
		cs_ret_code_t retCode = ConnectionEncryption::getInstance().encrypt(
				cs_data_t(_buffer.data, _valueLength),
				_encryptedBuffer,
				_config.minAccessLevel,
				_config.encryptionType);

		if (retCode != ERR_SUCCESS) {
			LOGe("Failed to encrypt data");
			_encryptedValueLength = 0;
			setGattValue();
			return retCode;
		}
		else {
			_encryptedValueLength = ConnectionEncryption::getEncryptedBufferSize(_valueLength, _config.encryptionType);
		}

		_log(LogLevelCharacteristicVerbose, false, "  encrypted: length=%u data=", _encryptedValueLength);
		_logArray(LogLevelCharacteristicVerbose, true, getGattValue(), getGattValueLength());
	}

	cs_ret_code_t retCode = setGattValue();
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	if (_config.autoNotify) {
		_notificationPending = false;
		_notificationOffset  = 0;

		// Ignore result.
		notify();
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::setGattValue() {
	// Update the value in the softdevice.
	// As of softdevice 7.0.0 we can use a null pointer as value to avoid a memcpy.
	// See https://devzone.nordicsemi.com/f/nordic-q-a/934/how-to-use-ble_gatts_vloc_user.
	ble_gatts_value_t gattsValue;
	gattsValue.len     = getGattValueLength();
	gattsValue.offset  = 0;
	gattsValue.p_value = nullptr;

	uint32_t nrfCode =
			sd_ble_gatts_value_set(_service->getStack()->getConnectionHandle(), _handles.value_handle, &gattsValue);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			break;
		}
		case BLE_ERROR_INVALID_CONN_HANDLE:
		case NRF_ERROR_INVALID_ADDR:
		case NRF_ERROR_INVALID_PARAM:
		case NRF_ERROR_NOT_FOUND:
		case NRF_ERROR_FORBIDDEN:
		case NRF_ERROR_DATA_SIZE:
		default: {
			LOGe("Failed to set GATT value: nrfCode=%u", nrfCode);
			return ERR_UNSPECIFIED;
		}
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CharacteristicBase::notify(uint16_t length, uint16_t offset) {
	if (!_config.notify) {
		return ERR_WRONG_STATE;
	}
	if (!isSubscribedForNotifications()) {
		return ERR_WRONG_STATE;
	}
	if (!_service->getStack()->isConnectedPeripheral()) {
		return ERR_WRONG_STATE;
	}

	if (_config.notificationChunker) {
		// This ignores the length and offset arguments.
		return notifyMultipart();
	}

	uint16_t gattValueLength = getGattValueLength();

	if (offset >= gattValueLength) {
		return ERR_WRONG_PARAMETER;
	}

	uint16_t notificationLength = std::min(gattValueLength, length);

	ble_gatts_hvx_params_t hvx_params;
	hvx_params.handle = _handles.value_handle;
	hvx_params.type   = _subscribedForNotifications ? BLE_GATT_HVX_NOTIFICATION : BLE_GATT_HVX_INDICATION;
	hvx_params.offset = 0;
	hvx_params.p_len  = &notificationLength;
	hvx_params.p_data = nullptr;

	_log(LogLevelCharacteristicVerbose, false, "notify size=%u data=", *hvx_params.p_len);
	_logArray(LogLevelCharacteristicVerbose, true, hvx_params.p_data, *hvx_params.p_len);
	uint32_t nrfCode = sd_ble_gatts_hvx(_service->getStack()->getConnectionHandle(), &hvx_params);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			_notificationPending = false;
			break;
		}
		case NRF_ERROR_RESOURCES: {
			// NRF: Too many notifications queued. Wait for a @ref BLE_GATTS_EVT_HVN_TX_COMPLETE event and retry.
			// Mark that there is a pending notification, we can retry later.
			_notificationPending = true;
			break;
		}
		case NRF_ERROR_TIMEOUT:
		case NRF_ERROR_INVALID_STATE:
		case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
		default: {
			_notificationPending = false;
			LOGe("Failed to notify: nrfCode=%u", nrfCode);
			return ERR_UNSPECIFIED;
		}
	}

	LOGCharacteristicVerbose("Actual number of bytes notified=%u", notificationLength);

	return ERR_SUCCESS;
}

/**
 * Because the softdevice overwrites the gatt value buffer with the notification,
 * we have to restore a part of the gatt value buffer.
 * Example:
 *   Value is:               [100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115]
 *   Size of notification.data = 6
 *   First notification is:  [  0, 100, 101, 102, 103, 104, 105]
 *   The value then becomes: [  0, 100, 101, 102, 103, 104, 105, 107, 108, 109, 110, 111, 112, 113, 114, 115]
 *   Note that 106 is missing.
 *   Second notification is: [  1, 105, 107, 108, 109, 110, 111]
 *   The value then becomes: [  1, 105, 107, 108, 109, 110, 111, 107, 108, 109, 110, 111, 112, 113, 114, 115]
 *   Last notification is:   [255, 112, 113, 114, 115]
 *   The value then becomes: [  1, 105, 107, 108, 109, 110, 111, 107, 108, 109, 110, 111, 112, 113, 114, 115]
 * So to get the correct notifications, we only have to copy 106, and place it back after the first notification.
 * To keep the correct value, we have to copy the first 7 bytes and place that back.
 * For now, we assume that if the user subscribed for notifications, the value won't be read,
 * so we dont need to fix the whole value.
 */
cs_ret_code_t CharacteristicBase::notifyMultipart() {
	uint16_t gattValueLength = getGattValueLength();

	ble_gatts_hvx_params_t hvx_params;
	hvx_params.handle = _handles.value_handle;
	hvx_params.type   = _subscribedForNotifications ? BLE_GATT_HVX_NOTIFICATION : BLE_GATT_HVX_INDICATION;
	hvx_params.offset = 0;

	_log(LogLevelCharacteristicVerbose, false, "GATT value before notify:");
	_logArray(LogLevelCharacteristicVerbose, true, getGattValue(), getGattValueLength());

	chunked_notification_t notification;

	uint8_t originalValue[sizeof(notification.partNr)];
	if (gattValueLength > sizeof(notification.data)) {
		memcpy(originalValue, getGattValue() + sizeof(notification.data), sizeof(originalValue));
	}

	// We can queue multiple notifications.
	while (_notificationOffset < gattValueLength) {

		uint16_t chunkSize = sizeof(notification.data);
		if (gattValueLength - _notificationOffset > chunkSize) {
			notification.partNr = _notificationOffset / chunkSize;
		}
		else {
			// Last chunk, recalculate chunk size, as it might be smaller.
			notification.partNr = CS_CHARACTERISTIC_NOTIFICATION_PART_LAST;
			chunkSize           = gattValueLength - _notificationOffset;
		}

		memcpy(notification.data, getGattValue() + _notificationOffset, chunkSize);

		uint16_t notificationLength = sizeof(notification.partNr) + chunkSize;

		hvx_params.p_len            = &notificationLength;
		hvx_params.p_data           = reinterpret_cast<uint8_t*>(&notification);

		// This call will write the notification buffer to the gatt value buffer.
		_log(LogLevelCharacteristicVerbose, false, "notify size=%u data=", *hvx_params.p_len);
		_logArray(LogLevelCharacteristicVerbose, true, hvx_params.p_data, *hvx_params.p_len);
		uint32_t nrfCode = sd_ble_gatts_hvx(_service->getStack()->getConnectionHandle(), &hvx_params);

		switch (nrfCode) {
			case NRF_SUCCESS: {
				// Notification is queued.
				_notificationOffset += chunkSize;
				break;
			}
			case NRF_ERROR_RESOURCES: {
				// NRF: Too many notifications queued. Wait for a @ref BLE_GATTS_EVT_HVN_TX_COMPLETE event and retry.
				// Mark that there is a pending notification, we can retry later.
				_notificationPending = true;
				return ERR_SUCCESS;
			}
			case NRF_ERROR_TIMEOUT:
			case NRF_ERROR_INVALID_STATE:
			case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
			default: {
				// Reset the notification state, we can't retry later.
				_notificationOffset  = 0;
				_notificationPending = false;
				LOGe("Failed to notify: nrfCode=%u", nrfCode);
				return ERR_UNSPECIFIED;
			}
		}

		_log(LogLevelCharacteristicVerbose, false, "GATT value after notify: ");
		_logArray(LogLevelCharacteristicVerbose, true, getGattValue(), getGattValueLength());

		if (notification.partNr == 0) {
			// Restore the value.
			memcpy(getGattValue() + sizeof(notification.data), originalValue, sizeof(originalValue));
			_log(LogLevelCharacteristicVerbose, false, "GATT value after restore:");
			_logArray(LogLevelCharacteristicVerbose, true, getGattValue(), getGattValueLength());
		}

		LOGCharacteristicVerbose("Actual number of bytes notified=%u", notificationLength);
	}

	_notificationOffset  = 0;
	_notificationPending = false;
	return ERR_SUCCESS;
}

void CharacteristicBase::onNotificationDone() {
	// if we have a notification pending, try to send it
	if (_notificationPending) {
		notify();
	}
	if (_callback) {
		_callback(CHARACTERISTIC_EVENT_NOTIFY_DONE, this, NOT_SET);
	}
}

void CharacteristicBase::onCccdWrite(const uint8_t* data, uint16_t size) {
	LOGCharacteristicVerbose("onCccdWrite [%s] size=%u", _name, size);
	if (size != 2) {
		return;
	}

	bool wasSubscribed          = _subscribedForNotifications;
	_subscribedForNotifications = ble_srv_is_notification_enabled(data);
	if (_callback && _subscribedForNotifications != wasSubscribed) {
		_callback(CHARACTERISTIC_EVENT_SUBSCRIPTION, this, NOT_SET);
	}

	wasSubscribed             = _subscribedForIndications;
	_subscribedForIndications = ble_srv_is_indication_enabled(data);
	if (_callback && _subscribedForIndications != wasSubscribed) {
		_callback(CHARACTERISTIC_EVENT_SUBSCRIPTION, this, NOT_SET);
	}
}

void CharacteristicBase::onConnect() {}

void CharacteristicBase::onDisconnect() {
	LOGCharacteristicVerbose("onDisconnect [%s]", _name);

	// In principle, we can get the CCCD value with sd_ble_gatts_value_get().
	// However, this value doesn't seem to be updated yet onCccdWrite, and I'm not sure if it gets updated
	// on disconnect.
	// So we'll have to keep up for each connection whether they subscribed for notifications.
	_subscribedForNotifications = false;
	_subscribedForIndications   = false;
}

void CharacteristicBase::onWrite(uint16_t length) {
	LOGCharacteristicDebug("onWrite [%s] length=%u", _name, length);

	EncryptionAccessLevel accessLevel = NOT_SET;

	if (isEncrypted()) {
		uint16_t plainTextSize = ConnectionEncryption::getPlaintextBufferSize(length, _config.encryptionType);
		_encryptedValueLength  = length;
		_valueLength           = plainTextSize;

		_log(LogLevelCharacteristicVerbose,
			 false,
			 "Encrypted valuePtr=%p valueLen=%u data=",
			 _encryptedBuffer.data,
			 _encryptedValueLength);
		_logArray(LogLevelCharacteristicVerbose, true, _encryptedBuffer.data, _encryptedValueLength);

		cs_ret_code_t retCode = ConnectionEncryption::getInstance().decrypt(
				cs_data_t(_encryptedBuffer.data, _encryptedValueLength), _buffer, accessLevel, _config.encryptionType);

		if (retCode != ERR_SUCCESS) {
			LOGCharacteristicInfo("Failed to decrypt retCode=%u", retCode);
			ConnectionEncryption::getInstance().disconnect();
			return;
		}

		if (!KeysAndAccess::getInstance().allowAccess(_config.minAccessLevel, accessLevel)) {
			LOGCharacteristicInfo("Insufficient access");
			ConnectionEncryption::getInstance().disconnect();
			return;
		}
	}
	else {
		_valueLength = length;
		accessLevel  = ENCRYPTION_DISABLED;
	}

	_log(LogLevelCharacteristicVerbose, false, "valuePtr=%p valueLen=%u data=", _buffer.data, _valueLength);
	_logArray(LogLevelCharacteristicVerbose, true, _buffer.data, _valueLength);

	if (_callback) {
		_callback(CHARACTERISTIC_EVENT_WRITE, this, accessLevel);
	}
}

cs_data_t CharacteristicBase::getValue() {
	return cs_data_t(_buffer.data, _valueLength);
}

uint16_t CharacteristicBase::getValueHandle() {
	return _handles.value_handle;
}

uint16_t CharacteristicBase::getCccdHandle() {
	return _handles.cccd_handle;
}

bool CharacteristicBase::isSubscribedForNotifications() {
	return _subscribedForNotifications || _subscribedForIndications;
}

uint16_t CharacteristicBase::getGattValueMaxLength() {
	if (isEncrypted()) {
		return _encryptedBuffer.len;
	}
	return _buffer.len;
}

uint16_t CharacteristicBase::getGattValueLength() {
	if (isEncrypted()) {
		return _encryptedValueLength;
	}
	return _valueLength;
}

uint8_t* CharacteristicBase::getGattValue() {
	if (isEncrypted()) {
		return _encryptedBuffer.data;
	}
	return _buffer.data;
}

bool CharacteristicBase::isEncrypted() {
	return (_config.encrypted && _config.minAccessLevel < ENCRYPTION_DISABLED);
}
