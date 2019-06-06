/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_CommandAdvHandler.h"
#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"
#include "events/cs_EventDispatcher.h"
//#include "ble/cs_Nordic.h"
#include "common/cs_Types.h"
#include "processing/cs_EncryptionHandler.h"
//#include "processing/cs_CommandHandler.h"
#include "storage/cs_State.h"

//#define COMMAND_ADV_VERBOSE

#if CMD_ADV_CLAIM_TIME_MS / TICK_INTERVAL_MS > 250
#error "Timeout counter will overflow."
#endif

CommandAdvHandler::CommandAdvHandler():
lastVerifiedEncryptedData(0),
timeoutCounter(0)
{
	LOGd("constructor");
	EventDispatcher::getInstance().addListener(this);
//	LOGd("this=%p timeoutCounter=%u %p", this, timeoutCounter, &timeoutCounter);
}

void CommandAdvHandler::parseAdvertisement(scanned_device_t* scannedDevice) {
//	_log(SERIAL_DEBUG, "adv: ");
//	_log(SERIAL_DEBUG, "rssi=%i addressType=%u MAC=", scannedDevice->rssi, scannedDevice->addressType);
//	BLEutil::printAddress((uint8_t*)(scannedDevice->address), MAC_ADDRESS_LEN);
// addressType:
//#define BLE_GAP_ADDR_TYPE_PUBLIC                        0x00 /**< Public address. */
//#define BLE_GAP_ADDR_TYPE_RANDOM_STATIC                 0x01 /**< Random Static address. */
//#define BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE     0x02 /**< Private Resolvable address. */
//#define BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE 0x03 /**< Private Non-Resolvable address. */

// scan_rsp: 1 if data is scan response, ignore type if so.

// type:
//#define BLE_GAP_ADV_TYPE_ADV_IND          0x00   /**< Connectable undirected. */
//#define BLE_GAP_ADV_TYPE_ADV_DIRECT_IND   0x01   /**< Connectable directed. */
//#define BLE_GAP_ADV_TYPE_ADV_SCAN_IND     0x02   /**< Scannable undirected. */
//#define BLE_GAP_ADV_TYPE_ADV_NONCONN_IND  0x03   /**< Non connectable undirected. */

// dlen: len of data
// data: uint8_t[]

	uint32_t errCode;
	cs_data_t services16bit;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, scannedDevice->data, scannedDevice->dataSize, &services16bit);
	if (errCode != ERR_SUCCESS) {
		return;
	}
	cs_data_t services128bit;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, scannedDevice->data, scannedDevice->dataSize, &services128bit);
	if (errCode != ERR_SUCCESS) {
		return;
	}

#ifdef COMMAND_ADV_VERBOSE
	LOGd("rssi=%i", scannedDevice->rssi);
	_log(SERIAL_DEBUG, "16bit services: ");
	BLEutil::printArray(services16bit.data, services16bit.len);
#endif
#ifdef COMMAND_ADV_VERBOSE
	_log(SERIAL_DEBUG, "128bit services: ");
	BLEutil::printArray(services128bit.data, services128bit.len); // Received as uint128, so bytes are reversed.
#endif

	if (services16bit.len < (CMD_ADV_NUM_SERVICES_16BIT * sizeof(uint16_t))) {
		return;
	}

	command_adv_header_t header = command_adv_header_t();
	bool foundSequences[CMD_ADV_NUM_SERVICES_16BIT] = { false };
	uint8_t nonce[CMD_ADV_NUM_SERVICES_16BIT * sizeof(uint16_t)];
	uint16_t encryptedPayloadRC5[2];
	// Fill the struct with data from the 4 service UUIDs.
	// Keep up which sequence numbers have been handled.
	// Fill the nonce with the service data in the correct order.
	for (int i=0; i < CMD_ADV_NUM_SERVICES_16BIT; ++i) {
		uint16_t serviceUuid = ((uint16_t*)services16bit.data)[i];
#ifdef COMMAND_ADV_VERBOSE
		LOGd("uuid=%u", serviceUuid);
#endif
		uint8_t sequence = (serviceUuid >> (16-2)) & 0x0003;
		foundSequences[sequence] = true;
		switch (sequence) {
		case 0:
			header.sequence0 = sequence;
			header.protocol =    (serviceUuid >> (16-2-3)) & 0x07;
			header.sphereId =    (serviceUuid >> (16-2-3-8)) & 0xFF;
			header.accessLevel = (serviceUuid >> (16-2-3-8-3)) & 0x07;
			memcpy(nonce+0, &serviceUuid, sizeof(uint16_t));
			break;
		case 1:
//			header.sequence1 = sequence;
			encryptedPayloadRC5[0] = ((serviceUuid >> (16-2-10-4)) & 0x0F) << (16-4);   // Last 4 bits of this service UUID are first 4 bits of encrypted payload[0].
			memcpy(nonce+2, &serviceUuid, sizeof(uint16_t));
			break;
		case 2:
//			header.sequence2 = sequence;
			encryptedPayloadRC5[0] += ((serviceUuid >> (16-2-12)) & 0x0FFF) << (16-4-12); // Bits 2 - 13 of this service UUID are last 12 bits of encrypted payload[0].
			encryptedPayloadRC5[1] = ((serviceUuid >> (16-2-12-2)) & 0x03) << (16-2);   // Last 2 bits of this service UUID are first 2 bits of encrypted payload[1].
			memcpy(nonce+4, &serviceUuid, sizeof(uint16_t));
			break;
		case 3:
//			header.sequence3 = sequence;
			encryptedPayloadRC5[1] += ((serviceUuid >> (16-2-14)) & 0x3FFF) << (16-2-14);      // Last 14 bits of this service UUID are last 14 bits of encrypted payload[1].
			memcpy(nonce+6, &serviceUuid, sizeof(uint16_t));
			break;
		}
	}
	for (int i=0; i < CMD_ADV_NUM_SERVICES_16BIT; ++i) {
		if (!foundSequences[i]) {
			return;
		}
	}

