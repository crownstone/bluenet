/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 15, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_UUID.h>
#include <events/cs_EventListener.h>

class CrownstoneCentral: EventListener {
public:
	cs_ret_code_t init();

	/**
	 * Connect, discover, and read session data.
	 */
	cs_ret_code_t connect(stone_id_t stoneId, uint16_t timeoutMs = 3000);

	/**
	 * Connect, discover, and read session data.
	 */
	cs_ret_code_t connect(const device_address_t& address, uint16_t timeoutMs = 3000);

	/**
	 * Disconnect
	 */
	cs_ret_code_t disconnect();

	/**
	 * Write a control command.
	 */
	cs_ret_code_t write(cs_control_cmd_t commandType, uint8_t* data, uint16_t size);

private:
	enum class Operation: uint8_t {
		NONE,
		CONNECT,
//		DISCONNECT,
//		READ,
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

	UUID _uuids[4];

	uint16_t _sessionKeyHandle;
	uint16_t _sessionDataHandle;
	uint16_t _controlHandle;
	uint16_t _resultHandle;
	uint16_t _resultCccdHandle;
	OperationMode _opMode = OperationMode::OPERATION_MODE_UNINITIALIZED;
	Operation _currentOperation = Operation::NONE;
	uint8_t _currentStep;

	uint8_t _notificationNextIndex = 0;
	uint16_t _notificationMergedDataSize = 0;

	void resetNotifactionMergerState();

	void enableNotifications();
	void readSessionData();
	cs_ret_code_t mergeNotification(cs_const_data_t& data);

	void setStep(ConnectSteps step);

	/**
	 * Returns true when you can continue.
	 */
	bool finalizeStep(uint8_t step, cs_ret_code_t retCode);
	bool finalizeStep(ConnectSteps step, cs_ret_code_t retCode);

	void finalizeOperation(Operation operation, cs_ret_code_t retCode);
	void finalizeOperation(Operation operation, uint8_t* data, uint8_t dataSize);
	void sendOperationResult(event_t& event);

	void onConnect(cs_ret_code_t retCode);
	void onDiscovery(ble_central_discovery_t& result);
	void onDiscoveryDone(cs_ret_code_t retCode);
	void onRead(ble_central_read_result_t& result);
	void onWrite(cs_ret_code_t result);
	void onNotification(ble_central_notification_t& result);

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& event);
};

