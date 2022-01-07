/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <ble/cs_UUID.h>
#include <ble_gatt.h>
#include <events/cs_EventListener.h>

/**
 * The class that handles transport layer communication.
 * It does not persist any progress statistics across reboots.
 * It assumes that its owner takes care of the connection management
 * (to prevent unnecessary disconnect-connect between different operations.)
 *
 * NOTE: corresponds to dfu_transport_ble.py
 */
class MeshDfuTransport : EventListener {
public:
	/**
	 * Load UUIDs and sets this class as listener.
	 */
	cs_ret_code_t init();

	// -------- util methods -----------
	bool isTargetInDfuMode();

	UUID* getUuids();
	uint8_t getUuidCount();

	// -------- main protocol methods -----------
	void open();
	void close();

	void programInitPacket();
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

	bool _firstInit = true;
	bool _ready = false;

	// ------------------------ event handlers ------------------------

	void onDisconnect();
	void onDiscover(ble_central_discovery_t& result);

	// ----------------------------- utils -----------------------------

	void clearUuidHandles();


	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ++++++++++++++++++++++++ nordic protocol ++++++++++++++++++++++++
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// ------------- the adapter layer for crownstone_ble -------------
	void writeCharacteristicWithoutResponse(uint16_t characteristicHandle, uint8_t* data, uint8_t len);
	void writeCharacteristicForResponse(uint16_t characteristicHandle, uint8_t* data, uint8_t len);

	void receiveRawNotification();

	// ----------------- utility forwardering methods -----------------
	void write_control_point(uint8_t* data, uint8_t len);
	void write_data_point(uint8_t* data, uint8_t len);

	// ----------------------- recovery methods -----------------------

	void try_to_recover_before_send_init();
	void try_to_recover_before_send_firmware();

	void validateCrcCommandResponse();

	// ------------------- nordic protocol commands -------------------
	void __create_command();
	void __create_data();
	void __create_object();
	void __set_prn();
	void __calculate_checksum();
	void __execute();

	void __select_command();
	void __select_data();
	void __select_object();

	// -------------------- raw data communication --------------------
	void __stream_data();
	void __parse_response();
	void __parse_checksum_response();


public:
	void handleEvent(event_t& event) override;
};
