/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 19, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */



#include <processing/cs_BackgroundAdvHandler.h>
#include <ble/cs_Nordic.h>
#include <logging/cs_Logger.h>
#include <encryption/cs_RC5.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandHandler.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <util/cs_Utils.h>
#include <ble/cs_BleConstants.h>

#define LOGBackgroundAdvDebug LOGnone
#define BACKGROUND_ADV_VERBOSE false


#if BACKGROUND_ADV_VERBOSE == true
#define LOGBackgroundAdvVerbose LOGd
#else
#define LOGBackgroundAdvVerbose LOGnone
#endif


#define BACKGROUND_SERVICES_MASK_TYPE 0x01
#define BACKGROUND_SERVICES_MASK_HEADER_LEN 3
#define BACKGROUND_SERVICES_MASK_LEN 16

BackgroundAdvertisementHandler::BackgroundAdvertisementHandler() {
	State::getInstance().get(CS_TYPE::CONFIG_SPHERE_ID, &_sphereId, sizeof(_sphereId));
	EventDispatcher::getInstance().addListener(this);
}

void BackgroundAdvertisementHandler::parseServicesAdvertisement(scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t serviceUuids;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE, scannedDevice->data, scannedDevice->dataSize, &serviceUuids);
	if (errCode != ERR_SUCCESS) {
		return;
	}

	memcpy(_lastMacAddress, scannedDevice->address, MAC_ADDRESS_LEN);

	_lastBitmask[0] = 0;
	_lastBitmask[1] = 0;
//	memset(_lastBitmask, 0, sizeof(_lastBitmask));

	// Loop over 16 bit service UUIDs.
	for (uint8_t i = 0; i < serviceUuids.len / 2; ++i) {
		LOGBackgroundAdvVerbose("uuid=%u %u (0x%02X 0x%02X)", serviceUuids.data[2 * i + 0], serviceUuids.data[2 * i + 1], serviceUuids.data[2 * i + 0], serviceUuids.data[2 * i + 1]);
		uint8_t index = serviceUuids.data[2 * i + 1];
		uint8_t bitPos = _uuidMap[index];
		if (bitPos == 255) {
			LOGBackgroundAdvVerbose("invalid bit pos %u for uuid %u", bitPos, index);
		}
		else {
			LOGBackgroundAdvVerbose("bit pos %u --> ind=%u shift=%u", bitPos, bitPos / 64, bitPos % 64);
			_lastBitmask[bitPos / 64] |= ((uint64_t)1) << (bitPos % 64);
		}
	}

	LOGBackgroundAdvVerbose("store bitmask 0x%08X%08X 0x%08X%08X", (uint32_t)(_lastBitmask[0] >> 32), (uint32_t)(_lastBitmask[0]), (uint32_t)(_lastBitmask[1] >> 32), (uint32_t)(_lastBitmask[1]));
}

void BackgroundAdvertisementHandler::parseAdvertisement(scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t manufacturerData;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, scannedDevice->data, scannedDevice->dataSize, &manufacturerData);
	if (errCode != ERR_SUCCESS) {
		return;
	}
	if (manufacturerData.len != BACKGROUND_SERVICES_MASK_HEADER_LEN + BACKGROUND_SERVICES_MASK_LEN) {
		return;
	}

	uint16_t companyId = *((uint16_t*)(manufacturerData.data));
	if (companyId != BleCompanyId::Apple) {
		return;
	}
	uint8_t appleAdvType = manufacturerData.data[2];
	if (appleAdvType != BACKGROUND_SERVICES_MASK_TYPE) {
		return;
	}

	uint8_t* servicesMask; // This is a mask of 128 bits.
	servicesMask = manufacturerData.data + BACKGROUND_SERVICES_MASK_HEADER_LEN;

#if BACKGROUND_ADV_VERBOSE == true
	_log(SERIAL_DEBUG, false, "rssi=%i servicesMask: ", scannedDevice->rssi);
	_logArray(SERIAL_DEBUG, true, servicesMask, BACKGROUND_SERVICES_MASK_LEN);
#endif

	// Put the data into large integers, so we can perform shift operations.
	// Can't cast, as the data is big endian (MSB).
