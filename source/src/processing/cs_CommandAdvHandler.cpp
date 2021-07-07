/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <encryption/cs_AES.h>
#include <encryption/cs_KeysAndAccess.h>
#include <encryption/cs_RC5.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandAdvHandler.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

// Defines to enable extra debug logs.
//#define COMMAND_ADV_DEBUG
//#define COMMAND_ADV_VERBOSE

#ifdef COMMAND_ADV_DEBUG
#define LOGCommandAdvDebug LOGd
#else
#define LOGCommandAdvDebug LOGnone
#endif

#ifdef COMMAND_ADV_VERBOSE
#define LOGCommandAdvVerbose LOGd
#else
#define LOGCommandAdvVerbose LOGnone
#endif

#if CMD_ADV_CLAIM_TIME_MS / TICK_INTERVAL_MS > 250
#error "Timeout counter will overflow."
#endif

constexpr int8_t RSSI_LOG_THRESHOLD = -40;

CommandAdvHandler::CommandAdvHandler() {
}

void CommandAdvHandler::init() {
	State::getInstance().get(CS_TYPE::CONFIG_SPHERE_ID, &_sphereId, sizeof(_sphereId));
	EventDispatcher::getInstance().addListener(this);
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

	if (scannedDevice->rssi > RSSI_LOG_THRESHOLD) {
#ifdef COMMAND_ADV_VERBOSE
		LOGCommandAdvVerbose("rssi=%i", scannedDevice->rssi);
		_log(SERIAL_DEBUG, false, "16bit services: ");
		_logArray(SERIAL_DEBUG, true, services16bit.data, services16bit.len);
#endif
#ifdef COMMAND_ADV_VERBOSE
		_log(SERIAL_DEBUG, false, "128bit services: ");
		_logArray(SERIAL_DEBUG, true, services128bit.data, services128bit.len); // Received as uint128, so bytes are reversed.
#endif
	}

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
		LOGCommandAdvVerbose("uuid=%u", serviceUuid);
		uint8_t sequence = (serviceUuid >> (16-2)) & 0x0003;
		foundSequences[sequence] = true;
		switch (sequence) {
		case 0:
//			header.sequence0 = sequence;                               // 2 bits sequence
			header.protocol =    (serviceUuid >> (16-2-3)) & 0x07;     // 3 bits protocol
			header.sphereId =    (serviceUuid >> (16-2-3-8)) & 0xFF;   // 8 bits sphereId
			header.accessLevel = (serviceUuid >> (16-2-3-8-3)) & 0x07; // 3 bits access level
			memcpy(nonce+0, &serviceUuid, sizeof(uint16_t));
			break;
		case 1:
//			header.sequence1 = sequence;                                  // 2 bits sequence
//			header.reserved = ;                                           // 2 bits reserved
			header.deviceToken =      (serviceUuid >> (16-2-2-8)) & 0xFF; // 8 bits device token
			encryptedPayloadRC5[0] = ((serviceUuid >> (16-2-2-8-4)) & 0x0F) << (16-4);    // Last 4 bits of this service UUID are first 4 bits of encrypted payload[0].
			memcpy(nonce+2, &serviceUuid, sizeof(uint16_t));
			break;
		case 2:
//			header.sequence2 = sequence;
			encryptedPayloadRC5[0] += ((serviceUuid >> (16-2-12)) & 0x0FFF) << (16-4-12); // Bits 2 - 13 of this service UUID are last 12 bits of encrypted payload[0].
			encryptedPayloadRC5[1] = ((serviceUuid >> (16-2-12-2)) & 0x03) << (16-2);     // Last 2 bits of this service UUID are first 2 bits of encrypted payload[1].
			memcpy(nonce+4, &serviceUuid, sizeof(uint16_t));
			break;
		case 3:
//			header.sequence3 = sequence;
			encryptedPayloadRC5[1] += ((serviceUuid >> (16-2-14)) & 0x3FFF) << (16-2-14); // Last 14 bits of this service UUID are last 14 bits of encrypted payload[1].
			memcpy(nonce+6, &serviceUuid, sizeof(uint16_t));
			break;
		}
	}
	for (int i=0; i < CMD_ADV_NUM_SERVICES_16BIT; ++i) {
		if (!foundSequences[i]) {
			if (scannedDevice->rssi > RSSI_LOG_THRESHOLD) {
				LOGCommandAdvVerbose("Missing UUID with sequence %i", i);
			}
			return;
		}
	}

	if (header.sphereId != _sphereId) {
		if (scannedDevice->rssi > RSSI_LOG_THRESHOLD) {
			LOGCommandAdvVerbose("Wrong sphereId got=%u stored=%u", header.sphereId, _sphereId);
		}
		return;
	}

	cs_data_t nonceData;
	nonceData.data = nonce;
	nonceData.len = sizeof(nonce);

	uint16_t decryptedPayloadRC5[2];
	bool validated = handleEncryptedCommandPayload(scannedDevice, header, nonceData, services128bit, encryptedPayloadRC5, decryptedPayloadRC5);
	if (validated) {
		handleDecryptedRC5Payload(scannedDevice, header, decryptedPayloadRC5);
	}
}

