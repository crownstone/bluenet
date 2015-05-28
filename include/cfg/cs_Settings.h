/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 11, 2015
 * License: LGPLv3+
 */
#pragma once

#include <drivers/cs_Serial.h>
#include <structs/buffer/cs_StreamBuffer.h>

#include <ble/cs_Stack.h>
#include <util/cs_Utils.h>

#include <events/cs_EventTypes.h>
#include <events/cs_EventDispatcher.h>

#include <ble/cs_UUID.h>

/* Configuration types
 *
 * Use in the characteristic to read and write configurations in <CommonService>.
 */
enum ConfigurationTypes {
	CONFIG_NAME_UUID                        = Configuration_Base,
	CONFIG_DEVICE_TYPE_UUID                 = 0x1,
	CONFIG_ROOM_UUID                        = 0x2,
	CONFIG_FLOOR_UUID                       = 0x3,
	CONFIG_NEARBY_TIMEOUT_UUID              = 0x4,
	CONFIG_PWM_FREQ_UUID                    = 0x5,
	CONFIG_IBEACON_MAJOR                    = 0x6,
	CONFIG_IBEACON_MINOR                    = 0x7,
	CONFIG_IBEACON_UUID                     = 0x8,
	CONFIG_IBEACON_RSSI                     = 0x9,
	CONFIG_WIFI_SETTINGS                    = 0xA,
	CONFIG_TYPES
};

using namespace BLEpp;

class Settings {

private:
	Settings() {
		Storage::getInstance().getHandle(PS_ID_CONFIGURATION, _storageHandle);
		loadPersistentStorage();
	};

	// This class is singleton, deny implementation
	Settings(Settings const&);
	// This class is singleton, deny implementation
	void operator=(Settings const &);

public:
	static Settings& getInstance() {
		static Settings instance;
		return instance;
	}

	void writeToStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
		uint8_t length = streamBuffer->length();
		uint8_t* payload = streamBuffer->payload();

