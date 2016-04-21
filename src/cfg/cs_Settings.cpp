/** Store settings in RAM or persistent memory
 *
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Sep 22, 2015
 * License: LGPLv3+
 */

#include "cfg/cs_Settings.h"


using namespace BLEpp;

Settings::Settings() {
	Storage::getInstance().getHandle(PS_ID_CONFIGURATION, _storageHandle);
	loadPersistentStorage();

	LOGi("load state vars");
	Storage::getInstance().getHandle(PS_ID_STATE, _stateVarsHandle);
	readStateVars();
};

//	void writeToStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
void Settings::writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent) {
	//		uint8_t length = streamBuffer->length();
	//		uint8_t* payload = streamBuffer->payload();

	switch(type) {
	case CONFIG_NAME_UUID: {
		LOGd("Write name");
		std::string str = std::string((char*)payload, length);
		LOGd("Set name to: %s", str.c_str());
		setBLEName(str);
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
		if (persistent) {
			savePersistentStorage();
		}
		break;
	}
	case CONFIG_NEARBY_TIMEOUT_UUID: {
		setUint16(type, payload, length, persistent, _storageStruct.nearbyTimeout);
		break;
	}
#if IBEACON==1 || DEVICE_TYPE==DEVICE_DOBEACON
	case CONFIG_IBEACON_MAJOR: {
		setUint16(type, payload, length, persistent, (uint32_t&)_storageStruct.beacon.major);
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		setUint16(type, payload, length, persistent, (uint32_t&)_storageStruct.beacon.minor);
		break;
	}
	case CONFIG_IBEACON_UUID: {
		if (length != 16) {
			LOGw("Expected 16 bytes for UUID, received: %d", length);
			return;
		}
		_log(INFO, "set uuid to: "); BLEutil::printArray(payload, 16);
		Storage::setArray<uint8_t>(payload, _storageStruct.beacon.uuid.uuid128, 16);
		if (persistent) {
			savePersistentStorage();
		}

		EventDispatcher::getInstance().dispatch(type, &_storageStruct.beacon.uuid.uuid128, 16);
		break;
	}
	case CONFIG_IBEACON_RSSI: {
		setInt8(type, payload, length, persistent, (int32_t&)_storageStruct.beacon.rssi);
		break;
	}
#endif
	case CONFIG_WIFI_SETTINGS: {
		LOGi("Temporarily store wifi settings");
		//! max length '{ "ssid": "32 bytes", "key": "32 bytes"}', 64+24 bytes = 88 bytes
		if (length > 88) {
			LOGe("Wifi settings string too long");
			break;
		}
		_wifiSettings = std::string((char*)payload, length);
		LOGi("Stored wifi settings [%i]: %s", length, _wifiSettings.c_str());
		break;
	}
	case CONFIG_TX_POWER: {
		setInt8(type, payload, length, persistent, _storageStruct.txPower);
		break;
	}
	case CONFIG_ADV_INTERVAL: {
		setUint16(type, payload, length, persistent, _storageStruct.advInterval);
		break;
	}
	case CONFIG_PASSKEY: {
		if (length != BLE_GAP_PASSKEY_LEN) {
			LOGw("Expected length %d for passkey", BLE_GAP_PASSKEY_LEN);
			return;
		}
		LOGi("Set passkey to %s", std::string((char*)payload, length).c_str());
		Storage::setArray(payload, _storageStruct.passkey, BLE_GAP_PASSKEY_LEN);
		if (persistent) {
			savePersistentStorage();
		}

		EventDispatcher::getInstance().dispatch(type, &_storageStruct.passkey, BLE_GAP_PASSKEY_LEN);
		break;
	}
	case CONFIG_MIN_ENV_TEMP: {
		setInt8(type, payload, length, persistent, _storageStruct.minEnvTemp);
		break;
	}
	case CONFIG_MAX_ENV_TEMP: {
		setInt8(type, payload, length, persistent, _storageStruct.maxEnvTemp);
		break;
	}
	case CONFIG_SCAN_DURATION:{
		setUint16(type, payload, length, persistent, _storageStruct.scanDuration);
		break;
	}
	case CONFIG_SCAN_SEND_DELAY:{
		setUint16(type, payload, length, persistent, _storageStruct.scanSendDelay);
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION:{
		setUint16(type, payload, length, persistent, _storageStruct.scanBreakDuration);
		break;
	}
	case CONFIG_BOOT_DELAY:{
		setUint16(type, payload, length, persistent, _storageStruct.bootDelay);
		break;
	}
	case CONFIG_MAX_CHIP_TEMP:{
		setInt8(type, payload, length, persistent, _storageStruct.maxChipTemp);
		break;
	}
	case CONFIG_SCAN_FILTER: {
		setUint8(type, payload, length, persistent, _storageStruct.scanFilter);
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION:{
		setUint16(type, payload, length, persistent, _storageStruct.scanFilterSendFraction);
		break;
	}
	case CONFIG_RESET_COUNTER: {
		setStateVar(type, payload, length, persistent, _stateVars.resetCounter);
		break;
	}
	default:
		LOGw("There is no such configuration type (%u).", type);
	}
}

bool Settings::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
	switch(type) {
	case CONFIG_NAME_UUID: {
		LOGd("Read name");
		std::string str = getBLEName();
		streamBuffer->fromString(str); //! TODO: can't we set this on buffer immediately?
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
#if IBEACON==1 || DEVICE_TYPE==DEVICE_DOBEACON
	case CONFIG_IBEACON_MAJOR: {
		LOGd("Read major");
		return getUint16(type, streamBuffer, _storageStruct.beacon.major, BEACON_MAJOR);
	}
	case CONFIG_IBEACON_MINOR: {
		LOGd("Read minor");
		return getUint16(type, streamBuffer, _storageStruct.beacon.minor, BEACON_MINOR);
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
		return getInt8(type, streamBuffer, _storageStruct.beacon.rssi, BEACON_RSSI);
	}
#endif
	case CONFIG_WIFI_SETTINGS: {
		LOGd("Read wifi settings. Does reset it.");
		//! copy string, because we clear it on read
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
	case CONFIG_TX_POWER: {
		LOGd("Read tx power");
		return getInt8(type, streamBuffer, _storageStruct.txPower, TX_POWER);
	}
	case CONFIG_ADV_INTERVAL: {
		LOGd("Read advertisement interval");
		return getUint16(type, streamBuffer, _storageStruct.advInterval, ADVERTISEMENT_INTERVAL);
	}
	case CONFIG_PASSKEY: {
		LOGd("Reading passkey");
		loadPersistentStorage();
		uint8_t plen = BLE_GAP_PASSKEY_LEN;
		uint8_t payload[BLE_GAP_PASSKEY_LEN];
		Storage::getArray<uint8_t>(_storageStruct.passkey, payload, (uint8_t*)STATIC_PASSKEY, plen);
		//! should we return the passkey? probably not ...
		//			streamBuffer->setPayload((uint8_t*)payload, plen);
		//			streamBuffer->setType(type);

		LOGd("passkey set in payload: %s", std::string((char*)payload, BLE_GAP_PASSKEY_LEN).c_str());
		//			return true;
		return false;
	}
	case CONFIG_MIN_ENV_TEMP: {
		LOGd("Read min env temp");
		return getInt8(type, streamBuffer, _storageStruct.minEnvTemp, MIN_ENV_TEMP);
	}
	case CONFIG_MAX_ENV_TEMP: {
		LOGd("Read max env temp");
		return getInt8(type, streamBuffer, _storageStruct.maxEnvTemp, MAX_ENV_TEMP);
	}
	case CONFIG_SCAN_DURATION: {
		LOGd("Read scan duration");
		return getUint16(type, streamBuffer, _storageStruct.scanDuration, SCAN_DURATION);
	}
	case CONFIG_SCAN_SEND_DELAY: {
		LOGd("Read scan send delay");
		return getUint16(type, streamBuffer, _storageStruct.scanSendDelay, SCAN_SEND_DELAY);
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		LOGd("Read scan break duration");
		return getUint16(type, streamBuffer, _storageStruct.scanBreakDuration, SCAN_BREAK_DURATION);
	}
	case CONFIG_BOOT_DELAY: {
		LOGd("Read boot delay");
		return getUint16(type, streamBuffer, _storageStruct.bootDelay, BOOT_DELAY);
	}
	case CONFIG_MAX_CHIP_TEMP: {
		LOGd("Read max chip temp");
		return getInt8(type, streamBuffer, _storageStruct.maxChipTemp, MAX_CHIP_TEMP);
	}
	case CONFIG_SCAN_FILTER: {
		LOGd("Read scan filter");
		return getUint8(type, streamBuffer, _storageStruct.scanFilter, SCAN_FILTER);
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		LOGd("Read scan filter send fraction");
		return getUint16(type, streamBuffer, _storageStruct.scanFilterSendFraction, SCAN_FILTER_SEND_FRACTION);
	}
	case CONFIG_RESET_COUNTER: {
		LOGd("Read reset counter");
		return getStateVar(type, streamBuffer, _stateVars.resetCounter, -1);
	}
	default: {
		LOGw("There is no such configuration type (%u).", type);
	}
	}
	return false;
}

bool Settings::getStateVar(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint32_t defaultValue) {
	readStateVars();
	uint8_t plen = 1;
	uint32_t payload[plen];
	Storage::getUint32(value, payload[0], defaultValue);
	streamBuffer->setPayload((uint8_t*)payload, plen*sizeof(uint32_t));
	streamBuffer->setType(type);
	LOGd("Value set in payload: %u with len %u", payload[0], streamBuffer->length());
	return true;
}

bool Settings::getStateVar(uint8_t type, uint32_t& target) {
	switch(type) {
	case CONFIG_RESET_COUNTER: {
		Storage::getUint32(_stateVars.resetCounter, target, -1);
		break;
	}
	}
//	LOGd("read state var: %u", target);
	return true;
}

bool Settings::setStateVar(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target) {
	if (length != 4) {
		LOGw("Expected uint32");
		return false;
	}
	uint16_t val = ((uint32_t*)payload)[0];
	LOGi("Set %u to %u", type, val);
	Storage::setUint32(val, target);
	if (persistent) {
		LOGi("target: %p", &target);

		pstorage_size_t offset = Storage::getOffset(&_stateVars, &target);
		Storage::getInstance().writeItem(_stateVarsHandle, offset, &target, 4);
	}
	EventDispatcher::getInstance().dispatch(type, &target, 4);
	return true;
}

bool Settings::setStateVar(uint8_t type, uint32_t& target) {
	switch(type) {
	case CONFIG_RESET_COUNTER: {
		Storage::setUint32(target, _stateVars.resetCounter);

		pstorage_size_t offset = Storage::getOffset(&_stateVars, &_stateVars.resetCounter);
		Storage::getInstance().writeItem(_stateVarsHandle, offset, &_stateVars.resetCounter, 4);

		break;
	}
	}
//	LOGd("set state var: %u", target);
	return true;
}

bool Settings::getUint16(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint16_t defaultValue) {
	loadPersistentStorage();
	uint8_t plen = 1;
	uint16_t payload[plen];
	Storage::getUint16(value, payload[0], defaultValue);
	streamBuffer->setPayload((uint8_t*)payload, plen*sizeof(uint16_t));
	streamBuffer->setType(type);
	LOGd("Value set in payload: %u with len %u", payload[0], streamBuffer->length());
	return true;
}

bool Settings::getInt8(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, int32_t value, int8_t defaultValue) {
	loadPersistentStorage();
	uint8_t plen = 1;
	int8_t payload[plen];
	Storage::getInt8(value, payload[0], defaultValue);
	streamBuffer->setPayload((uint8_t*)payload, plen);
	streamBuffer->setType(type);
	LOGd("Value set in payload: %i with len %u", payload[0], streamBuffer->length());
	return true;
}

bool Settings::getUint8(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint8_t defaultValue) {
	loadPersistentStorage();
	uint8_t plen = 1;
	uint8_t payload[plen];
	Storage::getUint8(value, payload[0], defaultValue);
	streamBuffer->setPayload((uint8_t*)payload, plen);
	streamBuffer->setType(type);
	LOGd("Value set in payload: %i with len %u", payload[0], streamBuffer->length());
	return true;
}

bool Settings::setUint16(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target) {
	if (length != 2) {
		LOGw("Expected uint16");
		return false;
	}
	uint16_t val = ((uint16_t*)payload)[0];
	LOGi("Set %u to %u", type, val);
	Storage::setUint16(val, target);
	if (persistent) {
		savePersistentStorage();
	}
	EventDispatcher::getInstance().dispatch(type, &target, 4);
	return true;
}

bool Settings::setInt8(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, int32_t& target) {
	if (length != 1) {
		LOGw("Expected int8");
		return false;
	}
	int8_t val = payload[0];
	LOGi("Set %u to %i", type, val);
	Storage::setInt8(val, target);
	if (persistent) {
		savePersistentStorage();
	}
	EventDispatcher::getInstance().dispatch(type, &target, 4);
	return true;
}

bool Settings::setUint8(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target) {
	if (length != 1) {
		LOGw("Expected int8");
		return false;
	}
	uint8_t val = payload[0];
	LOGi("Set %u to %i", type, val);
	Storage::setUint8(val, target);
	if (persistent) {
		savePersistentStorage();
	}
	EventDispatcher::getInstance().dispatch(type, &val, 1);
	return true;
}

ps_configuration_t& Settings::getConfig() {
	return _storageStruct;
}

ps_state_vars_t& Settings::getStateVars() {
	return _stateVars;
}

/** Get a handle to the persistent storage struct and load it from FLASH.
 *
 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
 * writing less than a minimal block strains the memory just as much as flashing the entire block.
 * Hence, there is an entire struct that can be filled and flashed at once.
 */
void Settings::loadPersistentStorage() {
	Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

/** Save to FLASH.
 */
void Settings::savePersistentStorage() {
	Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void Settings::readStateVars() {
	Storage::getInstance().readStorage(_stateVarsHandle, &_stateVars, sizeof(_stateVars));
}

void Settings::writeStateVars() {
	ps_state_vars_t _oldVars;

	Storage::getInstance().readStorage(_stateVarsHandle, &_oldVars, sizeof(_oldVars));

	// todo: compare flash with working copy and only store changed variables
}

//	void ConfigHelper::enable(ps_storage_id id, uint16_t size) {
//		Storage::getInstance().getHandle(id, _storageHandles[id]);
//		loadPersistentStorage(_storageHandles[id], );
//	}

/** Retrieve the Bluetooth name from the object representing the BLE stack.
 *
 * @return name of the device
 */
std::string & Settings::getBLEName() {
	loadPersistentStorage();
	std::string& _name = BLEpp::Nrf51822BluetoothStack::getInstance().getDeviceName();
	Storage::getString(_storageStruct.device_name, _name, _name);
	return _name;
}

/** Write the Bluetooth name to the object representing the BLE stack.
 *
 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It
 * has to be written to FLASH in that case.
 */
void Settings::setBLEName(const std::string &name, bool persistent) {
	if (name.length() > 31) {
		log(ERROR, MSG_NAME_TOO_LONG);
		return;
	}
	BLEpp::Nrf51822BluetoothStack::getInstance().updateDeviceName(name);
	Storage::setString(name, _storageStruct.device_name);
	if (persistent) {
		savePersistentStorage();
	}
}
