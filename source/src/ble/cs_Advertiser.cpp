/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Advertiser.h>
#include <cfg/cs_DeviceTypes.h>
#include <cfg/cs_UuidConfig.h>
#include <events/cs_EventDispatcher.h>
#include <uart/cs_UartHandler.h>

#define LOGAdvertiserDebug LOGvv
#define LOGAdvertiserVerbose LOGvv

Advertiser::Advertiser() {
	_stack = &(Stack::getInstance());
	_advData.adv_data.p_data = NULL;
	_advData.adv_data.len = 0;
	_advData.scan_rsp_data.p_data = NULL;
	_advData.scan_rsp_data.len = 0;
}

void Advertiser::init() {
	if (!isInitialized(false)) {
		LOGw("Already initialized");
		return;
	}
	// Can't update device name or appearance when radio is not initialized (error 0x3001 == BLE_ERROR_NOT_ENABLED).
	if (!_stack->checkCondition(Stack::C_RADIO_INITIALIZED, true)) {
		LOGw("Radio not (yet) initialized");
		return;
	}
	_isInitialized = true;
	EventDispatcher::getInstance().addListener(this);
	updateDeviceName(_deviceName);
	updateAppearance(_appearance);
	configureAdvertisementParameters();
}

bool Advertiser::isInitialized(bool expected) {
	if (expected != _isInitialized) {
		LOGw("Advertiser is %sinitialized", expected ? "not " : "");
	}
	return expected == _isInitialized;
}

void Advertiser::setAdvertisingTimeoutSeconds(uint16_t advertisingTimeoutSeconds) {
	isInitialized(false);
	_advertisingTimeout = advertisingTimeoutSeconds;
}

void Advertiser::setAppearance(uint16_t appearance) {
	isInitialized(false);
	_appearance = appearance;
}

void Advertiser::setDeviceName(const std::string& deviceName) {
	isInitialized(false);
	_deviceName = deviceName;
}

std::string & Advertiser::getDeviceName() {
	return _deviceName;
}

void Advertiser::setAdvertisingInterval(uint16_t advertisingInterval) {
	isInitialized(false);
	if (advertisingInterval < 0x0020 || advertisingInterval > 0x4000) {
		LOGw("Invalid advertising interval");
		return;
	}
	_advertisingInterval = advertisingInterval;
}

void Advertiser::updateAdvertisingInterval(uint16_t advertisingInterval) {
	LOGd("Update advertising interval");
	if (!isInitialized()) return;
	setAdvertisingInterval(advertisingInterval);
	configureAdvertisementParameters();
	updateAdvertisementParams();
}

void Advertiser::updateDeviceName(const std::string& deviceName) {
	LOGd("Set device name to %s", _deviceName.c_str());
	_deviceName = deviceName;
	if (!isInitialized()) return;

	std::string name = _deviceName.empty() ? "none" : deviceName;

	// Although this has nothing to do with advertising, this is required when changing the name on the soft device.
	ble_gap_conn_sec_mode_t nameCharacteristicSecurityMode;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&nameCharacteristicSecurityMode);

	uint32_t ret_code;
	LOGAdvertiserDebug("sd_ble_gap_device_name_set %s %u", name.c_str(), name.length());
	ret_code = sd_ble_gap_device_name_set(&nameCharacteristicSecurityMode, (uint8_t*) name.c_str(), name.length());
	APP_ERROR_CHECK(ret_code);
}

void Advertiser::updateAppearance(uint16_t appearance) {
	if (!isInitialized()) return;
	_appearance = appearance;
	LOGAdvertiserDebug("sd_ble_gap_appearance_set");
	BLE_CALL(sd_ble_gap_appearance_set, (_appearance));
}

void Advertiser::updateTxPower(int8_t powerDBm) {
	LOGAdvertiserDebug("updateTxPower %i", powerDBm);
	switch (powerDBm) {
		case -40: case -20: case -16: case -12: case -8: case -4: case 0: case 4:
			// accepted values
			break;
		default:
			// other values are not accepted
			return;
	}
	if (_txPower != powerDBm) {
		_txPower = powerDBm;
		updateTxPower();
	}
}

void Advertiser::changeToLowTxPower() {
	TYPIFY(CONFIG_LOW_TX_POWER) lowTxPower;
	State::getInstance().get(CS_TYPE::CONFIG_LOW_TX_POWER, &lowTxPower, sizeof(lowTxPower));
	updateTxPower(lowTxPower);
}

void Advertiser::changeToNormalTxPower() {
	TYPIFY(CONFIG_TX_POWER) txPower;
	State::getInstance().get(CS_TYPE::CONFIG_TX_POWER, &txPower, sizeof(txPower));
	updateTxPower(txPower);
}

