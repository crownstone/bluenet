/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_BleCentral.h>
#include <ble/cs_Nordic.h>
#include <ble/cs_Stack.h>
#include <common/cs_Types.h>
#include <events/cs_Event.h>
#include <events/cs_EventDispatcher.h>
#include <logging/cs_Logger.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_EncryptionBuffer.h>

#define LOGBleCentralInfo LOGi
#define LOGBleCentralDebug LOGvv
#define LogLevelBleCentralDebug SERIAL_VERY_VERBOSE

const uint16_t WRITE_OVERHEAD = 3;
const uint16_t LONG_WRITE_OVERHEAD = 5;

void handle_discovery(ble_db_discovery_evt_t* event) {
	// No need for null pointer check.
	BleCentral::getInstance().onDiscoveryEvent(*event);
}

BleCentral::BleCentral() {
}

void BleCentral::init() {
	_discoveryModule.discovery_in_progress = false;
	_discoveryModule.discovery_pending = false;
	_discoveryModule.conn_handle = BLE_CONN_HANDLE_INVALID;
	uint32_t nrfCode = ble_db_discovery_init(handle_discovery);
	APP_ERROR_CHECK(nrfCode);

	// Use the encryption buffer, as that contains the encrypted data, which is what we usually write or read.
	EncryptionBuffer::getInstance().getBuffer(_buf.data, _buf.len);

	State::getInstance().get(CS_TYPE::CONFIG_SCAN_INTERVAL_625US, &_scanInterval, sizeof(_scanInterval));
	State::getInstance().get(CS_TYPE::CONFIG_SCAN_WINDOW_625US, &_scanWindow, sizeof(_scanWindow));

	listen();
}

cs_ret_code_t BleCentral::connect(const device_address_t& address, uint16_t timeoutMs) {
	if (isBusy()) {
		LOGBleCentralInfo("Busy");
		return ERR_BUSY;
	}

	if (Stack::getInstance().isConnected() || isConnected()) {
		LOGBleCentralInfo("Already connected");
		return ERR_WRONG_STATE;
	}

	// In order to connect, other modules need to change their operation.
	event_t connectEvent(CS_TYPE::EVT_BLE_CENTRAL_CONNECT_START);
	connectEvent.dispatch();
	if (connectEvent.result.returnCode != ERR_SUCCESS) {
		LOGe("Unable to start connecting: err=%u", connectEvent.result.returnCode);
		// Let modules know they can resume their normal operation.
		event_t disconnectEvent(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED);
		disconnectEvent.dispatch();
		return connectEvent.result.returnCode;
	}

	ble_gap_addr_t gapAddress;
	memcpy(gapAddress.addr, address.address, sizeof(address.address));
	gapAddress.addr_id_peer = 0;
	gapAddress.addr_type = address.addressType;

	// The soft device has to scan for the address in order to connect, so we need to set the scan parameters.
	ble_gap_scan_params_t scanParams;
	scanParams.extended = 0;
	scanParams.report_incomplete_evts = 0;
	scanParams.active = 1;
	scanParams.filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL; // Scanning filter policy. See BLE_GAP_SCAN_FILTER_POLICIES
	scanParams.scan_phys = BLE_GAP_PHY_1MBPS;
	scanParams.timeout = timeoutMs / 10; // This acts as connection timeout.
	scanParams.channel_mask[0] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[1] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[2] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[3] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.channel_mask[4] = 0; // See ble_gap_ch_mask_t and sd_ble_gap_scan_start
	scanParams.interval = _scanInterval;
	scanParams.window = _scanWindow;

	if (scanParams.timeout < BLE_GAP_SCAN_TIMEOUT_MIN) {
		scanParams.timeout = BLE_GAP_SCAN_TIMEOUT_MIN;
	}

	ble_gap_conn_params_t connectionParams;
	connectionParams.min_conn_interval = MIN_CONNECTION_INTERVAL;
	connectionParams.max_conn_interval = MAX_CONNECTION_INTERVAL;
	connectionParams.slave_latency = SLAVE_LATENCY;
	connectionParams.conn_sup_timeout = CONNECTION_SUPERVISION_TIMEOUT;

	uint32_t nrfCode = sd_ble_gap_connect(&gapAddress, &scanParams, &connectionParams, APP_BLE_CONN_CFG_TAG);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Connect err=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}

	LOGBleCentralInfo("Connecting..");

	// We will get either BLE_GAP_EVT_CONNECTED or BLE_GAP_EVT_TIMEOUT.
	_currentOperation = Operation::CONNECT;
	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t BleCentral::disconnect() {
	if (isBusy()) {
		LOGBleCentralInfo("Cancel current operation");
		finalizeOperation(_currentOperation, ERR_CANCELED);
	}

	if (!isConnected()) {
		LOGBleCentralInfo("Already disconnected");
		return ERR_SUCCESS;
	}

	// NRF_ERROR_INVALID_STATE can safely be ignored, see: https://devzone.nordicsemi.com/question/81108/handling-nrf_error_invalid_state-error-code/
	// BLE_ERROR_INVALID_CONN_HANDLE can safely be ignored, see: https://devzone.nordicsemi.com/f/nordic-q-a/34353/error-0x3002/132078#132078
	uint32_t nrfCode = sd_ble_gap_disconnect(_connectionHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			LOGBleCentralInfo("Disconnecting..");
			// We will get BLE_GAP_EVT_DISCONNECTED.
			_currentOperation = Operation::DISCONNECT;
			return ERR_WAIT_FOR_SUCCESS;
		}
		case NRF_ERROR_INVALID_STATE: {
			LOGw("Disconnect error: invalid state. Assume already disconnected.");
			return ERR_SUCCESS;
		}
		case BLE_ERROR_INVALID_CONN_HANDLE: {
			// Assume already disconnected.
			LOGw("Disconnect error: invalid connection handle. Assume already disconnected.");
			return ERR_SUCCESS;
		}
		default: {
			LOGe("Disconnect err=%u", nrfCode);
			return ERR_UNSPECIFIED;
		}
	}
}

