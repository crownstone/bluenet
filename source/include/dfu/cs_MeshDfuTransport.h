/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <ble_gatt.h>
#include <ble/cs_BleCentral.h>
#include <ble/cs_UUID.h>
#include <common/cs_Component.h>
#include <dfu/cs_MeshDfuConstants.h>
#include <events/cs_EventListener.h>
#include <util/cs_Coroutine.h>

/**
 * The class that handles transport layer communication.
 * It does not persist any progress statistics across reboots.
 * It assumes that its owner takes care of the connection management
 * (to prevent unnecessary disconnect-connect between different operations.)
 *
 * NOTE: corresponds to dfu_transport_ble.py
 */
class MeshDfuTransport : public EventListener, public Component {
public:
	/**
	 * Load UUIDs and sets this class as listener.
	 */
	cs_ret_code_t init();

	// -------- util methods -----------
	/**
	 * Returns true if the DFU service id and both dfu characteristics have
	 * been discovered.
	 *
	 * Note: discovery doesn't have to be 'completed', returnvalue of this function
	 * changes as soon as the required events are handled.
	 */
	bool isTargetInDfuMode();

	UUID* getServiceUuids();
	uint8_t getServiceUuidCount();

	// -------- main protocol methods -----------

	/**
	 * sends first package of dfu protocol.
	 *
	 * dispatches EVT_MESH_DFU_TRANSPORT_RESULT when done.
	 */
	void prepare();

	/**
	 * sends the init packet
	 *
	 * dispatches EVT_MESH_DFU_TRANSPORT_RESULT when done.
	 */
	void programInitPacket();

	/**
	 * sends a chunk of firmware
	 *
	 * dispatches EVT_MESH_DFU_TRANSPORT_RESULT when done.
	 */
	void programFirmware();

private:
	enum Index : uint8_t {
		ControlPoint = 0,
		DataPoint,
		ENUMLEN
	};

	/**
	 * address elements as _uuids[Index::ControlPoint] etc.
	 */
	UUID _uuids[Index::ENUMLEN] = {};
	uint16_t _uuidHandles[Index::ENUMLEN] = {};

	UUID _dfuServiceUuid;
	bool _dfuServiceFound = false;

	bool _firstInit = true;
	bool _discoveryComplete = false;

	BleCentral* _bleCentral;


	// ------------------------ async flowcontrol ------------------------
	/**
	 * Administration variables for the method setEventCallback(..);
	 *
	 * _expectedEvent is only valid if _onExpectedEvent is set.
	 * _onExpectedEvent is a pointer to member function taking an event_t&.
	 */
	CS_TYPE _expectedEvent;
	typedef void(MeshDfuTransport::*ExpectedEventCallback)(event_t&);
	ExpectedEventCallback _onExpectedEvent = nullptr;

	/**
	 * Utility to call onEventCallbackTimeOut at desired moments.
	 */
	Coroutine _timeOutRoutine;
	typedef void(MeshDfuTransport::*TimeoutCallback)();
	TimeoutCallback _onTimeout = nullptr;

	/**
	 * sets a callback to be called when the given event type is received.
	 * The event is passed into the callback for further inspection.
	 *
	 * Returns true if a previous callback was overriden by this call.
	 *
	 * Note: be sure to set the callback before the event is triggered.
	 * Note: event callback is cleared after it is received. If you want to repeat, set it again.
	 */
	bool setEventCallback(
			CS_TYPE evttype,
			ExpectedEventCallback callback);

	/**
	 * sets the timeout callback and delay before calling it.
	 */
	bool setTimeoutCallback(
			TimeoutCallback onTimeout,
			uint32_t timeoutMs = MeshDfuConstants::DfuHostSettings::NotificationTimeoutMs
			);

	/**
	 * sets the event callback to nullptr.
	 */
	void clearEventCallback();

	/**
	 * sets the timeout callback to nullptr.
	 */
	void clearTimeoutCallback();

	/**
	 * This is called by the timeoutRoutine to call the current _onTimeout callback.
	 */
	void onEventCallbackTimeOut();