int CommandAdvHandler::checkSimilarCommand(uint8_t deviceToken, cs_data_t& encryptedData, uint16_t encryptedRC5, uint16_t& decryptedRC5) {
	assert(encryptedData.len == CMD_ADC_ENCRYPTED_DATA_SIZE, "Invalid size");
	int index = -1;
	for (int i=0; i<CMD_ADV_MAX_CLAIM_COUNT; ++i) {
		if (_claims[i].deviceToken == deviceToken) {
			if (_claims[i].timeoutCounter && _claims[i].encryptedRC5 == encryptedRC5 && memcmp(_claims[i].encryptedData, encryptedData.data, CMD_ADC_ENCRYPTED_DATA_SIZE) == 0) {
				LOGCommandAdvVerbose("Ignore similar payload");
				// Since all encrypted data is similar: set cached decrypted RC5.
				// The RC5 data does not use access level, so changing access level does not do anything.
				decryptedRC5 = _claims[i].decryptedRC5;
				return -2;
			}
			index = i;
			break;
		}
	}
	return index;
}

bool CommandAdvHandler::claim(uint8_t deviceToken, cs_data_t& encryptedData, uint16_t encryptedRC5, uint16_t decryptedRC5, int index) {
	assert(encryptedData.len == CMD_ADC_ENCRYPTED_DATA_SIZE, "Invalid size");
	assert(index > -2 && index < CMD_ADV_MAX_CLAIM_COUNT, "Invalid index");
	if (index == -1) {
		// Check if there's a spot to claim.
		for (int i=0; i<CMD_ADV_MAX_CLAIM_COUNT; ++i) {
			if (!_claims[i].timeoutCounter) {
				index = i;
				break;
			}
		}
	}
	if (index != -1) {
		// Claim the spot
		_claims[index].deviceToken = deviceToken;
		_claims[index].timeoutCounter = CMD_ADV_CLAIM_TIME_MS / TICK_INTERVAL_MS;
		memcpy(_claims[index].encryptedData, encryptedData.data, CMD_ADC_ENCRYPTED_DATA_SIZE);
		_claims[index].encryptedRC5 = encryptedRC5;
		_claims[index].decryptedRC5 = decryptedRC5;
		return true;
	}
	LOGCommandAdvDebug("No more claim spots");
	return false;
}

void CommandAdvHandler::tickClaims() {
	for (int i=0; i<CMD_ADV_MAX_CLAIM_COUNT; ++i) {
		if (_claims[i].timeoutCounter) {
			--_claims[i].timeoutCounter;
		}
	}
}

