/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


/**
 * The class that handles transport layer communication.
 *
 * NOTE: corresponds to dfu_transport_ble.py
 */
class MeshDfuTransport {
private:
	uint16_t _ControlPointHandle = 0x0;
	uint16_t _DataPointHandle = 0x0;

	device_address_t _targetDevice;

	// ----- the adapter layer for crownstone_ble -----
	void writeCharacteristicWithoutResponse(uint16_t characteristicHandle, uint8_t* data, uint8_t len);
	void writeCharacteristicForResponse(uint16_t characteristicHandle, uint8_t* data, uint8_t len);

	void receiveRawNotification();

	// ----- utility forwardering methods -----
	void write_control_point(uint8_t* data, uint8_t len);
	void write_data_point(uint8_t* data, uint8_t len);


	// ------------ recovery methods -----------

	void try_to_recover_before_send_init();
	void try_to_recover_before_send_firmware();

	void validateCrcCommandResponse();

	// -------------- nordic protocol commands -----------
	void __create_command();
	void __create_data();
	void __create_object();
	void __set_prn();
	void __calculate_checksum();
	void __execute();

	void __select_command();
	void __select_data();
	void __select_object();

	// ------------ raw data communication ------------
	void __stream_data();
	void __parse_response();
	void __parse_checksum_response();

public:
	void init(device_address_t target);
	void deinit();

	void connect();
	void disconnect();

	void putTargetInDfuMode();
	void waitForDisconnect();
	void isTargetInDfuMode();

	// -------- main protocol methods -----------
	void open();
	void close();

	void programInitPacket();
	void programFirmware();
};