int8_t Advertiser::getTxPower() {
	return _txPower;
}

/**
 * It seems that if we have a connectable advertisement and a subsequent connection, we cannot update the TX
 * power while already being connected. At least it returns a BLE_ERROR_INVALID_ADV_HANDLE.
 */
void Advertiser::updateTxPower() {
	if (!isInitialized()) {
		return;
	}
	if (_advHandle == BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		LOGd("Invalid handle");
		return;
	}
	uint32_t ret_code;
	LOGd("Update TX power to %i for handle %u", _txPower, _advHandle);
	LOGAdvertiserDebug("sd_ble_gap_tx_power_set");
	ret_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, _advHandle, _txPower);
	APP_ERROR_CHECK(ret_code);
}




void Advertiser::configureIBeaconAdvData(IBeacon* beacon) {
	LOGd("Configure iBeacon adv data");

	memset(&_ibeaconManufData, 0, sizeof(_ibeaconManufData));
	_ibeaconManufData.company_identifier = 0x004C;
	_ibeaconManufData.data.p_data = beacon->getArray();
	_ibeaconManufData.data.size = beacon->size();

	memset(&_configAdvertisementData, 0, sizeof(_configAdvertisementData));

	_configAdvertisementData.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	_configAdvertisementData.p_manuf_specific_data = &_ibeaconManufData;
	_configAdvertisementData.name_type = BLE_ADVDATA_NO_NAME;
	_configAdvertisementData.include_appearance = false;

	_includeAdvertisementData = true;
}

void Advertiser::configureServiceData(uint8_t deviceType, bool asScanResponse) {
	LOGd("Configure service data");

	uint8_t serviceDataLength = 0;
	ble_advdata_t* advData = &_configAdvertisementData;
	if (asScanResponse) {
		advData = &_configScanResponse;
	}

	memset(advData, 0, sizeof(*advData));

	advData->name_type = BLE_ADVDATA_SHORT_NAME;

	if (!asScanResponse) {
		advData->flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		advData->include_appearance = false;
	}

	if (_serviceData && deviceType != DEVICE_UNDEF) {
		memset(&_crownstoneServiceData, 0, sizeof(_crownstoneServiceData));

		_crownstoneServiceData.service_uuid = CROWNSTONE_PLUG_SERVICE_DATA_UUID;

		_crownstoneServiceData.data.p_data = _serviceData->getArray();
		_crownstoneServiceData.data.size = _serviceData->getArraySize();

		advData->p_service_data_array = &_crownstoneServiceData;
		advData->service_data_count = 1;

		LOGd("Add UUID %X", _crownstoneServiceData.service_uuid);
		serviceDataLength += 2 + sizeof(_crownstoneServiceData.service_uuid) + _crownstoneServiceData.data.size; // 2 For service data header.
	}

	uint8_t nameLength = 31 - 3 - serviceDataLength - 2; // 3 For flags field, 2 for name field header.
	LOGd("Max name length = %u", nameLength);
	uint8_t deviceNameLength = getDeviceName().length();
	nameLength = std::min(nameLength, deviceNameLength);
	LOGd("Set BLE name to length %i", nameLength);
	advData->short_name_len = nameLength;

	if (nameLength == 0) {
		LOGe("Scan response payload too large or device name not set");
		return;
	}
	if (asScanResponse) {
		_includeScanResponseData = true;
	}
	else {
		_includeAdvertisementData = true;
	}
}

void Advertiser::configureAdvertisement(__attribute__((unused))IBeacon* beacon, uint8_t deviceType) {
//	configureIBeaconAdvData(beacon);
	configureServiceData(deviceType, false);
//	configureAdvertisementParameters();
	updateAdvertisementData();
}

void Advertiser::configureAdvertisement(uint8_t deviceType) {
	configureServiceData(deviceType, false);
//	configureAdvertisementParameters();
	updateAdvertisementData();
}

/**
 * It is only possible to include TX power if the advertisement is an "extended" type.
 */
void Advertiser::configureAdvertisementParameters() {
	LOGd("set _advParams");
	_advParams.primary_phy                 = BLE_GAP_PHY_1MBPS;
	_advParams.properties.type             = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
	_advParams.properties.anonymous        = 0;
	_advParams.properties.include_tx_power = 0;
	_advParams.p_peer_addr                 = NULL;
	_advParams.filter_policy               = BLE_GAP_ADV_FP_ANY;
	_advParams.interval                    = _advertisingInterval;
	_advParams.duration                    = _advertisingTimeout;
}

void Advertiser::setConnectable(bool connectable) {
	LOGAdvertiserDebug("setConnectable %i", connectable);
	_wantConnectable = connectable;
}