bool BleCentral::isConnected() {
	return _connectionHandle != BLE_CONN_HANDLE_INVALID;
}

bool BleCentral::isBusy() {
	return _currentOperation != Operation::NONE;
}

cs_ret_code_t BleCentral::discoverServices(const UUID* uuids, uint8_t uuidCount) {
	if (isBusy()) {
		LOGBleCentralInfo("Busy");
		return ERR_BUSY;
	}
	if (!isConnected()) {
		LOGBleCentralInfo("Not connected");
		return ERR_WRONG_STATE;
	}

	uint32_t nrfCode = NRF_SUCCESS;
	for (uint8_t i = 0; i < uuidCount; ++i) {
		ble_uuid_t uuid = uuids[i].getUuid();
		nrfCode = ble_db_discovery_evt_register(&uuid);
		switch (nrfCode) {
			case NRF_SUCCESS:
				break;
			case NRF_ERROR_NO_MEM: {
				return ERR_NO_SPACE;
			}
			default:
				return ERR_UNSPECIFIED;
		}
	}

	nrfCode = ble_db_discovery_start(&_discoveryModule, _connectionHandle);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Failed to start discovery retCode=%u", nrfCode);
		// Bart 19-04-2021 TODO: Should we disconnect on errors?
		return ERR_UNSPECIFIED;
	}

	LOGBleCentralInfo("Discovering..");
	_currentOperation = Operation::DISCOVERY;
	return ERR_WAIT_FOR_SUCCESS;
}