//	uint64_t left = *((uint64_t*)servicesMask);
//	uint64_t right = *((uint64_t*)(servicesMask + 8));
	uint64_t left =
			((uint64_t)servicesMask[0] << (7*8)) +
			((uint64_t)servicesMask[1] << (6*8)) +
			((uint64_t)servicesMask[2] << (5*8)) +
			((uint64_t)servicesMask[3] << (4*8)) +
			((uint64_t)servicesMask[4] << (3*8)) +
			((uint64_t)servicesMask[5] << (2*8)) +
			((uint64_t)servicesMask[6] << (1*8)) +
			((uint64_t)servicesMask[7] << (0*8));
	uint64_t right =
				((uint64_t)servicesMask[0+8] << (7*8)) +
				((uint64_t)servicesMask[1+8] << (6*8)) +
				((uint64_t)servicesMask[2+8] << (5*8)) +
				((uint64_t)servicesMask[3+8] << (4*8)) +
				((uint64_t)servicesMask[4+8] << (3*8)) +
				((uint64_t)servicesMask[5+8] << (2*8)) +
				((uint64_t)servicesMask[6+8] << (1*8)) +
				((uint64_t)servicesMask[7+8] << (0*8));
	LOGBackgroundAdvVerbose("left=0x%08X%08X right=0x%08X%08X", (uint32_t)(left >> 32), (uint32_t)(left), (uint32_t)(right >> 32), (uint32_t)(right));

	if (memcmp(_lastMacAddress, scannedDevice->address, MAC_ADDRESS_LEN) == 0) {
		// TODO: make sure the right bits go to the right place.
		left |= _lastBitmask[1];
		right |= _lastBitmask[0];
		LOGBackgroundAdvVerbose("Use last bitmask left=0x%08X%08X right=0x%08X%08X", (uint32_t)(left >> 32), (uint32_t)(left), (uint32_t)(right >> 32), (uint32_t)(right));
	}

	// Clear any cached bitmask.
	_lastBitmask[0] = 0;
	_lastBitmask[1] = 0;



	// Divide the data into 3 parts, and do a bitwise majority vote, to correct for errors.
	// Each part is 42 bits.
	uint64_t part1 = (left >> (64-42)) & 0x03FFFFFFFFFF; // First 42 bits from left.
	uint64_t part2 = ((left & 0x3FFFFF) << 20) | ((right >> (64-20)) & 0x0FFFFF); // Last 64-42=22 bits from left, and first 42−(64−42)=20 bits from right.
	uint64_t part3 = (right >> 2) & 0x03FFFFFFFFFF; // Bits 21-62 from right.
	uint64_t result = ((part1 & part2) | (part2 & part3) | (part1 & part3)); // The majority vote
	LOGBackgroundAdvVerbose("part1=0x%08X%08X part2=0x%08X%08X part3=0x%08X%08X result=0x%08X%08X",
			(uint32_t)(part1 >> 32), (uint32_t)(part1),
			(uint32_t)(part2 >> 32), (uint32_t)(part2),
			(uint32_t)(part3 >> 32), (uint32_t)(part3),
			(uint32_t)(result >> 32), (uint32_t)(result));

	// Parse the resulting data.
	uint8_t protocol = (result >> (42-2)) & 0x03;

	// For protocol 1:
	//   uint8_t protocol     : 2;
	//   uint24_t deviceToken : 24;
	//   uint16_t reserved    : 16;
	if (protocol == 1) {
		TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1) parsed;
		parsed.rssi = scannedDevice->rssi;
		parsed.macAddress = scannedDevice->address;
		parsed.deviceToken[0] = (result >> (42-2-8)) & 0xFF;
		parsed.deviceToken[1] = (result >> (42-2-8-8)) & 0xFF;
		parsed.deviceToken[2] = (result >> (42-2-8-8-8)) & 0xFF;
		LOGBackgroundAdvDebug("v1 token=[%u %u %u]", parsed.deviceToken[0], parsed.deviceToken[1], parsed.deviceToken[2]);
		event_t event(CS_TYPE::EVT_ADV_BACKGROUND_PARSED_V1, &parsed, sizeof(parsed));
		EventDispatcher::getInstance().dispatch(event);
		return;
	}

	// For protocol 0:
	//	uint8_t protocol : 2;
	//	uint8_t sphereId : 8;
	//	uint16_t encryptedData[2];
	adv_background_t backgroundAdvertisement;
	uint16_t encryptedPayload[2];
	backgroundAdvertisement.protocol = protocol;
	backgroundAdvertisement.sphereId = (result >> (42-2-8)) & 0xFF;
	encryptedPayload[0] = (result >> (42-2-8-16)) & 0xFFFF;
	encryptedPayload[1] = (result >> (42-2-8-32)) & 0xFFFF;
	backgroundAdvertisement.macAddress = scannedDevice->address;
	backgroundAdvertisement.rssi = scannedDevice->rssi;

	if (backgroundAdvertisement.protocol != 0 || backgroundAdvertisement.sphereId != _sphereId) {
		LOGBackgroundAdvVerbose("wrong protocol (%u) or sphereId (%u vs %u)", backgroundAdvertisement.protocol, backgroundAdvertisement.sphereId, _sphereId);
		return;
	}

	LOGBackgroundAdvVerbose("encrypted=[%u %u]", encryptedPayload[0], encryptedPayload[1]);
	// TODO: can decrypt to same buffer?
	uint16_t decryptedPayload[2];
	bool success = RC5::getInstance().decrypt(encryptedPayload, sizeof(encryptedPayload), decryptedPayload, sizeof(decryptedPayload));
	if (!success) {
		return;
	}
	LOGBackgroundAdvVerbose("decrypted=[%u %u]", decryptedPayload[0], decryptedPayload[1]);

	// Validate
	uint32_t timestamp = SystemTime::posix();
	uint16_t timestampRounded = (timestamp >> 7) & 0x0000FFFF;
	LOGBackgroundAdvVerbose("validation=%u time=%u rounded=%u", decryptedPayload[0], timestamp, timestampRounded);

	// For now, we also allow CAFE as validation.
	bool validated = false;
	if (decryptedPayload[0] == 0xCAFE) {
		validated = true;
	}
	// Needs to work with overflows too, so can't use "smaller than".
	if (
			(decryptedPayload[0] == timestampRounded - 1) ||
			(decryptedPayload[0] == timestampRounded) ||
			(decryptedPayload[0] == timestampRounded + 1)) {
		validated = true;
	}
	if (!validated) {
		return;
	}

	backgroundAdvertisement.data = (uint8_t*)decryptedPayload;
	backgroundAdvertisement.dataSize = sizeof(decryptedPayload);

	handleBackgroundAdvertisement(&backgroundAdvertisement);
}

