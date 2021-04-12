/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_BleCentral.h>
#include <logging/cs_Logger.h>
#include <events/cs_Event.h>
#include <ble/cs_Nordic.h>

void handle_discovery(ble_db_discovery_evt_t* event) {
	BleCentral::getInstance().onDiscoveryEvent(event);
}

BleCentral::BleCentral() {
	_discoveryModule.discovery_in_progress = false;
	_discoveryModule.discovery_pending = false;
	_discoveryModule.conn_handle = BLE_CONN_HANDLE_INVALID;
	uint32_t nrfCode = ble_db_discovery_init(handle_discovery);
	APP_ERROR_CHECK(nrfCode);
}

void BleCentral::init() {

}

cs_ret_code_t BleCentral::connect(const device_address_t& address, uint16_t timeoutMs = 3000) {
	if (isConnected()) {
		LOGi("Already connected");
		return ERR_BUSY;
	}

	// In order to connect, other modules need to change their operation.
	event_t connectEvent(CS_TYPE::EVT_OUTGOING_CONNECT_START);
	connectEvent.dispatch();
	if (connectEvent.result.returnCode != ERR_SUCCESS) {
		LOGe("Unable to start connecting: err=%u", connectEvent.result.returnCode);
		// Let modules know they can resume their normal operation.
		event_t disconnectEvent(CS_TYPE::EVT_OUTGOING_DISCONNECTED);
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
	scanParams.interval = 160;
	scanParams.window = 80;

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

	LOGi("Connecting..");

	// We will get either BLE_GAP_EVT_CONNECTED or BLE_GAP_EVT_TIMEOUT.
	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t BleCentral::disconnect() {
	uint32_t nrfCode = sd_ble_gap_disconnect(_connectionHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Disconnect err=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}

	LOGi("Disconnecting..");

	// We will get either BLE_GAP_EVT_ or BLE_GAP_EVT_TIMEOUT.
	return ERR_WAIT_FOR_SUCCESS;
}

bool BleCentral::isConnected() {
	return false;
}

cs_ret_code_t BleCentral::discoverServices(const UUID* uuids, uint8_t uuidCount) {
	uint32_t nrfCode = NRF_SUCCESS;
	for (uint8_t i = 0; i < uuidCount; ++i) {
		nrfCode = ble_db_discovery_evt_register(&(uuids[i].getUuid()));
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
//		disconnect();
		return ERR_UNSPECIFIED;
	}

	return ERR_SUCCESS;
}

void BleCentral::onDiscoveryEvent(ble_db_discovery_evt_t* event) {
	uint32_t retCode;
	switch (event->evt_type) {
		case BLE_DB_DISCOVERY_COMPLETE: {
			LOGi("Discovery found uuid=0x%04X type=%u characteristicCount=%u", event->params.discovered_db.srv_uuid.uuid, event->params.discovered_db.srv_uuid.type, event->params.discovered_db.char_count);
			if (event->params.discovered_db.srv_uuid.type >= BLE_UUID_TYPE_VENDOR_BEGIN) {
				ble_uuid128_t fullUuid;
				uint8_t uuidSize = 0;
				retCode = sd_ble_uuid_encode(&(event->params.discovered_db.srv_uuid), &uuidSize, fullUuid.uuid128);
				if (retCode == NRF_SUCCESS && uuidSize == sizeof(fullUuid)) {
#if CS_SERIAL_NRF_LOG_ENABLED == 0
					LOGd("Full uuid: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
							fullUuid.uuid128[15],
							fullUuid.uuid128[14],
							fullUuid.uuid128[13],
							fullUuid.uuid128[12],
							fullUuid.uuid128[11],
							fullUuid.uuid128[10],
							fullUuid.uuid128[9],
							fullUuid.uuid128[8],
							fullUuid.uuid128[7],
							fullUuid.uuid128[6],
							fullUuid.uuid128[5],
							fullUuid.uuid128[4],
							fullUuid.uuid128[3],
							fullUuid.uuid128[2],
							fullUuid.uuid128[1],
							fullUuid.uuid128[0]);
#endif
				}
				else {
					LOGw("Failed to get full uuid");
				}
			}
			break;
		}
		case BLE_DB_DISCOVERY_SRV_NOT_FOUND: {
			LOGi("Discovery not found uuid=0x%04X type=%u", event->params.discovered_db.srv_uuid.uuid, event->params.discovered_db.srv_uuid.type);
			break;
		}
		case BLE_DB_DISCOVERY_ERROR: {
			LOGw("Discovery error %u", event->params.err_code);
			break;
		}
		case BLE_DB_DISCOVERY_AVAILABLE: {
			// A bug prevents this event from ever firing. It is fixed in SDK 16.0.0.
			// Instead, we should be done when the number of registered uuids equals the number of received BLE_DB_DISCOVERY_COMPLETE + BLE_DB_DISCOVERY_SRV_NOT_FOUND events.
			// See https://devzone.nordicsemi.com/f/nordic-q-a/20846/getting-service-count-from-database-discovery-module
			// We apply a similar patch to the SDK.
			LOGi("Discovery done");

			// According to the doc, the "services" struct is only for internal usage.
			// But it seems to store all the discovered services and characteristics.
			for (uint8_t s = 0; s < _discoveryModule.discoveries_count; ++s) {
				LOGi("service: uuidType=%u uuid=0x%04X", _discoveryModule.services[s].srv_uuid.type, _discoveryModule.services[s].srv_uuid.uuid);
				for (uint8_t c = 0; c < _discoveryModule.services[s].char_count; ++c) {
					ble_gatt_db_char_t* characteristic = &(_discoveryModule.services[s].charateristics[c]);
					LOGi("    char: cccd_handle=0x%X uuidType=%u uuid=0x%04X handle_value=0x%X handle_decl=0x%X",
							characteristic->cccd_handle,
							characteristic->characteristic.uuid.type,
							characteristic->characteristic.uuid.uuid,
							characteristic->characteristic.handle_value,
							characteristic->characteristic.handle_decl);
				}
			}

			break;
		}
	}
	LOGd("Discovery progress=%u pending=%u numServicesHandled=%u numServices=%u",
			_discoveryModule.discovery_in_progress,
			_discoveryModule.discovery_pending,
			_discoveryModule.discoveries_count,
			_discoveryModule.srv_count);
}

cs_ret_code_t BleCentral::write(uint16_t handle, const uint8_t* data, uint16_t len) {

	// Copy the data to the buffer.
	if (len > sizeof(_buf)) {
		return ERR_BUFFER_TOO_SMALL;
	}
	memcpy(_buf, data, len);

	uint32_t nrfCode = ERR_UNSPECIFIED;

	ble_gattc_write_params_t writeParams;

	writeParams.flags = 0;
	writeParams.handle = handle;
	writeParams.p_value = _buf;

	if (len > _writeMtu) {
		// We need to break up the write into chunks.
		// Although it seems like what we're looking for, the "Queued Write module" is NOT what we can use for this.
		//     https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.2.0/lib_ble_queued_write.html
		//     https://devzone.nordicsemi.com/f/nordic-q-a/43140/queued-write-module
		// Sequence chart of long write:
		//     https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.1.1/group___b_l_e___g_a_t_t_c___v_a_l_u_e___l_o_n_g___w_r_i_t_e___m_s_c.html
		// multiple      BLE_GATT_OP_PREP_WRITE_REQ
		// finalized by  BLE_GATT_OP_EXEC_WRITE_REQ

		_bufDataSize = len;
		return nextWrite(handle, 0);
	}
	else {
		// We can do a single write with response.
		writeParams.write_op = BLE_GATT_OP_WRITE_REQ; // Write with response (ack). Triggers BLE_GATTC_EVT_WRITE_RSP.
		writeParams.offset = 0;
		writeParams.len = len;

		nrfCode = sd_ble_gattc_write(_connectionHandle, &writeParams);
		if (nrfCode != NRF_SUCCESS) {
			LOGe("Failed to write. nrfCode=%u", nrfCode);
			return ERR_UNSPECIFIED;
		}
	}
	return ERR_WAIT_FOR_SUCCESS;
}

cs_ret_code_t BleCentral::nextWrite(uint16_t handle, uint16_t offset) {
	ble_gattc_write_params_t writeParams;
	if (offset < _bufDataSize) {
		const uint16_t chunkSize = _writeMtu - 2;
		writeParams.write_op = BLE_GATT_OP_PREP_WRITE_REQ;
		writeParams.flags = 0;
		writeParams.handle = handle;
		writeParams.offset = offset;
		writeParams.len = std::min(chunkSize, _bufDataSize - offset);
		writeParams.p_value = _buf + offset;
	}
	else {
		writeParams.write_op = BLE_GATT_OP_EXEC_WRITE_REQ;
		writeParams.flags = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;
		writeParams.handle = handle;
		writeParams.offset = 0;
		writeParams.len = 0;
		writeParams.p_value = nullptr;
	}
	uint32_t nrfCode = sd_ble_gattc_write(_connectionHandle, &writeParams);
	if (nrfCode != NRF_SUCCESS) {
		LOGe("Failed to prep write. nrfCode=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}
	return ERR_WAIT_FOR_SUCCESS;
}



cs_ret_code_t BleCentral::read(uint16_t handle) {
	uint32_t nrfCode;
	nrfCode = sd_ble_gattc_read(_connectionHandle, handle, 0);
}