void BleCentral::onDiscoveryEvent(ble_db_discovery_evt_t& event) {
	switch (event.evt_type) {
		case BLE_DB_DISCOVERY_COMPLETE: {
			LOGBleCentralDebug("Discovery found service uuid=0x%04X type=%u characteristicCount=%u", event.params.discovered_db.srv_uuid.uuid, event.params.discovered_db.srv_uuid.type, event.params.discovered_db.char_count);

			// Send an event for the service
			ble_central_discovery_t packet = {
				.uuid = UUID(event.params.discovered_db.srv_uuid),
				.valueHandle = BLE_GATT_HANDLE_INVALID,
				.cccdHandle = BLE_GATT_HANDLE_INVALID
			};
			event_t eventOut(CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY, &packet, sizeof(packet));
			eventOut.dispatch();

			// Send an event for each characteristic
			for (uint8_t i = 0; i < event.params.discovered_db.char_count; ++i) {
				packet.uuid = UUID(event.params.discovered_db.charateristics[i].characteristic.uuid);
				packet.valueHandle = event.params.discovered_db.charateristics[i].characteristic.handle_value;
				packet.cccdHandle = event.params.discovered_db.charateristics[i].cccd_handle;
				event_t eventOut(CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY, &packet, sizeof(packet));
				eventOut.dispatch();
			}

			break;
		}
		case BLE_DB_DISCOVERY_SRV_NOT_FOUND: {
			LOGBleCentralDebug("Discovery did not find service uuid=0x%04X type=%u", event.params.discovered_db.srv_uuid.uuid, event.params.discovered_db.srv_uuid.type);
			break;
		}
		case BLE_DB_DISCOVERY_ERROR: {
			LOGw("Discovery error %u", event.params.err_code);
			finalizeOperation(Operation::DISCOVERY, ERR_UNSPECIFIED);
			break;
		}
		case BLE_DB_DISCOVERY_AVAILABLE: {
			// A bug prevents this event from ever firing. It is fixed in SDK 16.0.0.
			// Instead, we should be done when the number of registered uuids equals the number of received BLE_DB_DISCOVERY_COMPLETE + BLE_DB_DISCOVERY_SRV_NOT_FOUND events.
			// See https://devzone.nordicsemi.com/f/nordic-q-a/20846/getting-service-count-from-database-discovery-module
			// We apply a similar patch to the SDK.
			LOGBleCentralInfo("Discovery done");

			// According to the doc, the "services" struct is only for internal usage.
			// But it seems to store all the discovered services and characteristics.
			// Also contains the services that have not been found.
			for (uint8_t s = 0; s < _discoveryModule.discoveries_count; ++s) {
				LOGBleCentralDebug("service: uuidType=%u uuid=0x%04X", _discoveryModule.services[s].srv_uuid.type, _discoveryModule.services[s].srv_uuid.uuid);
				for (uint8_t c = 0; c < _discoveryModule.services[s].char_count; ++c) {
					__attribute__((unused)) ble_gatt_db_char_t* characteristic = &(_discoveryModule.services[s].charateristics[c]);
					LOGBleCentralDebug("    char: cccd_handle=0x%X uuidType=%u uuid=0x%04X handle_value=0x%X handle_decl=0x%X",
							characteristic->cccd_handle,
							characteristic->characteristic.uuid.type,
							characteristic->characteristic.uuid.uuid,
							characteristic->characteristic.handle_value,
							characteristic->characteristic.handle_decl);
				}
			}
			finalizeOperation(Operation::DISCOVERY, ERR_SUCCESS);
			break;
		}
	}
}

cs_data_t BleCentral::requestWriteBuffer() {
	if (isBusy()) {
		return cs_data_t();
	}
	uint16_t maxWriteSize = 256;
	return cs_data_t(_buf.data, std::min(_buf.len, maxWriteSize));
}