bool CommandAdvHandler::handleEncryptedCommandPayload(scanned_device_t* scannedDevice, const command_adv_header_t& header, const cs_data_t& nonce, cs_data_t& encryptedPayload, uint16_t encryptedPayloadRC5[2], uint16_t decryptedPayloadRC5[2]) {
	int claimIndex = checkSimilarCommand(header.deviceToken, encryptedPayload, encryptedPayloadRC5[1], decryptedPayloadRC5[1]);
	if (claimIndex == -2) {
		LOGCommandAdvVerbose("Ignore already handled command");
		// Command was already validated previous time.
		// Since the RC5 data does not use the access level, it can safely be handled.
		return true;
	}

	EncryptionAccessLevel accessLevel;
	switch (header.accessLevel) {
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
		LOGw("Invalid access level %u", header.accessLevel);
		return false;
	}

	// TODO: can decrypt to same buffer?
	cs_ret_code_t decryptionResult = ERR_NO_ACCESS;
	uint8_t decryptedData[AES_BLOCK_SIZE];
	uint8_t key[ENCRYPTION_KEY_LENGTH];
	cs_buffer_size_t decryptedPayloadSize;
	if (KeysAndAccess::getInstance().getKey(accessLevel, key, sizeof(key))) {
		decryptionResult = AES::getInstance().decryptCtr(
				cs_data_t(key, sizeof(key)),
				nonce,
				encryptedPayload,
				cs_data_t(),
				cs_data_t(decryptedData, sizeof(decryptedData)),
				decryptedPayloadSize
		);
	}
	if (decryptionResult != ERR_SUCCESS) {
		LOGCommandAdvVerbose("Decrypt failed");
		return false;
	}

#ifdef COMMAND_ADV_VERBOSE
	_log(SERIAL_DEBUG, false, "decrypted data: ");
	_logArray(SERIAL_DEBUG, true, decryptedData, 16);
#endif

	uint32_t validationTimestamp = (decryptedData[0] << 0) | (decryptedData[1] << 8) | (decryptedData[2] << 16) | (decryptedData[3] << 24);
	uint8_t typeInt = decryptedData[4];
	AdvCommandTypes type = (AdvCommandTypes)typeInt;
	uint16_t headerSize = sizeof(validationTimestamp) + sizeof(typeInt);
	uint16_t length = SOC_ECB_CIPHERTEXT_LENGTH - headerSize;
	uint8_t* commandData = decryptedData + headerSize;

	uint32_t timestamp = SystemTime::posix();
	LOGCommandAdvVerbose("validation=%u time=%u", validationTimestamp, timestamp);

	// For now, we also allow CAFEBABE as validation.
	bool validated = false;
	if (validationTimestamp == 0xCAFEBABE) {
		validated = true;
	}
	if (timestamp - 2*60 < validationTimestamp && validationTimestamp < timestamp + 2*60) {
		validated = true;
	}
	if (!validated) {
		return false;
	}
	if (!decryptRC5Payload(encryptedPayloadRC5, decryptedPayloadRC5)) {
		return false;
	}
	// Validated, so from here on, return true.

	// Claim only after validation
	if (!claim(header.deviceToken, encryptedPayload, encryptedPayloadRC5[1], decryptedPayloadRC5[1], claimIndex)) {
		return true;
	}

	cmd_source_with_counter_t source(cmd_source_t(CS_CMD_SOURCE_TYPE_BROADCAST, header.deviceToken), (decryptedPayloadRC5[0] >> 8) & 0xFF);
	TYPIFY(CMD_CONTROL_CMD) controlCmd;
	controlCmd.protocolVersion = CS_CONNECTION_PROTOCOL_VERSION;
	controlCmd.type = CTRL_CMD_UNKNOWN;
	controlCmd.accessLevel = accessLevel;
	controlCmd.data = commandData;
	controlCmd.size = length;

	LOGCommandAdvDebug("adv cmd type=%u deviceId=%u accessLvl=%u", type, header.deviceToken, accessLevel);
	if (!KeysAndAccess::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		LOGCommandAdvDebug("no access");
		return true;
	}


	switch (type) {
		case ADV_CMD_NOOP: {
			break;
		}
		case ADV_CMD_MULTI_SWITCH: {
			controlCmd.type = CTRL_CMD_MULTI_SWITCH;
			LOGCommandAdvDebug("send cmd type=%u source: type=%u id=%u count=%u", controlCmd.type, source.source.type, source.source.id, source.count);
			event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd), source);
			event.dispatch();
			break;
		}
		case ADV_CMD_SET_TIME: {
			uint8_t flags;
			size16_t flagsSize = sizeof(flags);
			size16_t setTimeSize = sizeof(uint32_t);
			size16_t setSunTimeSize = 6; // 2 uint24
			if (length < flagsSize + setTimeSize + setSunTimeSize) {
				return true;
			}
			// First get flags
			flags = commandData[0];

			// Set time, if valid.
			if (BLEutil::isBitSet(flags, 0)) {
				controlCmd.type = CTRL_CMD_SET_TIME;
				controlCmd.data = commandData + flagsSize;
				controlCmd.size = setTimeSize;
				LOGCommandAdvDebug("send cmd type=%u source: type=%u id=%u count=%u", controlCmd.type, source.source.type, source.source.id, source.count);
				event_t eventSetTime(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd), source);
				eventSetTime.dispatch();
			}

			// Set sun time, if valid.
			if (BLEutil::isBitSet(flags, 1)) {
				sun_time_t sunTime;
				sunTime.sunrise = (commandData[flagsSize + setTimeSize + 0] << 0) + (commandData[flagsSize + setTimeSize + 1] << 8) + (commandData[flagsSize + setTimeSize + 2] << 16);
				sunTime.sunset  = (commandData[flagsSize + setTimeSize + 3] << 0) + (commandData[flagsSize + setTimeSize + 4] << 8) + (commandData[flagsSize + setTimeSize + 5] << 16);
				LOGCommandAdvDebug("sunTime: %u %u", sunTime.sunrise, sunTime.sunset);
				controlCmd.type = CTRL_CMD_SET_SUN_TIME;
				controlCmd.data = (buffer_ptr_t)&sunTime;
				controlCmd.size = sizeof(sunTime);
				event_t eventSetSunTime(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd), source);
				eventSetSunTime.dispatch();
			}
			break;
		}
		case ADV_CMD_SET_BEHAVIOUR_SETTINGS: {
			size16_t requiredSize = sizeof(TYPIFY(STATE_BEHAVIOUR_SETTINGS));
			if (length < requiredSize) {
				return true;
			}
			// Set state.
			cs_state_data_t stateData(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, commandData, requiredSize);
			State::getInstance().set(stateData);
			// Send over mesh.
			event_t meshCmd(CS_TYPE::CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS, commandData, requiredSize);
			meshCmd.dispatch();
			break;
		}
		case ADV_CMD_UPDATE_TRACKED_DEVICE: {
			size16_t requiredSize = sizeof(update_tracked_device_packet_t);
			if (length < requiredSize) {
				return true;
			}
			TYPIFY(CMD_UPDATE_TRACKED_DEVICE) eventData;
			eventData.accessLevel = accessLevel;
			eventData.data = *((update_tracked_device_packet_t*)commandData);
			event_t event(CS_TYPE::CMD_UPDATE_TRACKED_DEVICE, &eventData, sizeof(eventData));
			event.dispatch();
			break;
		}
		default:
			LOGd("Unkown adv cmd: %u", type);
			return true;
	}
	return true;
}