#ifdef COMMAND_ADV_VERBOSE
//	uint8_t* pHeader = (uint8_t*)&header;
//	LOGd("header pointer=%p size=%u", pHeader, sizeof(header));
//	_logSerial(SERIAL_DEBUG, "data=");
//	for (int i=0; i<8; ++i) {
//		uint8_t* ptr = pHeader + i;
//		_logSerial(SERIAL_DEBUG, "%u%u%u%u %u%u%u%u ",
//				((*ptr) & 0x80) >> 7,
//				((*ptr) & 0x40) >> 6,
//				((*ptr) & 0x20) >> 5,
//				((*ptr) & 0x10) >> 4,
//				((*ptr) & 0x08) >> 3,
//				((*ptr) & 0x04) >> 2,
//				((*ptr) & 0x02) >> 1,
//				((*ptr) & 0x01) >> 0
//				);
//	}
//	_logSerial(SERIAL_DEBUG, SERIAL_CRLF);
	LOGd("protocol=%u sphereId=%u accessLevel=%u address=", header.protocol, header.sphereId, header.accessLevel);
	BLEutil::printAddress(scannedDevice->address, MAC_ADDRESS_LEN);
//	logSerial(SERIAL_DEBUG, "nonce: ");
//	BLEutil::printArray(nonce, sizeof(nonce));
#endif

	cs_data_t nonceData;
	nonceData.data = nonce;
	nonceData.len = sizeof(nonce);
	bool validated = handleEncryptedCommandPayload(scannedDevice, header, nonceData, services128bit);
//	LOGd("this=%p instance=%p timeoutCounter=%u %p", this, &(CommandAdvertisementHandler::getInstance()), timeoutCounter, &timeoutCounter);
	if (validated) {
		handleEncryptedRC5Payload(scannedDevice, header, encryptedPayloadRC5);
	}
}