cs_ret_code_t BleCentral::write(uint16_t handle, const uint8_t* data, uint16_t len) {
	if (isBusy()) {
		LOGBleCentralInfo("Busy");
		return ERR_BUSY;
	}

	if (!isConnected()) {
		LOGBleCentralInfo("Not connected");
		return ERR_WRONG_STATE;
	}

	if (handle == BLE_GATT_HANDLE_INVALID) {
		LOGBleCentralInfo("Invalid handle");
		return ERR_WRONG_PARAMETER;
	}

	if (data == nullptr) {
		return ERR_BUFFER_UNASSIGNED;
	}

	// Copy the data to the buffer.
	if (len > _buf.len) {
		return ERR_BUFFER_TOO_SMALL;
	}

	// Only copy data if it points to a different buffer.
	if (_buf.data != data) {
		// Use memmove, as it can handle overlapping buffers.
		memmove(_buf.data, data, len);
	}
	else {
		LOGBleCentralDebug("Skip copy");
	}

	if (len > _mtu - WRITE_OVERHEAD) {
		// We need to break up the write into chunks.
		// Although it seems like what we're looking for, the "Queued Write module" is NOT what we can use for this.
		//     https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.2.0/lib_ble_queued_write.html
		//     https://devzone.nordicsemi.com/f/nordic-q-a/43140/queued-write-module
		// Sequence chart of long write:
		//     https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.1.1/group___b_l_e___g_a_t_t_c___v_a_l_u_e___l_o_n_g___w_r_i_t_e___m_s_c.html
		// multiple      BLE_GATT_OP_PREP_WRITE_REQ
		// finalized by  BLE_GATT_OP_EXEC_WRITE_REQ

		_bufDataSize = len;
		// Current operation is set in nextWrite().
		return nextWrite(handle, 0);
	}

	// We can do a single write with response.
	ble_gattc_write_params_t writeParams = {
			.write_op = BLE_GATT_OP_WRITE_REQ, // Write with response (ack). Triggers BLE_GATTC_EVT_WRITE_RSP.
			.flags = 0,
			.handle = handle,
			.offset = 0,
			.len = len,
			.p_value = _buf.data
	};

	uint32_t nrfCode = sd_ble_gattc_write(_connectionHandle, &writeParams);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Failed to write. nrfCode=%u", nrfCode);
		switch (nrfCode) {
			default:                             return ERR_UNSPECIFIED;
		}
	}

	_currentOperation = Operation::WRITE;
	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t BleCentral::nextWrite(uint16_t handle, uint16_t offset) {
	ble_gattc_write_params_t writeParams;
	if (offset < _bufDataSize) {
		const int chunkSize = _mtu - LONG_WRITE_OVERHEAD;
		writeParams.write_op = BLE_GATT_OP_PREP_WRITE_REQ;
		writeParams.flags = 0;
		writeParams.handle = handle;
		writeParams.offset = offset;
		writeParams.len = std::min(chunkSize, _bufDataSize - offset);
		writeParams.p_value = _buf.data + offset;
	}
	else {
		writeParams.write_op = BLE_GATT_OP_EXEC_WRITE_REQ;
		writeParams.flags = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;
		writeParams.handle = handle;
		writeParams.offset = 0;
		writeParams.len = 0;
		writeParams.p_value = nullptr;
	}
	LOGBleCentralDebug("nextWrite offset=%u _bufDataSize=%u writeParams: offset=%u len=%u op=%u", offset, _bufDataSize, writeParams.offset, writeParams.len, writeParams.write_op);

	uint32_t nrfCode = sd_ble_gattc_write(_connectionHandle, &writeParams);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Failed to long write. nrfCode=%u", nrfCode);
		switch (nrfCode) {
			case BLE_ERROR_INVALID_CONN_HANDLE:  return ERR_WRONG_PARAMETER;
			default:                             return ERR_UNSPECIFIED;
		}
	}
	_currentOperation = Operation::WRITE;
	return ERR_WAIT_FOR_SUCCESS;
}