	// ------------------------ event handlers ------------------------

	void onDisconnect();

	/**
	 * Updates the handles of dfu characteristic handles and checks if
	 * the DFU service is found.
	 *
	 * If _discoveryComplete is set to true, this calls clearConnectionData.
	 */
	void onDiscover(ble_central_discovery_t& result);

	/**
	 * sets _discoveryComplete in order to prevent connection handle information
	 */
	void onDiscoveryComplete();

	/**
	 * handles notifications received from the target dfu device after
	 * writing a nordic protocol dfu operation.
	 *
	 * checks for errors using __parse_response and _lastOperation
	 */
	void onNotificationReceived(event_t& event);

	/**
	 * check if we were busy. if so, send event of type corresponding to _lastOperation
	 * with ERR_TIMEOUT as result. Clear event callback
	 */
	void onTimeout();

	// ----------------------------- utils -----------------------------

	/**
	 * clears all handles etc. after calling this, isTargetInDfuMode
	 * returns false.
	 */
	void clearConnectionData();

	/**
	 * true: an operation is sent to the connected device and we are
	 * awaiting response.
	 */
	bool isBusy();

	void dispatchResult(cs_result_t res);
	void dispatchResponse(MeshDfuTransportResponse res);

	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ++++++++++++++++++++++++ nordic protocol ++++++++++++++++++++++++
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	enum class OP_CODE : uint8_t {
		None         = 0x00, // added by Crownstone
		CreateObject = 0x01,
		SetPRN       = 0x02,
		CalcChecSum  = 0x03,
		Execute      = 0x04,
		ReadObject   = 0x06,
		Response     = 0x60,
	};

	enum class RES_CODE : uint8_t {
		InvalidCode           = 0x00,
		Success               = 0x01,
		NotSupported          = 0x02,
		InvalidParameter      = 0x03,
		InsufficientResources = 0x04,
		InvalidObject         = 0x05,
		InvalidSignature      = 0x06,
		UnsupportedType       = 0x07,
		OperationNotPermitted = 0x08,
		OperationFailed       = 0x0A,
		ExtendedError         = 0x0B,
	};

	OP_CODE _lastOperation = OP_CODE::None;
	uint16_t _prn = 0; // nordic protocol: Packet Receipt Notification.

	// ------------- the adapter layer for crownstone_ble -------------
	void writeCharacteristicWithoutResponse(uint16_t characteristicHandle, cs_data_t buff);
	void writeCharacteristicForResponse(uint16_t characteristicHandle, cs_data_t buff);

	// ----------------- utility forwardering methods -----------------
	void write_control_point(cs_data_t buff);
	void write_data_point(cs_data_t buff);

	// ----------------------- recovery methods -----------------------

	void try_to_recover_before_send_init();
	void try_to_recover_before_send_firmware();

	void validateCrcCommandResponse();

	// ------------------- nordic protocol commands -------------------

	/**
	 * All these methods send a dfu packet on the control point.
	 * Some require a notification to be parsed.
	 */

	void _createCommand(uint32_t size);
	void _createData(uint32_t size);

	void _createObject(uint8_t objectType, uint32_t size);

	void _setPrn();
	void _calculateChecksum();
	void _execute();

	void _selectCommand();
	void _selectData();
	void _selectObject(uint8_t objectType);

	// -------------------- raw data communication --------------------
	void __stream_data();

	/**
	 * checks if structure of incoming notifications matches _lastOperation.
	 */
	cs_ret_code_t _parseResult(cs_const_data_t evtData);

	/**
	 * Extracts data from a notification received after an OP_CODE::ReadObject command.
	 * E.g. _selectData, _selectCommand and _selectObject.
	 */
	MeshDfuTransportResponse _parseResponseReadObject(cs_const_data_t evtData);

	/**
	 * Extracts data from a notification received after an OP_CODE::CalcChecksum command.
	 */
	MeshDfuTransportResponse _parseResponseCalcChecksum(cs_const_data_t evtData);

	void __parse_checksum_response();

public:
	void handleEvent(event_t& event) override;
};