void BackgroundAdvertisementHandler::handleBackgroundAdvertisement(adv_background_t* backgroundAdvertisement) {
	if (backgroundAdvertisement->dataSize != sizeof(uint16_t) * 2) {
		return;
	}
	uint16_t* decryptedPayload = (uint16_t*)(backgroundAdvertisement->data);
#if BACKGROUND_ADV_VERBOSE == true
	_log(SERIAL_DEBUG, false, "bg adv: ");
	_log(SERIAL_DEBUG, false, "protocol=%u sphereId=%u rssi=%i ", backgroundAdvertisement->protocol, backgroundAdvertisement->sphereId, backgroundAdvertisement->rssi);
	_log(SERIAL_DEBUG, false, "payload=[%u %u] address=", decryptedPayload[0], decryptedPayload[1]);
	BLEutil::printAddress(backgroundAdvertisement->macAddress, BLE_GAP_ADDR_LEN);
#endif


	TYPIFY(EVT_ADV_BACKGROUND_PARSED) parsed;
	parsed.macAddress = backgroundAdvertisement->macAddress;

	//struct __attribute__((__packed__)) BackgroundAdvertisementPayload {
	//	uint8_t locationId : 6;
	//	uint8_t profileId : 3;
	//	int8_t rssiOffset : 4;
	//	uint8_t flags : 3;
	//};
	parsed.locationId =  (decryptedPayload[1] >> (16-6)) & 0x3F;
	parsed.profileId =   (decryptedPayload[1] >> (16-6-3)) & 0x07;
	uint8_t rssiOffset = (decryptedPayload[1] >> (16-6-3-4)) & 0x0F;
	parsed.flags =       (decryptedPayload[1] >> (16-6-3-4-3)) & 0x07;
	parsed.adjustedRssi = getAdjustedRssi(backgroundAdvertisement->rssi, rssiOffset);
	LOGBackgroundAdvDebug("validation=%u locationId=%u profileId=%u rssiOffset=%u flags=%u rssi=%i adjusted_rssi=%i", decryptedPayload[0], parsed.locationId, parsed.profileId, rssiOffset, parsed.flags, backgroundAdvertisement->rssi, parsed.adjustedRssi);
	event_t event(CS_TYPE::EVT_ADV_BACKGROUND_PARSED, &parsed, sizeof(parsed));
	EventDispatcher::getInstance().dispatch(event);
}

int8_t BackgroundAdvertisementHandler::getAdjustedRssi(int16_t rssi, int16_t rssiOffset) {
	rssi += (rssiOffset - 8) * 2;
	if (rssi > -1) {
		rssi = -1;
	}
	if (rssi < -127) {
		rssi = -127;
	}
	return rssi;
}

void BackgroundAdvertisementHandler::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			TYPIFY(EVT_DEVICE_SCANNED)* scannedDevice = (TYPIFY(EVT_DEVICE_SCANNED)*)event.data;
			parseAdvertisement(scannedDevice);
			parseServicesAdvertisement(scannedDevice);
			break;
		}
		case CS_TYPE::EVT_ADV_BACKGROUND: {
			TYPIFY(EVT_ADV_BACKGROUND)* backgroundAdv = (TYPIFY(EVT_ADV_BACKGROUND)*)event.data;
			handleBackgroundAdvertisement(backgroundAdv);
			break;
		}
		default:
			break;
	}
}