cs_ret_code_t BleCentral::read(uint16_t handle) {
	if (isBusy()) {
		LOGBleCentralInfo("Busy");
		return ERR_BUSY;
	}

	if (!isConnected()) {
		LOGBleCentralInfo("Not connected");
		return ERR_WRONG_STATE;
	}

	if (handle == BLE_GATT_HANDLE_INVALID) {
		LOGBleCentralInfo("Invalid handle");
		return ERR_WRONG_PARAMETER;
	}

	uint32_t nrfCode = sd_ble_gattc_read(_connectionHandle, handle, 0);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Failed to read. nrfCode=%u", nrfCode);
		switch (nrfCode) {
			default:                             return ERR_UNSPECIFIED;
		}
	}

	_currentOperation = Operation::READ;
	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t BleCentral::writeNotificationConfig(uint16_t cccdHandle, bool enable) {
	uint16_t value = enable ? BLE_GATT_HVX_NOTIFICATION : 0;
	return write(cccdHandle, reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

void BleCentral::finalizeOperation(Operation operation, cs_ret_code_t retCode) {
	switch (operation) {
		case Operation::NONE:
		case Operation::CONNECT:
		case Operation::DISCONNECT:
		case Operation::DISCOVERY:
		case Operation::WRITE: {
			finalizeOperation(operation, reinterpret_cast<uint8_t*>(&retCode), sizeof(retCode));
			break;
		}
		case Operation::READ: {
			ble_central_read_result_t result = {
					.retCode = retCode,
					.data = cs_data_t()
			};
			finalizeOperation(operation, reinterpret_cast<uint8_t*>(&result), sizeof(result));
			break;
		}
	}
}

void BleCentral::finalizeOperation(Operation operation, uint8_t* data, uint8_t dataSize) {
	LOGBleCentralDebug("finalizeOperation operation=%u _currentOperation=%u dataSize=%u", operation, _currentOperation, dataSize);
	event_t event(CS_TYPE::CONFIG_DO_NOT_USE);

	// Handle error first.
	if (_currentOperation != operation) {
		switch (_currentOperation) {
			case Operation::NONE: {
				break;
			}
			case Operation::CONNECT: {
				// Ignore data, finalize with error instead.
				TYPIFY(EVT_BLE_CENTRAL_CONNECT_RESULT) result = ERR_WRONG_STATE;
				event_t errEvent(CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT, reinterpret_cast<uint8_t*>(&result), sizeof(result));
				sendOperationResult(errEvent);
				return;
			}
			case Operation::DISCONNECT: {
				// Keep on waiting for the disconnect event.
				break;
			}
			case Operation::DISCOVERY: {
				// Ignore data, finalize with error instead.
				TYPIFY(EVT_BLE_CENTRAL_DISCOVERY_RESULT) result = ERR_WRONG_STATE;
				event_t errEvent(CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT, reinterpret_cast<uint8_t*>(&result), sizeof(result));
				sendOperationResult(errEvent);
				return;
			}
			case Operation::READ: {
				// Ignore data, finalize with error instead.
				TYPIFY(EVT_BLE_CENTRAL_READ_RESULT) result = {
						.retCode = ERR_WRONG_STATE,
						.data = cs_data_t()
				};
				event_t errEvent(CS_TYPE::EVT_BLE_CENTRAL_READ_RESULT, reinterpret_cast<uint8_t*>(&result), sizeof(result));
				sendOperationResult(errEvent);
				return;
			}
			case Operation::WRITE: {
				// Ignore data, finalize with error instead.
				TYPIFY(EVT_BLE_CENTRAL_WRITE_RESULT) result = ERR_WRONG_STATE;
				event_t errEvent(CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT, reinterpret_cast<uint8_t*>(&result), sizeof(result));
				sendOperationResult(errEvent);
				return;
			}
		}
	}

	event.data = data;
	event.size = dataSize;
	switch (_currentOperation) {
		case Operation::NONE: {
			LOGBleCentralDebug("No operation was in progress");
			break;
		}
		case Operation::CONNECT: {
			event.type = CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT;
			break;
		}
		case Operation::DISCONNECT: {
			// Skip: if the finalized operation is disconnect, we always send it.
			break;
		}
		case Operation::DISCOVERY: {
			event.type = CS_TYPE::EVT_BLE_CENTRAL_DISCOVERY_RESULT;
			break;
		}
		case Operation::READ: {
			event.type = CS_TYPE::EVT_BLE_CENTRAL_READ_RESULT;
			break;
		}
		case Operation::WRITE: {
			event.type = CS_TYPE::EVT_BLE_CENTRAL_WRITE_RESULT;
			break;
		}
	}

	sendOperationResult(event);

	if (operation == Operation::DISCONNECT) {
		// Always send the disconnect event, but after the current operation result event.
		LOGBleCentralDebug("Dispatch disconnect event");
		event_t disconnectEvent(CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED);
		disconnectEvent.dispatch();
	}
}

void BleCentral::sendOperationResult(event_t& event) {
	LOGBleCentralDebug("sendOperationResult type=%u size=%u", event.type, event.size);
	// Set current operation before dispatching the event, so that a new command can be issued on event.
	_currentOperation = Operation::NONE;

	if (event.type != CS_TYPE::CONFIG_DO_NOT_USE) {
		event.dispatch();
	}
}




void BleCentral::onConnect(uint16_t connectionHandle, const ble_gap_evt_connected_t& event) {
	if (event.role != BLE_GAP_ROLE_CENTRAL) {
		return;
	}
	LOGBleCentralInfo("Connected");

	_connectionHandle = connectionHandle;

	uint32_t nrfCode = sd_ble_gattc_exchange_mtu_request(_connectionHandle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
	if (nrfCode == NRF_SUCCESS) {
		// Wait for MTU response.
	}
	else {
		LOGw("Failed MTU request nrfCode=%u", nrfCode);
		finalizeOperation(Operation::CONNECT, ERR_SUCCESS);
	}
}

void BleCentral::onGapTimeout(const ble_gap_evt_timeout_t& event) {
	if (event.src != BLE_GAP_TIMEOUT_SRC_CONN) {
		return;
	}
	LOGBleCentralInfo("Connect timeout");

	// Connection failed, reset state.
	_connectionHandle = BLE_CONN_HANDLE_INVALID;
	finalizeOperation(Operation::CONNECT, ERR_TIMEOUT);
}

void BleCentral::onDisconnect(const ble_gap_evt_disconnected_t& event) {
	if (!isConnected()) {
		// Ignore disconnect event as peripheral.
		return;
	}
	// TODO: reconnect on certain errors (for example BLE_HCI_CONN_FAILED_TO_BE_ESTABLISHED).
	LOGBleCentralInfo("Disconnected reason=%u", event.reason);

	// Disconnected, reset state.
	_connectionHandle = BLE_CONN_HANDLE_INVALID;
	finalizeOperation(Operation::DISCONNECT, nullptr, 0);
}

void BleCentral::onMtu(uint16_t gattStatus, const ble_gattc_evt_exchange_mtu_rsp_t& event) {
	if (gattStatus != BLE_GATT_STATUS_SUCCESS) {
		LOGw("gattStatus=%u", gattStatus);
		finalizeOperation(Operation::CONNECT, ERR_GATT_ERROR);
		return;
	}
	_mtu = event.server_rx_mtu;
	LOGBleCentralInfo("MTU=%u", _mtu);
	finalizeOperation(Operation::CONNECT, ERR_SUCCESS);
}

void BleCentral::onRead(uint16_t gattStatus, const ble_gattc_evt_read_rsp_t& event) {
	_log(LogLevelBleCentralDebug, false, "Read offset=%u len=%u data=", event.offset, event.len);
	_logArray(LogLevelBleCentralDebug, true, event.data, event.len);

	// According to https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.1.1/group___b_l_e___g_a_t_t_c___v_a_l_u_e___r_e_a_d___m_s_c.html
	// we should continue reading, until we get a gatt status of invalid offset.
	// But it seems like the last read we get is with a length of 0.
	if (gattStatus == BLE_GATT_STATUS_ATTERR_INVALID_OFFSET) {
		ble_central_read_result_t result = {
				.retCode = ERR_SUCCESS,
				.data = cs_data_t(_buf.data, event.offset)
		};
		finalizeOperation(Operation::READ, reinterpret_cast<uint8_t*>(&result), sizeof(result));
		return;
	}

	if (gattStatus != BLE_GATT_STATUS_SUCCESS) {
		LOGw("gattStatus=%u", gattStatus);
		finalizeOperation(Operation::READ, ERR_GATT_ERROR);
		return;
	}

	if (event.len == 0) {
		ble_central_read_result_t result = {
				.retCode = ERR_SUCCESS,
				.data = cs_data_t(_buf.data, event.offset)
		};
		finalizeOperation(Operation::READ, reinterpret_cast<uint8_t*>(&result), sizeof(result));
		return;
	}

	// Copy data that was read.
	if (event.offset + event.len > _buf.len) {
		finalizeOperation(Operation::READ, ERR_BUFFER_TOO_SMALL);
		return;
	}
	memcpy(_buf.data + event.offset, event.data, event.len);

	// Continue long read.
	uint32_t nrfCode = sd_ble_gattc_read(_connectionHandle, event.handle, event.offset + event.len);
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to continue long read. retCode=%u", nrfCode);
		finalizeOperation(Operation::READ, ERR_READ_FAILED);
		return;
	}
}

void BleCentral::onWrite(uint16_t gattStatus, const ble_gattc_evt_write_rsp_t& event) {
	LOGBleCentralDebug("onWrite write_op=%u offset=%u len=%u", event.write_op, event.offset, event.len);

	if (gattStatus != BLE_GATT_STATUS_SUCCESS) {
		LOGw("gattStatus=%u", gattStatus);
		finalizeOperation(Operation::WRITE, ERR_GATT_ERROR);
		return;
	}

	switch (event.write_op) {
		case BLE_GATT_OP_WRITE_REQ:
		case BLE_GATT_OP_EXEC_WRITE_REQ: {
			finalizeOperation(Operation::WRITE, ERR_SUCCESS);
			break;
		}
		case BLE_GATT_OP_PREP_WRITE_REQ: {
			cs_ret_code_t retCode = nextWrite(event.handle, event.offset + event.len);
			if (retCode != ERR_WAIT_FOR_SUCCESS) {
				finalizeOperation(Operation::WRITE, retCode);
			}
			break;
		}
		default: {
			LOGw("Unhandled write op: %u", event.write_op);
		}
	}
}

void BleCentral::onNotification(uint16_t gattStatus, const ble_gattc_evt_hvx_t& event) {
	_log(LogLevelBleCentralDebug, false, "onNotification handle=%u len=%u data=", event.handle, event.len);
	_logArray(LogLevelBleCentralDebug, true, event.data, event.len);

	if (gattStatus != BLE_GATT_STATUS_SUCCESS) {
		LOGw("gattStatus=%u", gattStatus);
		return;
	}

	// BLE_GATT_HVX_NOTIFICATION are unacknowledged, while BLE_GATT_HVX_INDICATION have to be acknowledged.
	if (event.type != BLE_GATT_HVX_NOTIFICATION) {
		LOGw("unexpected type=%u", event.type);
		return;
	}

	TYPIFY(EVT_BLE_CENTRAL_NOTIFICATION) packet = {
			.handle = event.handle,
			.data = cs_const_data_t(event.data, event.len)
	};
	event_t eventOut(CS_TYPE::EVT_BLE_CENTRAL_NOTIFICATION, &packet, sizeof(packet));
	eventOut.dispatch();
}




void BleCentral::onGapEvent(uint16_t evtId, const ble_gap_evt_t& event) {
	switch (evtId) {
		case BLE_GAP_EVT_CONNECTED: {
			onConnect(event.conn_handle, event.params.connected);
			break;
		}
		case BLE_GAP_EVT_DISCONNECTED: {
			onDisconnect(event.params.disconnected);
			break;
		}
		case BLE_GAP_EVT_TIMEOUT: {
			onGapTimeout(event.params.timeout);
			break;
		}
	}
}

void BleCentral::onGattCentralEvent(uint16_t evtId, const ble_gattc_evt_t& event) {
	if (event.conn_handle != _connectionHandle) {
		return;
	}
	if (event.gatt_status != BLE_GATT_STATUS_SUCCESS) {
		LOGBleCentralDebug("Event id=%u status=%u", evtId, event.gatt_status);
	}
	switch (evtId) {
		case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
			onMtu(event.gatt_status, event.params.exchange_mtu_rsp);
			break;
		}
		case BLE_GATTC_EVT_READ_RSP: {
			onRead(event.gatt_status, event.params.read_rsp);
			break;
		}
		case BLE_GATTC_EVT_WRITE_RSP: {
			onWrite(event.gatt_status, event.params.write_rsp);
			break;
		}
		case BLE_GATTC_EVT_HVX: {
			onNotification(event.gatt_status, event.params.hvx);
			break;
		}
	}
}

void BleCentral::onBleEvent(const ble_evt_t* event) {
	ble_db_discovery_on_ble_evt(event, &_discoveryModule);

	if (BLE_GAP_EVT_BASE <= event->header.evt_id && event->header.evt_id <= BLE_GAP_EVT_LAST) {
		onGapEvent(event->header.evt_id, event->evt.gap_evt);
	}
	else if (BLE_GATTC_EVT_BASE <= event->header.evt_id && event->header.evt_id <= BLE_GATTC_EVT_LAST) {
		onGattCentralEvent(event->header.evt_id, event->evt.gattc_evt);
	}
}

void BleCentral::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::CMD_BLE_CENTRAL_CONNECT: {
			auto packet = CS_TYPE_CAST(CMD_BLE_CENTRAL_CONNECT, event.data);
			event.result.returnCode = connect(packet->address, packet->timeoutMs);
			break;
		}
		case CS_TYPE::CMD_BLE_CENTRAL_DISCONNECT: {
			event.result.returnCode = disconnect();
			break;
		}
		case CS_TYPE::CMD_BLE_CENTRAL_DISCOVER: {
			auto packet = CS_TYPE_CAST(CMD_BLE_CENTRAL_DISCOVER, event.data);
			event.result.returnCode = discoverServices(packet->uuids, packet->uuidCount);
			break;
		}
		case CS_TYPE::CMD_BLE_CENTRAL_READ: {
			auto packet = CS_TYPE_CAST(CMD_BLE_CENTRAL_READ, event.data);
			event.result.returnCode = read(packet->handle);
			break;
		}
		case CS_TYPE::CMD_BLE_CENTRAL_WRITE: {
			auto packet = CS_TYPE_CAST(CMD_BLE_CENTRAL_WRITE, event.data);
			event.result.returnCode = write(packet->handle, packet->data.data, packet->data.len);
			break;
		}
		default: {
			break;
		}
	}
}