uint32_t Advertiser::startAdvertising() {
	LOGd("Start advertising");
	_wantAdvertising = true;
	if (!isInitialized()) {
		return NRF_ERROR_INVALID_STATE;
	}
	if (_advertising) {
		LOGAdvertiserDebug("Already advertising");
		return NRF_SUCCESS;
	}
	uint32_t err;
	LOGAdvertiserDebug("sd_ble_gap_adv_set_configure with params");
	err = sd_ble_gap_adv_set_configure(&_advHandle, &_advData, &_advParams);
	if (err != NRF_SUCCESS) {
		LOGw("sd_ble_gap_adv_set_configure failed: %u", err);
		printAdvertisement();
		return err;
	}
	_advParamsChanged = false;
	LOGAdvertiserDebug("sd_ble_gap_adv_start(_adv_handle=%u)", _advHandle);
	err = sd_ble_gap_adv_start(_advHandle, APP_BLE_CONN_CFG_TAG); // Only one advertiser may be active at any time.
	if (err == NRF_SUCCESS) {
		_advertising = true;
	}
	else {
		// This often fails because this function is called, while the SD is already connected.
		// The on connect event is scheduled, but not processed by us yet.
		LOGw("startAdvertising failed: %u", err);
//		APP_ERROR_CHECK(err);
	}
	return err;
}

void Advertiser::stopAdvertising() {
	LOGd("Stop advertising");
	_wantAdvertising = false;
	if (!isInitialized()) {
		return;
	}
	if (!_advertising) {
		LOGAdvertiserDebug("Not advertising");
		return;
	}
	LOGAdvertiserDebug("sd_ble_gap_adv_stop(_adv_handle=%u)", _advHandle);
	uint32_t ret_code = sd_ble_gap_adv_stop(_advHandle);

	// This often fails because this function is called, while the SD is already connected, thus advertising was stopped.
	// The on connect event is scheduled, but not processed by us yet.

	// 30-10-2019 However: there also seems to be the case where stopAdvertising() is called right after startAdvertising()
	// In this case, we also get the invalid state error, and advertising isn't actually stopped.
	// So we can't really trust _advertising to be correct.
	APP_ERROR_CHECK_EXCEPT(ret_code, NRF_ERROR_INVALID_STATE);
	_advertising = false;
}

void Advertiser::setConnectableAdvParams() {
	LOGAdvertiserDebug("setConnectableAdvParams");
	// The mac address cannot be changed while advertising, scanning or creating a connection. Maybe Have a
	// function that sets address, which sends an event "set address" that makes the scanner pause (etc).
	_advParams.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
	_isConnectable = true;
}

void Advertiser::setNonConnectableAdvParams() {
	LOGAdvertiserDebug("setNonConnectableAdvParams");
	_advParams.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
	_isConnectable = false;
}

void Advertiser::updateAdvertisementParams() {
	if (!_advertising && !_wantAdvertising) {
		return;
	}
	if (!isInitialized()) {
		return;
	}

	if (_stack->isScanning()) {
		// Skip while scanning or we will get invalid state results when stopping/starting advertisement.
		// TODO: is that so???
		LOGw("Updating while scanning");
//		return;
	}

	bool connectable = _wantConnectable;
	if (connectable && _stack->isConnected()) {
		connectable = false;
	}
	if (connectable && !_isConnectable) {
		_advParamsChanged = true;
	}

	LOGAdvertiserDebug("updateAdvertisementParams connectable=%i change=%i", connectable, _advParamsChanged);
	if (!_advParamsChanged) {
		return;
	}
	if (connectable) {
		setConnectableAdvParams();
	}
	else {
		setNonConnectableAdvParams();
	}
	restartAdvertising();
}

void Advertiser::updateAdvertisementData() {
	if (!isInitialized()) {
		return;
	}
	uint32_t err;

	uint8_t bufIndex = (_advBufferInUse + 1) % 2;
	LOGAdvertiserVerbose("updateAdvertisementData buf=%u", bufIndex);
	_advBufferInUse = bufIndex;
	if (_includeAdvertisementData) {
		LOGAdvertiserVerbose("include adv data");
		_advData.adv_data.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
		if (_advertisementDataBuffers[bufIndex] == NULL) {
			_advertisementDataBuffers[bufIndex] = (uint8_t*)calloc(sizeof(uint8_t), _advData.adv_data.len);
		}
		_advData.adv_data.p_data = _advertisementDataBuffers[bufIndex];
		err = ble_advdata_encode(&_configAdvertisementData, _advData.adv_data.p_data, &_advData.adv_data.len);
		APP_ERROR_CHECK(err);
	}
	if (_includeScanResponseData) {
		LOGAdvertiserVerbose("include scan response");
		_advData.scan_rsp_data.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
		if (_scanResponseBuffers[bufIndex] == NULL) {
			_scanResponseBuffers[bufIndex] = (uint8_t*)calloc(sizeof(uint8_t), _advData.scan_rsp_data.len);
		}
		_advData.scan_rsp_data.p_data = _scanResponseBuffers[bufIndex];
		err = ble_advdata_encode(&_configScanResponse, _advData.scan_rsp_data.p_data, &_advData.scan_rsp_data.len);
		APP_ERROR_CHECK(err);
	}

	if (_advHandle != BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		LOGAdvertiserVerbose("sd_ble_gap_adv_set_configure without params");
		err = sd_ble_gap_adv_set_configure(&_advHandle, &_advData, NULL);
		APP_ERROR_CHECK(err);
	}

	// Sometimes startAdvertising fails, so for now, just retry in here.
	if (!_advertising && _wantAdvertising) {
		startAdvertising();
	}
}

