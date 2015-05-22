/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdint>
//#include <stdint.h>
//#include <common/cs_Types.h>

#include <ble/cs_Nordic.h>

#include <events/cs_EventListener.h>
#include <events/cs_EventTypes.h>

//struct __attribute__((__packed__)) EventMeshPackage {
//	EventType evt;
//	uint8_t* p_data;
//};

#define EVENT_CHANNEL        1
#define DATA_CHANNEL         2
#define HUB_CHANNEL          3

class MeshControl : public EventListener {
private:
	MeshControl();
	MeshControl(MeshControl const&); // singleton, deny implementation
	void operator=(MeshControl const &); // singleton, deny implementation
public:
	// use static variant of singelton, no dynamic memory allocation
	static MeshControl& getInstance() {
		static MeshControl instance;
		return instance;
	}

	/**
	 * Get incoming messages and perform certain actions.
	 */
	void process(uint8_t channel, void* p_data, uint16_t length);

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	void sendPwmValue(uint8_t* address, uint8_t value);
	void sendIBeaconMessage(uint8_t* address, uint16_t major, uint16_t minor, ble_uuid128_t uuid, int8_t rssi);

	void decodeDataMessage(void* p_data, uint16_t length);
private:


};
