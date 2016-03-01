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
#include <drivers/cs_Storage.h>

/**Configuration types
 *
 * Use in the characteristic to read and write configurations in <CommonService>.
 */
enum ConfigurationTypes {
	CONFIG_NAME_UUID                        = Configuration_Base,
	CONFIG_DEVICE_TYPE_UUID                 = 0x01,
	CONFIG_ROOM_UUID                        = 0x02,
	CONFIG_FLOOR_UUID                       = 0x03,
	CONFIG_NEARBY_TIMEOUT_UUID              = 0x04,
	CONFIG_PWM_FREQ_UUID                    = 0x05,
	CONFIG_IBEACON_MAJOR                    = 0x06,
	CONFIG_IBEACON_MINOR                    = 0x07,
	CONFIG_IBEACON_UUID                     = 0x08,
	CONFIG_IBEACON_RSSI                     = 0x09,
	CONFIG_WIFI_SETTINGS                    = 0x0A, //! 10
	CONFIG_TX_POWER                         = 0x0B, //! 11
	CONFIG_ADV_INTERVAL                     = 0x0C, //! 12
	CONFIG_PASSKEY							= 0x0D, //! 13
	CONFIG_MIN_ENV_TEMP                     = 0x0E, //! 14
	CONFIG_MAX_ENV_TEMP                     = 0x0F, //! 15
	CONFIG_SCAN_DURATION                    = 0x10, //! 16
	CONFIG_SCAN_SEND_DELAY                  = 0x11, //! 17
	CONFIG_SCAN_BREAK_DURATION              = 0x12, //! 18
	CONFIG_BOOT_DELAY                       = 0x13, //! 19
	CONFIG_MAX_CHIP_TEMP                    = 0x14, //! 20
	CONFIG_SCAN_FILTER                      = 0x15, //! 21
	CONFIG_SCAN_FILTER_SEND_FRACTION        = 0x16, //! 22
	CONFIG_TYPES
};

using namespace BLEpp;

class Settings {

private:
	Settings();

	//! This class is singleton, deny implementation
	Settings(Settings const&);
	//! This class is singleton, deny implementation
	void operator=(Settings const &);

public:
	static Settings& getInstance() {
		static Settings instance;
		return instance;
	}

	//	void writeToStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
	void writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent = true);

	bool readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer);

	ps_configuration_t& getConfig();

	/** Get a handle to the persistent storage struct and load it from FLASH.
	 *
	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
	 * writing less than a minimal block strains the memory just as much as flashing the entire block.
	 * Hence, there is an entire struct that can be filled and flashed at once.
	 */
	void loadPersistentStorage();

	/** Save to FLASH.
	 */
	void savePersistentStorage();

//	void ConfigHelper::enable(ps_storage_id id, uint16_t size) {
//		Storage::getInstance().getHandle(id, _storageHandles[id]);
//		loadPersistentStorage(_storageHandles[id], );
//	}

	/** Retrieve the Bluetooth name from the object representing the BLE stack.
	 *
	 * @return name of the device
	 */
	std::string & getBLEName();

	/** Write the Bluetooth name to the object representing the BLE stack.
	 *
	 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It
	 * has to be written to FLASH in that case.
	 */
	void setBLEName(const std::string &name, bool persistent = true);

protected:
	//	pstorage_handle_t _storageHandles[PS_ID_TYPES];
	//	ps_configuration_t* _storageStructs[PS_ID_TYPES];

	//! handle to storage (required to write to and read from FLASH)
	pstorage_handle_t _storageHandle;

	//! struct that storage object understands
	ps_configuration_t _storageStruct;

	//! non-persistent configuration options
	std::string _wifiSettings;

	/*
	 * Writes value from storage to streambuffer
	 */
	bool getUint16(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint16_t defaultValue);
	bool getInt8(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, int32_t value, int8_t defaultValue);
	bool getUint8(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint8_t defaultValue);

	bool setUint16(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target);
	bool setInt8(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, int32_t& target);
	bool setUint8(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target);
};