void Advertiser::restartAdvertising() {
	LOGAdvertiserDebug("Restart advertising");
	// 30-10-2019 See stopAdvertising(): _advertising isn't always up to date, so we might want to ignore it and stop
	// regardless.
	if (_advertising) {
		stopAdvertising();
	}
	startAdvertising();
}

void Advertiser::printAdvertisement() {
	LOGd("_adv_handle=%u", _advHandle);

	_log(SERIAL_DEBUG, false, "adv_data len=%u data: ", _advData.adv_data.len);
	_logArray(SERIAL_DEBUG, true, _advData.adv_data.p_data, _advData.adv_data.len);

	_log(SERIAL_DEBUG, false, "scan_rsp_data len=%u data: ", _advData.scan_rsp_data.len);
	_logArray(SERIAL_DEBUG, true, _advData.scan_rsp_data.p_data, _advData.scan_rsp_data.len);

	LOGd("type=%u", _advParams.properties.type);
	LOGd("channel mask: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
			_advParams.channel_mask[0],
			_advParams.channel_mask[1],
			_advParams.channel_mask[2],
			_advParams.channel_mask[3],
			_advParams.channel_mask[4]);
}

void Advertiser::onConnect() {
	// Advertising stops on connect, see: https://devzone.nordicsemi.com/question/80959/check-if-currently-advertising/
	// Do this after _conn_handle is set, as that's used to check if isConnected().
	// Also after callbacks, so that in the callback, parameters can be updated.
	if (_advertising) {
		_advertising = false;
		setNonConnectableAdvParams();
		startAdvertising();
	}
}

void Advertiser::onDisconnect() {
//	if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
////			_advertiser->changeToLowTxPower();
//		_advertiser->changeToNormalTxPower();
//		_advertiser->updateAdvertisementParams();
//	}
	updateAdvertisementParams();
}

void Advertiser::onConnectOutgoing() {
	// When making an outgoing connection, we should not advertise as being connectable anymore.
	if (_advertising) {
		setNonConnectableAdvParams();
		restartAdvertising();
	}
}

void Advertiser::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_BLE_CONNECT: {
			onConnect();
			break;
		}
		case CS_TYPE::EVT_BLE_DISCONNECT: {
			onDisconnect();
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_START: {
			onConnectOutgoing();
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_CONNECT_RESULT: {
			cs_ret_code_t* retCode = CS_TYPE_CAST(EVT_BLE_CENTRAL_CONNECT_RESULT, event.data);
			if (*retCode != ERR_SUCCESS) {
				onDisconnect();
			}
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED: {
			onDisconnect();
			break;
		}

		case CS_TYPE::CONFIG_NAME: {
			updateDeviceName(std::string((char*)event.data, event.size));
			break;
		}
		case CS_TYPE::CONFIG_TX_POWER: {
			updateTxPower(*(TYPIFY(CONFIG_TX_POWER)*)event.data);
			break;
		}
		case CS_TYPE::CONFIG_ADV_INTERVAL: {
			updateAdvertisingInterval(*(TYPIFY(CONFIG_ADV_INTERVAL)*)event.data);
			break;
		}
		case CS_TYPE::CMD_ENABLE_ADVERTISEMENT: {
			TYPIFY(CMD_ENABLE_ADVERTISEMENT) enable = *(TYPIFY(CMD_ENABLE_ADVERTISEMENT)*)event.data;
			if (enable) {
				startAdvertising();
			}
			else {
				stopAdvertising();
			}
			UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ADVERTISEMENT_ENABLED, (uint8_t*)&enable, 1);
			break;
		}
		case CS_TYPE::EVT_ADVERTISEMENT_UPDATED: {
			updateAdvertisementData();
			break;
		}
		default: {}
	}
}
