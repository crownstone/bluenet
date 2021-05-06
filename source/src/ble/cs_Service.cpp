/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <algorithm>
#include <ble/cs_Service.h>
#include <logging/cs_Logger.h>

///! Service //////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* Service::defaultServiceName = "unnamed";

/** Base class for a BLE service
 */
Service::Service():
		_stack(NULL),
		_name(""),
		_service_handle(BLE_CONN_HANDLE_INVALID)
		{
}

/** Initialize service.
 *
 * This initializes each characteristic. A pointer to the stack is stored locally. By default characteristics are not
 * encrypted. If required, setAesEncrypted() should be called next. This will encrypt all characteristics in this
 * service.
 *
 */
void Service::init(Stack* stack) {
	if (isInitialized(C_SERVICE_INITIALIZED)) return;

	_stack = stack;

	uint32_t err_code;

	const ble_uuid_t& uuid = _uuid.getUuid();

	_service_handle = BLE_CONN_HANDLE_INVALID;
	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, (uint16_t*) &_service_handle);
	APP_ERROR_CHECK(err_code);

	for (CharacteristicBase* characteristic : getCharacteristics()) {
		characteristic->init(this);
	}

	setInitialized(C_SERVICE_INITIALIZED);
}

/** Set encryption.
 *
 * For encryption AES is used. A symmetric cipher. In a service every characteristic is encrypted in the same way.
 * The encryption has to be done by the characteristic itself.
 */
void Service::setAesEncrypted(bool encrypted) {
	// set all characteristics to encrypted
	LOGd("Enable AES encryption for %s", _name.c_str());
	for (CharacteristicBase* characteristic : getCharacteristics()) {
		characteristic->setAesEncrypted(encrypted);
	}
}

/**
 * Set the UUID. It is only possible to do before starting the service. Nordic does not support removing a service.
 * There is only a sd_ble_gatts_service_changed call. If the UUID needs changed in the mean-time, restart the
 * SoftDevice. Calling setUUID when running will only evoke an error. It will have no consequences.
 */
Service& Service::setUUID(const UUID& uuid) {
	if (isInitialized(C_SERVICE_INITIALIZED)) {
		LOGw("Already started. Needs SoftDevice reset.");
	}
	_uuid = uuid;
	return *this;
}

/**
 * A service can receive a BLE event. Currently we pass the connection events through as well as the write event.
 * The latter is wired through on_write() which we pass the evt.gatts_evt.params.write part of the event.
 */
void Service::on_ble_event(const ble_evt_t * p_ble_evt) {
	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		on_connect(p_ble_evt->evt.gap_evt.conn_handle, p_ble_evt->evt.gap_evt.params.connected);
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		on_disconnect(p_ble_evt->evt.gap_evt.conn_handle, p_ble_evt->evt.gap_evt.params.disconnected);
		break;

	case BLE_GATTS_EVT_WRITE:
		on_write(p_ble_evt->evt.gatts_evt.params.write, p_ble_evt->evt.gatts_evt.params.write.handle);
		break;

	default:
		break;
	}
}

/** On connect event for an individual service
 *
 * An individual service normally doesn't respond to a connect event. However, it can be used to for example allocate
 * memory only when a user is connected.
 */
void Service::on_connect(uint16_t conn_handle, const ble_gap_evt_connected_t& gap_evt) {
	// nothing here yet.
}

/** On disconnect event for an individual service
 *
 * Just as on_connect this method can be used to for example deallocate structures that only need to exist when a user
 * is connected.
 */
void Service::on_disconnect(uint16_t conn_handle, const ble_gap_evt_disconnected_t& gap_evt) {
	// nothing here yet.
}

/** Write incoming value to data structures on the device.
 *
 * On an incoming write event we go through the characteristics that belong to this service and compare a handle
 * to see which one we have to write. Subsequently, characteristic->written will be called with length, offset, and
 * the data itself. Depending on the characteristic this can be retrieved as a string or any kind of data type.
 * There is currently no exception handling when the characteristic cannot be found.
 *
 * Note that the write can also be not the value, but the flag to enable or disable notification. In this case
 * write_evt.handle is compared instead of write_evt.context.value_handle. Of course, the corresponding handle in
 * the characteristic object is also different.
 */
bool Service::on_write(const ble_gatts_evt_write_t& write_evt, uint16_t value_handle) {

	for (CharacteristicBase* characteristic : getCharacteristics()) {

		if (characteristic->getCccdHandle() == write_evt.handle && write_evt.len == 2) {
			// received write to enable/disable notification
			characteristic->setNotifyingEnabled(ble_srv_is_notification_enabled(write_evt.data));
			return true;

		} else if (characteristic->getValueHandle() == value_handle) {

			if (write_evt.op == BLE_GATTS_OP_WRITE_REQ
					|| write_evt.op == BLE_GATTS_OP_WRITE_CMD
					|| write_evt.op == BLE_GATTS_OP_SIGN_WRITE_CMD) {

				characteristic->written(write_evt.len);

			} else if (write_evt.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {

				// get length of data, header does not contain full length but rather the "step size"
				ble_gatts_value_t gatts_value;
				gatts_value.len = 0;
				gatts_value.offset = 0;
				gatts_value.p_value = NULL;
				
				uint32_t err_code;
				err_code = sd_ble_gatts_value_get(
						getStack()->getConnectionHandle(),
						characteristic->getValueHandle(),
						&gatts_value
				);
				APP_ERROR_CHECK(err_code);

				characteristic->written(gatts_value.len);

				//uint16_t length = 0;
				//cs_sd_ble_gatts_value_get(getStack()->getConnectionHandle(), characteristic->getValueHandle(),
				//		&length, NULL);
				//characteristic->written(length);
			}

			return true;
		}
	}
	return false;
}

/** Propagate BLE event to characteristics.
 *
 * Inform all characteristics that transmission was completed in case they have notifications pending.
 */
void Service::onTxComplete(const ble_common_evt_t * p_ble_evt) {
	for (CharacteristicBase* characteristic : getCharacteristics()) {
		characteristic->onTxComplete(p_ble_evt);
	}
}

