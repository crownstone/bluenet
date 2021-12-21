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

public:
	void init(device_address_t target);
	void deinit();

	void connect();
	void disconnect();

	void putTargetInDfuMode();
	void waitForDisconnect();
	void isTargetInDfuMode();

	void open();
	void close();

	void programInitPacket();
	void programFirmware();
};
