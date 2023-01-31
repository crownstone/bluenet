/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "common/cs_Types.h"
#include "events/cs_EventListener.h"
#include "util/cs_Utils.h"

#define CMD_ADV_NUM_SERVICES_16BIT 4  // There are 4 16 bit service UUIDs in a command advertisement.

/**
 * Time that advertisements of other devices are ignored once a valid command advertisement has been received.
 */
#define CMD_ADV_CLAIM_TIME_MS 1500

/**
 * Number of devices that can simultaneously advertise commands
 */
#define CMD_ADV_MAX_CLAIM_COUNT 10

#define CMD_ADC_ENCRYPTED_DATA_SIZE 16

/**
 * Struct used to prevent double handling of similar command advertisements.
 * And to prevent handling command advertisements of many devices at once.
 */
struct __attribute__((__packed__)) command_adv_claim_t {
	uint8_t deviceToken;
	uint8_t timeoutCounter = 0;
	uint8_t encryptedData[CMD_ADC_ENCRYPTED_DATA_SIZE];
	uint16_t encryptedRC5;
	uint16_t decryptedRC5;
};

struct __attribute__((__packed__)) command_adv_header_t {
	//	uint8_t sequence0 : 2;
	uint16_t protocol : 3;
	uint16_t sphereId : 8;
	uint16_t accessLevel : 3;

	//	uint8_t sequence1 : 2;
	//	uint8_t reserved : 2;
	uint16_t deviceToken : 8;
	//	uint16_t payload1 : 4;

	//	uint8_t sequence2 : 2;
	//	uint16_t payload2 : 14;

	//	uint8_t sequence3 : 2;
	//	uint16_t payload3 : 14;
};

class CommandAdvHandler : public EventListener {
public:
	static CommandAdvHandler& getInstance() {
		static CommandAdvHandler staticInstance;
		return staticInstance;
	}
	void init();
	void handleEvent(event_t& event);

private:
	CommandAdvHandler();
	command_adv_claim_t _claims[CMD_ADV_MAX_CLAIM_COUNT];
	TYPIFY(CONFIG_SPHERE_ID) _sphereId = 0;

	void parseAdvertisement(scanned_device_t* scannedDevice);

	// Return true when command payload is validated, and RC5 payload is decrypted.
	bool handleEncryptedCommandPayload(
			scanned_device_t* scannedDevice,
			const command_adv_header_t& header,
			const cs_data_t& nonce,
			cs_data_t& encryptedPayload,
			uint16_t encryptedPayloadRC5[2],
			uint16_t decryptedPayloadRC5[2]);

	bool decryptRC5Payload(uint16_t encryptedPayload[2], uint16_t decryptedPayload[2]);

	void handleDecryptedRC5Payload(
			scanned_device_t* scannedDevice, const command_adv_header_t& header, uint16_t decryptedPayload[2]);

	EncryptionAccessLevel getRequiredAccessLevel(const AdvCommandTypes type);

	/**
	 * Return index of claim with this device token.
	 *
	 * @param[out] decryptedRC5   When previous encrypted data is similar: set to previous decrypted RC5 data.
	 *
	 * Returns -1 when the device token was not found.
	 * Returns -2 when the device token was found, but the previous encryptedData is similar.
	 */
	int checkSimilarCommand(
			uint8_t deviceToken, cs_data_t& encryptedData, uint16_t encryptedRC5, uint16_t& decryptedRC5);

	// Return true when device claimed successfully: when there's a claim spot.
	bool claim(
			uint8_t deviceToken,
			cs_data_t& encryptedData,
			uint16_t encryptedRC5,
			uint16_t decryptedRC5,
			int indexOfDevice);

	void tickClaims();
};