bool CommandAdvHandler::decryptRC5Payload(uint16_t encryptedPayload[2], uint16_t decryptedPayload[2]) {
	LOGCommandAdvVerbose("encrypted RC5=[%u %u]", encryptedPayload[0], encryptedPayload[1]);
	bool success = RC5::getInstance().decrypt(encryptedPayload, sizeof(uint16_t) * 2, decryptedPayload, sizeof(uint16_t) * 2); // Can't use sizeof(encryptedPayload) as that returns size of pointer.
	LOGCommandAdvVerbose("decrypted RC5=[%u %u]", decryptedPayload[0], decryptedPayload[1]);
	return success;
}

void CommandAdvHandler::handleDecryptedRC5Payload(scanned_device_t* scannedDevice, const command_adv_header_t& header, uint16_t decryptedPayload[2]) {
	adv_background_t backgroundAdv;
	switch (header.protocol) {
	case 0:
		backgroundAdv.protocol = 0;
		break;
	default:
		return;
	}
	backgroundAdv.sphereId = header.sphereId;
	backgroundAdv.data = (uint8_t*)(decryptedPayload);
	backgroundAdv.dataSize = sizeof(uint16_t) * 2;
	backgroundAdv.macAddress = scannedDevice->address;
	backgroundAdv.rssi = scannedDevice->rssi;

	event_t event(CS_TYPE::EVT_ADV_BACKGROUND, &backgroundAdv, sizeof(backgroundAdv));
	EventDispatcher::getInstance().dispatch(event);
}

EncryptionAccessLevel CommandAdvHandler::getRequiredAccessLevel(const AdvCommandTypes type) {
	switch (type) {
		case ADV_CMD_NOOP:
		case ADV_CMD_MULTI_SWITCH:
		case ADV_CMD_UPDATE_TRACKED_DEVICE:
			return BASIC;
		case ADV_CMD_SET_TIME:
		case ADV_CMD_SET_BEHAVIOUR_SETTINGS:
			return MEMBER;
		case ADV_CMD_UNKNOWN:
			return NOT_SET;
	}
	return NOT_SET;
}

void CommandAdvHandler::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			TYPIFY(EVT_DEVICE_SCANNED)* scannedDevice = (TYPIFY(EVT_DEVICE_SCANNED)*)event.data;
			parseAdvertisement(scannedDevice);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			tickClaims();
			break;
		}
		default:
			break;
	}
}
