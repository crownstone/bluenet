/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 15, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_UUID.h>
#include <events/cs_EventListener.h>

/**
 * Class to connect to another crownstone, and write control commands.
 *
 * The result of a control command will be received via notifications.
 * These notifications are merged into a single result packet.
 *
 * Performs encryption and decryption of packets.
 *
 * This class uses:
 * - The EncryptionBuffer to read notifications to.
 * - The CharacteristicReadBuffer to decrypt read data to.
 * - The CharacteristicWriteBuffer to construct control packets.
 */
class CrownstoneCentral: EventListener {
public:
	/**
	 * Initializes the class:
	 * - Constructs the service UUIDs.
	 * - Starts listening for events.
	 */
	cs_ret_code_t init();

	/**
	 * Connect, discover, and read session data.
	 *
	 * @param[in] stoneId              The stone ID of the crownstone you want to connect to.
	 * @param[in] timeoutMs            How long (in milliseconds) to try connecting, before giving up.
	 *
	 * @return ERR_WRONG_STATE         When already connected.
	 * @return ERR_BUSY                An operation is in progress (connect, write, disconnect).
	 * @return ERR_WAIT_FOR_SUCCESS    When the connection will be attempted. Wait for EVT_CS_CENTRAL_CONNECT_RESULT.
	 */
	cs_ret_code_t connect(stone_id_t stoneId, uint16_t timeoutMs = 3000);

	/**
	 * Connect, discover, and read session data.
	 *
	 * @param[in] address              The MAC address of the crownstone you want to connect to.
	 * @param[in] timeoutMs            How long (in milliseconds) to try connecting, before giving up.
	 *
	 * @return ERR_WRONG_STATE         When already connected.
	 * @return ERR_BUSY                An operation is in progress (connect, write, disconnect).
	 * @return ERR_WAIT_FOR_SUCCESS    When the connection will be attempted. Wait for EVT_CS_CENTRAL_CONNECT_RESULT.
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
	 * Write a control command, and get the result.
	 *
	 * When the result code in the result data is ERR_WAIT_FOR_SUCCESS, you will get another EVT_CS_CENTRAL_WRITE_RESULT event.
	 *
	 * TODO: add timeout.
	 *
	 * @param[in] commandType          What control command.
	 * @param[in] data                 Pointer to the command payload data, which will be copied.
	 *                                 If the pointer equals the requested write buffer, no copying will take place.
	 * @param[in] size                 Size of the command payload data.
	 *
	 * @return ERR_WAIT_FOR_SUCCESS    When the write is started. Wait for EVT_CS_CENTRAL_WRITE_RESULT.
	 */
	cs_ret_code_t write(cs_control_cmd_t commandType, uint8_t* data, uint16_t size);

	/**
	 * Request the write buffer. You can then put your data in this buffer and use it as data in the write() command, so no copy has to take place.
	 *
	 * @return     When busy:          A null pointer.
	 * @return     On success:         A pointer to the write buffer, and the length of the buffer.
	 */
	cs_data_t requestWriteBuffer();

private:
	enum ServiceIndex {
		SERVICE_INDEX_CROWNSTONE = 0,
		SERVICE_INDEX_SETUP = 1,
		SERVICE_INDEX_DEVICE_INFO = 2,
		SERVICE_INDEX_DFU = 3,
		SERVICE_INDEX_COUNT = 4
	};

	enum class Operation: uint8_t {
		NONE,
		CONNECT,
		WRITE
	};

	enum class ConnectSteps: uint8_t {
		NONE = 0,
		GET_ADDRESS,
		CONNECT,
		DISCOVER,
		ENABLE_NOTIFICATIONS,
		SESSION_KEY,
		SESSION_DATA,
		DONE
	};

	enum class WriteControlSteps: uint8_t {
		NONE = 0,
		WRITE,
		RECEIVE_RESULT,
		DONE
	};

	/**
	 * Service UUIDs we need for discovery.
	 */
	UUID _serviceUuids[SERVICE_INDEX_COUNT];

	/**
	 * Handles that we need for reading and writing.
	 */
	uint16_t _sessionKeyHandle;
	uint16_t _sessionDataHandle;
	uint16_t _controlHandle;
	uint16_t _resultHandle;
	uint16_t _resultCccdHandle;

	/**
	 * Operation mode of the crownstone we are connected to.
	 */
	OperationMode _opMode = OperationMode::OPERATION_MODE_UNINITIALIZED;

	/**
	 * The operation we're currently executing.
	 */
	Operation _currentOperation = Operation::NONE;

	/**
	 * Asynchronous step of an operation we are currently executing.
	 */
	uint8_t _currentStep;

	/**
	 * The notification index that we expect to get next.
	 */
	uint8_t _notificationNextIndex = 0;

	/**
	 * The current size of the merged notification data.
	 */
	uint16_t _notificationMergedDataSize = 0;

	/**
	 * Keep up to which stone ID we're connected.
	 * Not always set though.
	 */
	stone_id_t _stoneId = 0;

	/**
	 * Keep up which timeout we use.
	 */
	uint16_t _timeoutMs;

	/**
	 * A countdown until timeout.
	 */
	uint16_t _timeoutCountDown = 0;

	/**
	 * Reset connection variables.
	 */
	void reset();

	/**
	 * Reset notification merger variables.
	 */
	void resetNotifactionMergerState();

	/**
	 * Read session key or session data, depending on operation mode.
	 * Sets current step.
	 */
	void readSessionData();

	/**
	 * Merge this notification data with that of others.
	 *
	 * @param[in] notificationData     The data of this notification.
	 * @param[out] resultData          The result data, only set on success.
	 *
	 * @return ERR_SUCCESS             All notifications have been merged, and decrypted. The resultData is set.
	 * @return ERR_WAIT_FOR_SUCCESS    The notification has been merged, but there are more to come.
	 * @return ERR_*                   The notification data could not be processed.
	 */
	cs_ret_code_t mergeNotification(const cs_const_data_t& notificationData, cs_data_t& resultData);

	/**
	 * Check whether an operation is in progress.
	 */
	bool isBusy();

	void startTimeoutTimer(uint16_t timeoutMs);
	void stopTimeoutTimer();

	void setStep(ConnectSteps step);
	void setStep(WriteControlSteps step);
	void setStep(uint8_t step);

	/**
	 * Returns true when you can continue.
	 */
	bool finalizeStep(uint8_t step, cs_ret_code_t retCode);
	bool finalizeStep(ConnectSteps step, cs_ret_code_t retCode);
	bool finalizeStep(WriteControlSteps step, cs_ret_code_t retCode);

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
	void onTimeout();
	void onMacAddress(mesh_topo_mac_result_t& result);
	void onConnect(cs_ret_code_t retCode);
	void onDisconnect();
	void onDiscovery(ble_central_discovery_t& result);
	void onDiscoveryDone(cs_ret_code_t retCode);
	void onRead(ble_central_read_result_t& result);
	void onReadDuringConnect(ble_central_read_result_t& result);
	void onWrite(cs_ret_code_t result);
	void onNotification(ble_central_notification_t& result);

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& event);
};

