/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "events/cs_EventListener.h"
#include "util/cs_Utils.h"

#define CMD_ADV_NUM_SERVICES_16BIT 4 // There are 4 16 bit service UUIDs in a command advertisement.

/**
 * Time that advertisements of other devices are ignored once a valid command advertisement has been received.
 */
#define CMD_ADV_CLAIM_TIME_MS 1500

struct __attribute__((__packed__)) command_adv_header_t {
	uint8_t sequence0 : 2;
	uint8_t protocol : 3;
	uint8_t sphereId : 8;
	uint8_t accessLevel : 3;
//
//	uint8_t sequence1 : 2;
//	uint16_t reserved : 10;
//	uint16_t payload1 : 4;
//
//	uint8_t sequence2 : 2;
//	uint16_t payload2 : 14;
//
//	uint8_t sequence3 : 2;
//	uint16_t payload3 : 14;
};

class CommandAdvHandler : public EventListener {
public:
	static CommandAdvHandler& getInstance() {
		static CommandAdvHandler staticInstance;
		return staticInstance;
	}
	void handleEvent(event_t & event);


private:
	CommandAdvHandler();
	uint32_t lastVerifiedEncryptedData; // Part of the encrypted data of last verified command advertisement. Used to prevent double handling of command advertisements.
//	uint32_t lastTimestamp = 0; // Decrypted timestamp of last command
	uint8_t timeoutCounter;
	uint8_t timeoutAddress[BLE_GAP_ADDR_LEN];

	void parseAdvertisement(scanned_device_t* scannedDevice);
	// Return true when validated command payload.
	bool handleEncryptedCommandPayload(scanned_device_t* scannedDevice, const command_adv_header_t& header, const cs_data_t& nonce, cs_data_t& encryptedPayload);
	void handleEncryptedRC5Payload(scanned_device_t* scannedDevice, const command_adv_header_t& header, uint16_t encryptedPayload[2]);
};