// Return true when validated command payload.
bool CommandAdvHandler::handleEncryptedCommandPayload(scanned_device_t* scannedDevice, const command_adv_header_t& header, const cs_data_t& nonce, cs_data_t& encryptedPayload) {
	if (memcmp(&lastVerifiedEncryptedData, encryptedPayload.data + 4, sizeof(lastVerifiedEncryptedData)) == 0) {
		// Ignore this command, as it has already been handled.
#ifdef COMMAND_ADV_VERBOSE
		LOGd("Ignore similar payload");
#endif
		return true; // TODO: should be false, but that means we don't get any background advertisement payloads when command advertisement remains unchanged.
	}

	EncryptionAccessLevel accessLevel;
	switch(header.accessLevel) {
	case 0:
		accessLevel = ADMIN;
		break;
	case 1:
		accessLevel = MEMBER;
		break;
	case 2:
		accessLevel = BASIC;
		break;
	case 4:
		accessLevel = SETUP;
		break;
	default:
		accessLevel = NOT_SET;
		break;
	}
	if (accessLevel == NOT_SET) {
		return false;
	}

	// TODO: can decrypt to same buffer?
	uint8_t decryptedData[16];
	if (!EncryptionHandler::getInstance().decryptBlockCTR(encryptedPayload.data, encryptedPayload.len, decryptedData, 16, accessLevel, nonce.data, nonce.len)) {
		return false;
	}
#ifdef COMMAND_ADV_VERBOSE
	_log(SERIAL_DEBUG, "decrypted data: ");
	BLEutil::printArray(decryptedData, 16);
#endif

	uint32_t validationTimestamp = (decryptedData[0] << 0) | (decryptedData[1] << 8) | (decryptedData[2] << 16) | (decryptedData[3] << 24);
	TYPIFY(STATE_TIME) timestamp;
	State::getInstance().get(CS_TYPE::STATE_TIME, &timestamp, sizeof(timestamp));
	uint8_t type = decryptedData[4];
	uint16_t length = 16 - 5;
	uint8_t* commandData = decryptedData + 5;

	// TODO: validate with time.
	if (validationTimestamp != 0xCAFEBABE) {
		return false;
	}
#ifdef COMMAND_ADV_VERBOSE
	LOGd("timeoutCounter=%u time=%u validation=%u type=%u length=%u data:", timeoutCounter, timestamp, validationTimestamp, type, length);
	BLEutil::printArray(commandData, length);
#endif
	// Let a phone claim the advertisement commands.
	// Do this after validation, so that validation can still take place.
#ifdef COMMAND_ADV_VERBOSE
	if (timeoutCounter) {
		LOGd("timeout: memcmp=%i address=", memcmp(timeoutAddress, scannedDevice->address, MAC_ADDRESS_LEN));
		BLEutil::printAddress(scannedDevice->address, MAC_ADDRESS_LEN);
	}
#endif
	if ((timeoutCounter) && (memcmp(timeoutAddress, scannedDevice->address, MAC_ADDRESS_LEN) != 0)) {
		return true;
	}

	// Can't do this (yet?), because phones may have different times.
//	if (validationTimestamp < lastTimestamp) {
//		// Ignore this command, a newer command has been received.
//		return true;
//	}

	// After validation, remember the last verified data.
	// TODO: this doesn't check the whole encrypted payload, while changing the Nth byte in the decrypted payload, only changes the Nth byte in the encrypted payload.
	// For now: check byte 4-7: the bytes that will change.
	memcpy(&lastVerifiedEncryptedData, encryptedPayload.data + 4, sizeof(lastVerifiedEncryptedData));
//	lastTimestamp = validationTimestamp;

	CommandHandlerTypes commandType = CTRL_CMD_UNKNOWN;
	switch (type) {
	case 1:
		commandType = CTRL_CMD_MULTI_SWITCH;
		break;
	}

	if (commandType == CTRL_CMD_UNKNOWN) {
		return true;
	}

	// TODO: for some reason, "this" changes after the call to CommandHandler::getInstance().handleCommand(). This is why we set timeoutCounter before the command handler.

//	LOGd("this=%p instance=%p timeoutCounter=%u %p", this, &(CommandAdvertisementHandler::getInstance()), timeoutCounter, &timeoutCounter);
	timeoutCounter = CMD_ADV_CLAIM_TIME_MS / TICK_INTERVAL_MS;
	memcpy(timeoutAddress, scannedDevice->address, MAC_ADDRESS_LEN);
//	LOGd("this=%p instance=%p timeoutCounter=%u %p", this, &(CommandAdvertisementHandler::getInstance()), timeoutCounter, &timeoutCounter);
	TYPIFY(CMD_CONTROL_CMD) controlCmd;
	controlCmd.type = commandType;
	controlCmd.accessLevel = accessLevel;
	controlCmd.data = commandData;
	controlCmd.size = length;
	LOGd("send cmd type=%u", controlCmd.type);
	event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd));
	EventDispatcher::getInstance().dispatch(event);
//	LOGd("this=%p instance=%p timeoutCounter=%u %p", this, &(CommandAdvertisementHandler::getInstance()), timeoutCounter, &timeoutCounter);
	return true;
}

void CommandAdvHandler::handleEncryptedRC5Payload(scanned_device_t* scannedDevice, const command_adv_header_t& header, uint16_t encryptedPayload[2]) {
#ifdef COMMAND_ADV_VERBOSE
	LOGd("encrypted RC5=[%u %u]", encryptedPayload[0], encryptedPayload[1]);
#endif
	// TODO: can decrypt to same buffer?
	uint16_t decryptedPayload[2];
	bool success = EncryptionHandler::getInstance().RC5Decrypt(encryptedPayload, sizeof(uint16_t) * 2, decryptedPayload, sizeof(decryptedPayload)); // Can't use sizeof(encryptedPayload) as that returns size of pointer.
	if (!success) {
		return;
	}
#ifdef COMMAND_ADV_VERBOSE
	LOGd("decrypted RC5=[%u %u]", decryptedPayload[0], decryptedPayload[1]);
#endif

	// TODO: overwrite the first byte of the payload, so that it contains the timestamp?

	adv_background_t backgroundAdv;
	switch (header.protocol) {
	case 1:
		backgroundAdv.protocol = 1;
	}
	backgroundAdv.sphereId = header.sphereId;
	backgroundAdv.data = (uint8_t*)(decryptedPayload);
	backgroundAdv.dataSize = sizeof(decryptedPayload);
	backgroundAdv.macAddress = scannedDevice->address;
	backgroundAdv.rssi = scannedDevice->rssi;

	event_t event(CS_TYPE::EVT_ADV_BACKGROUND, &backgroundAdv, sizeof(backgroundAdv));
	EventDispatcher::getInstance().dispatch(event);
}

void CommandAdvHandler::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::EVT_DEVICE_SCANNED: {
		TYPIFY(EVT_DEVICE_SCANNED)* scannedDevice = (TYPIFY(EVT_DEVICE_SCANNED)*)event.data;
		parseAdvertisement(scannedDevice);
		break;
	}
	case CS_TYPE::EVT_TICK: {
		if (timeoutCounter) {
			timeoutCounter--;
#ifdef COMMAND_ADV_VERBOSE
			LOGd("timeoutCounter=%u", timeoutCounter);
#endif
		}
		break;
	}
	default:
		break;
	}
}
