/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Service.h>
#include <logging/cs_Logger.h>

#include <algorithm>

Service::Service() {}

/**
 * Initialize the service.
 *
 * This initializes each characteristic. A pointer to the stack is stored locally. By default characteristics are not
 * encrypted. If required, setAesEncrypted() should be called next. This will encrypt all characteristics in this
 * service.
 *
 */
void Service::init(Stack* stack) {
	if (isInitialized(C_SERVICE_INITIALIZED)) {
		return;
	}

	_stack                 = stack;

	const ble_uuid_t& uuid = _uuid.getUuid();

	_handle                = BLE_CONN_HANDLE_INVALID;
	uint32_t nrfCode       = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, (uint16_t*)&_handle);
	switch (nrfCode) {
		case NRF_SUCCESS:
			// * @retval ::NRF_SUCCESS Successfully added a service declaration.
			break;
		case NRF_ERROR_INVALID_ADDR:
			// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
			// This shouldn't happen: crash.
		case NRF_ERROR_INVALID_PARAM:
			// * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied, Vendor Specific UUIDs need to be
			// present in the table. This shouldn't happen: crash.
		case NRF_ERROR_FORBIDDEN:
			// * @retval ::NRF_ERROR_FORBIDDEN Forbidden value supplied, certain UUIDs are reserved for the stack.
			// This shouldn't happen: crash.
		case NRF_ERROR_NO_MEM:
			// * @retval ::NRF_ERROR_NO_MEM Not enough memory to complete operation.
			// This shouldn't happen: crash.
		default:
			// Crash
			APP_ERROR_HANDLER(nrfCode);
	}

	for (CharacteristicBase* characteristic : _characteristics) {
		characteristic->init(this);
	}

	setInitialized(C_SERVICE_INITIALIZED);
}

void Service::addCharacteristic(CharacteristicBase* characteristic) {
	_characteristics.push_back(characteristic);

	if (isInitialized(C_SERVICE_INITIALIZED)) {
		characteristic->init(this);
	}
}

/** Set encryption.
 *
 * For encryption AES is used. A symmetric cipher. In a service every characteristic is encrypted in the same way.
 * The encryption has to be done by the characteristic itself.
 */
void Service::setAesEncrypted(bool encrypted) {
	// set all characteristics to encrypted
	LOGd("Enable AES encryption");
	for (CharacteristicBase* characteristic : _characteristics) {
		characteristic->setAesEncrypted(encrypted);
	}
}

/**
 * Set the UUID. It is only possible to do before starting the service. Nordic does not support removing a service.
 * There is only a sd_ble_gatts_service_changed call. If the UUID needs changed in the mean-time, restart the
 * SoftDevice. Calling setUUID when running will only evoke an error. It will have no consequences.
 */
void Service::setUUID(const UUID& uuid) {
	if (isInitialized(C_SERVICE_INITIALIZED)) {
		LOGw("Already started. Needs SoftDevice reset.");
	}
	_uuid = uuid;
}

/**
 * A service can receive a BLE event. Currently we pass the connection events through as well as the write event.
 * The latter is wired through on_write() which we pass the evt.gatts_evt.params.write part of the event.
 */
void Service::onBleEvent(const ble_evt_t* event) {
	switch (event->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			onConnect(event->evt.gap_evt.conn_handle, event->evt.gap_evt.params.connected);
			break;

		case BLE_GAP_EVT_DISCONNECTED:
			onDisconnect(event->evt.gap_evt.conn_handle, event->evt.gap_evt.params.disconnected);
			break;

		case BLE_GATTS_EVT_WRITE:
			onWrite(event->evt.gatts_evt.params.write, event->evt.gatts_evt.params.write.handle);
			break;

		default: break;
	}
}

/** On connect event for an individual service
 *
 * An individual service normally doesn't respond to a connect event. However, it can be used to for example allocate
 * memory only when a user is connected.
 */
void Service::onConnect(
		[[maybe_unused]] uint16_t connectionHandle, [[maybe_unused]] const ble_gap_evt_connected_t& event) {
	// nothing here yet.
}

/** On disconnect event for an individual service
 *
 * Just as on_connect this method can be used to for example deallocate structures that only need to exist when a user
 * is connected.
 */
void Service::onDisconnect(
		[[maybe_unused]] uint16_t connectionHandle, [[maybe_unused]] const ble_gap_evt_disconnected_t& event) {
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
bool Service::onWrite(const ble_gatts_evt_write_t& event, uint16_t gattHandle) {

	for (CharacteristicBase* characteristic : _characteristics) {

		if (characteristic->getCccdHandle() == event.handle && event.len == 2) {
			// received write to enable/disable notification
			characteristic->setNotifyingEnabled(ble_srv_is_notification_enabled(event.data));
			return true;
		}
		else if (characteristic->getValueHandle() == gattHandle) {

			if (event.op == BLE_GATTS_OP_WRITE_REQ || event.op == BLE_GATTS_OP_WRITE_CMD
				|| event.op == BLE_GATTS_OP_SIGN_WRITE_CMD) {

				characteristic->onWrite(event.len);
			}
			else if (event.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {

				// get length of data, header does not contain full length but rather the "step size"
				ble_gatts_value_t gattValue;
				gattValue.len     = 0;
				gattValue.offset  = 0;
				gattValue.p_value = NULL;

				uint32_t nrfCode  = sd_ble_gatts_value_get(
                        getStack()->getConnectionHandle(), characteristic->getValueHandle(), &gattValue);
				switch (nrfCode) {
					case NRF_SUCCESS:
						// * @retval ::NRF_SUCCESS Successfully retrieved the value of the attribute.
						break;
					case BLE_ERROR_INVALID_CONN_HANDLE:
						// * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied on a system
						// attribute. This shouldn't happen, as the connection handle is only set in the main thread.
						LOGe("Invalid handle");
						break;
					case NRF_ERROR_INVALID_ADDR:
						// * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
						// This shouldn't happen: crash.
					case NRF_ERROR_NOT_FOUND:
						// * @retval ::NRF_ERROR_NOT_FOUND Attribute not found.
						// This shouldn't happen: crash.
					case NRF_ERROR_INVALID_PARAM:
						// * @retval ::NRF_ERROR_INVALID_PARAM Invalid attribute offset supplied.
						// This shouldn't happen: crash.
					case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
						// * @retval ::BLE_ERROR_GATTS_SYS_ATTR_MISSING System attributes missing, use @ref
						// sd_ble_gatts_sys_attr_set to set them to a known value. This shouldn't happen: crash.
					default:
						// Crash
						APP_ERROR_HANDLER(nrfCode);
				}

				characteristic->onWrite(gattValue.len);
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
void Service::onTxComplete(const ble_common_evt_t* event) {
	for (CharacteristicBase* characteristic : _characteristics) {
		characteristic->onTxComplete(event);
	}
}
