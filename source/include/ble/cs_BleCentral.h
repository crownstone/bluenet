/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 26, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <ble/cs_UUID.h>
#include <common/cs_Types.h>
#include <events/cs_Event.h>
#include <structs/cs_PacketsInternal.h>
#include <events/cs_EventListener.h>

/**
 * Class that enables you to connect to a device, and perform write or read operations.
 *
 * Generally, you first connect, then discover, then perform read and/or write operations, and finally disconnect.
 * Note that the disconnect event might happen at any time.
 *
 * This class uses the EncryptionBuffer as write and read buffer.
 *
 * TODO: introduce a timeout, so that a connection cannot remain open.
 */
class BleCentral: EventListener {
public:
	static BleCentral& getInstance() {
		static BleCentral instance;
		return instance;
	}

	/**
	 * Initializes the class:
	 * - Assigns read/write buffer.
	 * - Read config from State.
	 * - Starts listening for events.
	 */
	void init();

	/**
	 * Connect to a device.
	 *
	 * @return ERR_WRONG_STATE         When already connected.
	 * @return ERR_BUSY                An operation is in progress (discovery, read, write, connect, disconnect).
	 * @return ERR_WAIT_FOR_SUCCESS    When the connection will be attempted. Wait for EVT_BLE_CENTRAL_CONNECT_RESULT.
	 */
	cs_ret_code_t connect(const device_address_t& address, uint16_t timeoutMs = 3000);

	/**
	 * Terminate current connection.
	 *
	 * Can be called at any moment: will cancel the current operation if any.
	 *
	 * @return ERR_SUCCESS             When already disconnected.
	 * @return ERR_WAIT_FOR_SUCCESS    When disconnecting. Wait for EVT_BLE_CENTRAL_DISCONNECTED.
	 */
	cs_ret_code_t disconnect();

	/**
	 * Discover services.
	 *
	 * Unfortunately you cannot simply discover all services, you will need to tell in advance which services you are looking for.
	 *
	 * For each discovered service and characteristic, EVT_BLE_CENTRAL_DISCOVERY is dispatched.
	 * Use these events to figure out which handles to use for reading and writing.
	 *
	 * @param[in] uuids                Array of UUIDs that will be attempted to discover.
	 * @param[in] uuidCount            Number of UUIDs in the array.
	 *
	 * @return ERR_WAIT_FOR_SUCCESS    When the discovery is started. Wait for EVT_BLE_CENTRAL_DISCOVERY_RESULT.
	 */
	cs_ret_code_t discoverServices(const UUID* uuids, uint8_t uuidCount);

	/**
	 * Read data from a characteristic.
	 *
	 * @param[in] handle               The characteristic handle to read from. The handle was received during discovery.
	 *
	 * @return ERR_BUSY                An operation is in progress (discovery, read, write, connect, disconnect).
	 * @return ERR_WAIT_FOR_SUCCESS    When the read is started. Wait for EVT_BLE_CENTRAL_READ_RESULT.
	 */
	cs_ret_code_t read(uint16_t handle);

	/**
	 * Write data to a characteristic.
	 *
	 * @param[in] handle               The characteristic handle to write to. The handle was received during discovery.
	 * @param[in] data                 Pointer to data, which will be copied.
	 *                                 If the pointer equals the requested write buffer, no copying will take place.
	 * @param[in] len                  Length of the data to write.
	 *
	 * @return ERR_BUFFER_TOO_SMALL    The data size is too large.
	 * @return ERR_BUSY                An operation is in progress (discovery, read, write, connect, disconnect).
	 * @return ERR_WAIT_FOR_SUCCESS    When the write is started. Wait for EVT_BLE_CENTRAL_WRITE_RESULT.
	 */
	cs_ret_code_t write(uint16_t handle, const uint8_t* data, uint16_t len);

	/**
	 * Performs a write() with the value to enable or disable notifications.
	 *
	 * @param[in] cccdHandle           The characteristic CCCD handle to write to. The handle was received during discovery.
	 * @param[in] enableNotifications  True to enable notifications.
	 *
	 * @return                         See write().
	 */
	cs_ret_code_t writeNotificationConfig(uint16_t cccdHandle, bool enableNotifications);

	/**
	 * Request the write buffer. You can then put your data in this buffer and use it as data in the write() command, so no copy has to take place.
	 *
	 * @return     When busy:          A null pointer.
	 * @return     On success:         A pointer to the write buffer, and the length of the buffer.
	 */
	cs_data_t requestWriteBuffer();

	/**
	 * Check whether this crownstone is connected as central.
	 */
	bool isConnected();

	/**
	 * Check whether an operation is in progress.
	 */
	bool isBusy();

private:
	enum class Operation: uint8_t {
		NONE,
		CONNECT,
		DISCONNECT,
		DISCOVERY,
		READ,
		WRITE
	};

	BleCentral();
	BleCentral(BleCentral const&);
	void operator=(BleCentral const &);

	/**
	 * Buffer used for reading and writing.
	 */
	cs_data_t _buf;

	/**
	 * How much data is actually in the buffer.
	 */
	uint16_t _bufDataSize = 0;

	uint16_t _connectionHandle = BLE_CONN_HANDLE_INVALID;

	ble_db_discovery_t _discoveryModule;

	/**
	 * Current MTU.
	 */
	uint16_t _mtu = BLE_GATT_ATT_MTU_DEFAULT;

	/**
	 * Operation in progress.
	 */
	Operation _currentOperation = Operation::NONE;

	/**
	 * Scan setting to be used when connecting.
	 * Will be retrieved from State at init.
	 */
	TYPIFY(CONFIG_SCAN_INTERVAL_625US) _scanInterval;
	TYPIFY(CONFIG_SCAN_WINDOW_625US) _scanWindow;


	/**
	 * Writes the next chunk of a long write.
	 */
	cs_ret_code_t nextWrite(uint16_t handle, uint16_t offset);

	/**
	 * Finalize an operation.
	 * Will always lead to sending an event, and resetting current operation.
	 */
	void finalizeOperation(Operation operation, cs_ret_code_t retCode);
	void finalizeOperation(Operation operation, uint8_t* data, uint8_t dataSize);
	void sendOperationResult(event_t& event);

	/**
	 * Event handlers.
	 */
	void onGapEvent(uint16_t evtId, const ble_gap_evt_t& event);
	void onGattCentralEvent(uint16_t evtId, const ble_gattc_evt_t& event);

	void onConnect(uint16_t connectionHandle, const ble_gap_evt_connected_t& event);
	void onDisconnect(const ble_gap_evt_disconnected_t& event);
	void onGapTimeout(const ble_gap_evt_timeout_t& event);

	void onMtu(uint16_t gattStatus, const ble_gattc_evt_exchange_mtu_rsp_t& event);
	void onRead(uint16_t gattStatus, const ble_gattc_evt_read_rsp_t& event);
	void onWrite(uint16_t gattStatus, const ble_gattc_evt_write_rsp_t& event);
	void onNotification(uint16_t gattStatus, const ble_gattc_evt_hvx_t& event);


public:
	/**
	 * Internal usage: to be called on BLE events.
	 */
	void onBleEvent(const ble_evt_t* event);

	/**
	 * Internal usage.
	 */
	void onDiscoveryEvent(ble_db_discovery_evt_t& event);

	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& event);
};