		switch(type) {
		case CONFIG_NAME_UUID: {
			LOGd("Write name");
			std::string str = std::string((char*)payload, length);
			LOGd("Set name to: %s", str.c_str());
			setBLEName(str);
//			Storage::setString(str, _storageStruct.device_name);
//			savePersistentStorage();
			break;
		}
		case CONFIG_FLOOR_UUID: {
			LOGd("Set floor level");
			if (length != 1) {
				LOGw("We do not account for buildings of more than 255 floors yet");
				return;
			}
			uint8_t floor = payload[0];
			LOGi("Set floor to %i", floor);
			Storage::setUint8(floor, _storageStruct.floor);
			savePersistentStorage();
			break;
		}
		case CONFIG_NEARBY_TIMEOUT_UUID: {
			if (length != 2) {
				LOGw("Nearby timeout is of type uint16");
				return;
			}
			uint16_t counts = ((uint16_t*)payload)[0]; //TODO: other byte order?
			LOGd("setNearbyTimeout(%i)", counts);
			Storage::setUint16(counts, _storageStruct.nearbyTimeout);
			savePersistentStorage();
	//		setNearbyTimeout(counts);
			// TODO: write to persistent storage and trigger update event
			break;
		}
#if IBEACON==1
		case CONFIG_IBEACON_MAJOR: {
			if (length != 2) {
				LOGw("We do not account for a value of more than %d");
				return;
			}
//			uint16_t major;
//			popUint16(major, payload);
			uint16_t major = ((uint16_t*)payload)[0];
			LOGi("Set major to %d", major);
			Storage::setUint16(major, (uint32_t&)_storageStruct.beacon.major);
			savePersistentStorage();

			EventDispatcher::getInstance().dispatch((EventType)type, &_storageStruct.beacon.major, 4);
			break;
		}
		case CONFIG_IBEACON_MINOR: {
			if (length != 2) {
				LOGw("We do not account for a value of more than %d");
				return;
			}
//			uint16_t minor;
//			popUint16(minor, payload);
			uint16_t minor = ((uint16_t*)payload)[0];
			LOGi("Set minor to %d", minor);
			Storage::setUint16(minor, (uint32_t&)_storageStruct.beacon.minor);
			savePersistentStorage();

			EventDispatcher::getInstance().dispatch((EventType)type, &_storageStruct.beacon.minor, 4);
			break;
		}
		case CONFIG_IBEACON_UUID: {
			if (length != 16) {
				LOGw("Expected 16 bytes for UUID");
			}
			Storage::setArray<uint8_t>(payload, _storageStruct.beacon.uuid.uuid128, 16);
			savePersistentStorage();

			EventDispatcher::getInstance().dispatch((EventType)type, &_storageStruct.beacon.uuid.uuid128, 16);
			break;
		}
		case CONFIG_IBEACON_RSSI: {
			if (length != 1) {
				LOGw("We do not account for a value of more than 255");
			}
			int8_t rssi = payload[0];
			LOGi("Set beacon rssi to %d", rssi);
			Storage::setInt8(rssi, (int32_t&)_storageStruct.beacon.rssi);
			savePersistentStorage();

			EventDispatcher::getInstance().dispatch((EventType)type, &_storageStruct.beacon.rssi, 1);
			break;
		}
#endif
		case CONFIG_WIFI_SETTINGS: {
			LOGi("Temporarily store wifi settings");
			// max length '{ "ssid": "32 bytes", "key": "32 bytes"}', 64+24 bytes = 88 bytes
			if (length > 88) {
				LOGe("Wifi settings string too long");
				break;
			}
			_wifiSettings = std::string((char*)payload, length);
			LOGi("Stored wifi settings [%i]: %s", length, _wifiSettings.c_str());
			break;
		}
		default:
			LOGw("There is no such configuration type (%i)! Or not yet implemented!", type);
		}
	}

	bool readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
		switch(type) {
		case CONFIG_NAME_UUID: {
			LOGd("Read name");
			std::string str = getBLEName();
			streamBuffer->fromString(str);
			streamBuffer->setType(type);

			LOGd("Name read %s", str.c_str());

			return true;
		}
		case CONFIG_FLOOR_UUID: {
			LOGd("Read floor");
			loadPersistentStorage();
			uint8_t plen = 1;
			uint8_t payload[plen];
			Storage::getUint8(_storageStruct.floor, payload[0], 0);
			streamBuffer->setPayload(payload, plen);
			streamBuffer->setType(type);

			LOGd("Floor level set in payload: %i with len %i", streamBuffer->payload()[0], streamBuffer->length());

			return true;
		}
#if IBEACON==1
		case CONFIG_IBEACON_MAJOR: {
			LOGd("Read major");
			loadPersistentStorage();
			uint8_t plen = 1;
			uint16_t payload[plen];
			Storage::getUint16(_storageStruct.beacon.major, payload[0], BEACON_MAJOR);
			streamBuffer->setPayload((uint8_t*)payload, plen*sizeof(uint16_t));
			streamBuffer->setType(type);

			LOGd("Major value set in payload: %d with len %d", streamBuffer->payload()[0], streamBuffer->length());
			return true;
		}
		case CONFIG_IBEACON_MINOR: {
			LOGd("Read minor");
			loadPersistentStorage();
			uint8_t plen = 1;
			uint16_t payload[plen];
			Storage::getUint16(_storageStruct.beacon.minor, payload[0], BEACON_MINOR);
			streamBuffer->setPayload((uint8_t*)payload, plen*sizeof(uint16_t));
			streamBuffer->setType(type);

			LOGd("Minor value set in payload: %d with len %d", streamBuffer->payload()[0], streamBuffer->length());
			return true;
		}
		case CONFIG_IBEACON_UUID: {
			LOGd("Read UUID");
			loadPersistentStorage();
			uint8_t plen = 16;
			uint8_t payload[plen];
			Storage::getArray<uint8_t>(_storageStruct.beacon.uuid.uuid128, payload, ((ble_uuid128_t)UUID(BEACON_UUID)).uuid128, plen);
			streamBuffer->setPayload(payload, plen);
			streamBuffer->setType(type);

			LOGd("UUID set in payload .. with len %d", streamBuffer->length());
			return true;
		}
		case CONFIG_IBEACON_RSSI: {
			LOGd("Read tx power");
			loadPersistentStorage();
			uint8_t plen = 1;
			int8_t payload[plen];
			Storage::getInt8(_storageStruct.beacon.rssi, payload[0], BEACON_RSSI);
			streamBuffer->setPayload((uint8_t*)payload, plen);
			streamBuffer->setType(type);

			LOGd("Beacon rssi value set in payload: %d with len %d", payload[0], streamBuffer->length());
			return true;
		}
#endif
		case CONFIG_WIFI_SETTINGS: {
			LOGd("Read wifi settings. Does reset it.");
			// copy string, because we clear it on read
			std::string str;
			if (_wifiSettings == "") {
				str = "{}";
			} else {
				str = _wifiSettings;
			}
			streamBuffer->fromString(str);
			streamBuffer->setType(type);
			_wifiSettings = "";
			LOGd("Wifi settings read");
			return true;
		}
		default: {
			LOGd("There is no such configuration type (%i), or not yet implemented.", type);
		}
		}
		return false;
	}

	ps_configuration_t& getConfig() {
		return _storageStruct;
	}

	/* Get a handle to the persistent storage struct and load it from FLASH.
	 *
	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
	 * writing less than a minimal block strains the memory just as much as flashing the entire block.
	 * Hence, there is an entire struct that can be filled and flashed at once.
	 */
	void loadPersistentStorage() {
		Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
	}

	/* Save to FLASH.
	 */
	void savePersistentStorage() {
		Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
	}

//	void ConfigHelper::enable(ps_storage_id id, uint16_t size) {
//		Storage::getInstance().getHandle(id, _storageHandles[id]);
//		loadPersistentStorage(_storageHandles[id], );
//	}

	/* Retrieve the Bluetooth name from the object representing the BLE stack.
	 *
	 * @return name of the device
	 */
	std::string & getBLEName() {
		loadPersistentStorage();
		std::string& _name = BLEpp::Nrf51822BluetoothStack::getInstance().getDeviceName();
		Storage::getString(_storageStruct.device_name, _name, _name);
		return _name;
	}

	/* Write the Bluetooth name to the object representing the BLE stack.
	 *
	 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It
	 * has to be written to FLASH in that case.
	 */
	void setBLEName(const std::string &name) {
		if (name.length() > 31) {
			log(ERROR, MSG_NAME_TOO_LONG);
			return;
		}
		BLEpp::Nrf51822BluetoothStack::getInstance().updateDeviceName(name);
		Storage::setString(name, _storageStruct.device_name);
		savePersistentStorage();
	}

protected:
//	pstorage_handle_t _storageHandles[PS_ID_TYPES];
//	ps_configuration_t* _storageStructs[PS_ID_TYPES];

	// handle to storage (required to write to and read from FLASH)
	pstorage_handle_t _storageHandle;

	// struct that storage object understands
	ps_configuration_t _storageStruct;

	// non-persistent configuration options
	std::string _wifiSettings;
};


