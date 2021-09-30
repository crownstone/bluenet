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
	_advData.adv_data.p_data = nullptr;
	_advData.adv_data.len = 0;
	_advData.scan_rsp_data.p_data = nullptr;
	_advData.scan_rsp_data.len = 0;

	configureAdvertisementParameters();
}

void Advertiser::init() {
	LOGd("init");
	if (_isInitialized) {
		LOGw("Already initialized");
		return;
	}
	// Can't update device name when radio is not initialized (error 0x3001 == BLE_ERROR_NOT_ENABLED).
	if (!_stack->checkCondition(Stack::C_RADIO_INITIALIZED, true)) {
		LOGw("Radio not initialized");
		return;
	}

	_isInitialized = true;
	// Now that we're initialized, we can update the device name.
	setDeviceName(_deviceName);

	EventDispatcher::getInstance().addListener(this);
}


void Advertiser::setAdvertisingInterval(uint16_t advertisingInterval) {
	LOGd("Set advertising interval %u", advertisingInterval);
	if (advertisingInterval < 0x0020 || advertisingInterval > 0x4000) {
		LOGw("Invalid advertising interval %u", advertisingInterval);
		return;
	}
	_advertisingInterval = advertisingInterval;
	configureAdvertisementParameters();

	if (!_isInitialized) {
		return;
	}

	updateAdvertisementParams();
}

void Advertiser::setDeviceName(const std::string& deviceName) {
	LOGd("Set device name to %s", deviceName.c_str());
	_deviceName = deviceName;

	if (!_isInitialized) {
		return;
	}

	std::string name = _deviceName.empty() ? "none" : _deviceName;

	// Although this has nothing to do with advertising, this is required when changing the name on the soft device.
	ble_gap_conn_sec_mode_t nameCharacteristicSecurityMode;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&nameCharacteristicSecurityMode);

	LOGAdvertiserDebug("sd_ble_gap_device_name_set %s %u", name.c_str(), name.length());
	uint32_t nrfCode = sd_ble_gap_device_name_set(&nameCharacteristicSecurityMode, (uint8_t*) name.c_str(), name.length());
	/**
	 * @retval ::NRF_SUCCESS GAP device name and permissions set successfully.
	 * @retval ::NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
	 * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
	 * @retval ::NRF_ERROR_DATA_SIZE Invalid data size(s) supplied.
	 * @retval ::NRF_ERROR_FORBIDDEN Device name is not writable.
	 */
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to set device name: nrfCode=%u", nrfCode);
		return;
	}
}

void Advertiser::setTxPower(int8_t powerDBm) {
	if (_txPower == powerDBm) {
		return;
	}
	LOGAdvertiserDebug("setTxPower %i", powerDBm);

	switch (powerDBm) {
		case -40: case -20: case -16: case -12: case -8: case -4: case 0: case 4:
			// accepted values
			break;
		default:
			LOGw("Invalid TX power: %i", powerDBm);
			// other values are not accepted
			return;
	}

	_txPower = powerDBm;

	if (_isInitialized && _advHandle != BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		updateTxPower();
	}
}

void Advertiser::updateTxPower() {
	if (!_isInitialized) {
		LOGw("Not initialized");
		return;
	}

	if (_advHandle == BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		LOGw("Invalid handle");
		return;
	}

	LOGi("Update TX power to %i for handle %u", _txPower, _advHandle);
	uint32_t nrfCode = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, _advHandle, _txPower);
	/**
	 * @retval ::NRF_SUCCESS Successfully changed the transmit power.
	 * @retval ::NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
	 * @retval ::BLE_ERROR_INVALID_ADV_HANDLE Advertising handle not found.
	 * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied.
	 */
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to set TX power: nrfCode=%u", nrfCode);
		return;
	}
}

void Advertiser::setLowTxPower() {
	TYPIFY(CONFIG_LOW_TX_POWER) lowTxPower;
	State::getInstance().get(CS_TYPE::CONFIG_LOW_TX_POWER, &lowTxPower, sizeof(lowTxPower));
	setTxPower(lowTxPower);
}

void Advertiser::setNormalTxPower() {
	TYPIFY(CONFIG_TX_POWER) txPower;
	State::getInstance().get(CS_TYPE::CONFIG_TX_POWER, &txPower, sizeof(txPower));
	setTxPower(txPower);
}




void Advertiser::setAdvertisementData(ServiceData& serviceData, bool asScanResponse) {
	LOGd("Set service data");

	_serviceData = &serviceData;

	ble_advdata_t* advData = &_configAdvertisementData;
	if (asScanResponse) {
		advData = &_configScanResponse;
	}

	memset(advData, 0, sizeof(*advData));
	uint8_t advertisementDataSize = 0;

	// Add the flags.
	if (!asScanResponse) {
		advData->flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		advData->include_appearance = false;
		// 1 byte AD LEN, 1 byte AD TYPE, 1 byte FLAGS.
		advertisementDataSize += 3;
	}

	// Add the service data.
	memset(&_crownstoneServiceData, 0, sizeof(_crownstoneServiceData));
	_crownstoneServiceData.service_uuid = CROWNSTONE_PLUG_SERVICE_DATA_UUID;
	_crownstoneServiceData.data.p_data = _serviceData->getArray();
	_crownstoneServiceData.data.size = _serviceData->getArraySize();

	advData->p_service_data_array = &_crownstoneServiceData;
	advData->service_data_count = 1;

	LOGd("Add service data UUID %04X", _crownstoneServiceData.service_uuid);
	// 1 byte AD LEN, 1 byte AD TYPE, 2 bytes UUID, N bytes payload.
	advertisementDataSize += 2 + sizeof(_crownstoneServiceData.service_uuid) + _crownstoneServiceData.data.size;

	// Add the name
	advData->name_type = BLE_ADVDATA_SHORT_NAME;
	// 1 byte AD LEN, 1 byte AD TYPE.
	advertisementDataSize += 2;
	uint8_t bytesLeft = BLE_GAP_ADV_SET_DATA_SIZE_MAX - advertisementDataSize;
	if (advertisementDataSize > BLE_GAP_ADV_SET_DATA_SIZE_MAX) {
		bytesLeft = 0;
	}

	LOGd("Max name length = %u", bytesLeft);
	uint8_t nameLength = _deviceName.length();
	nameLength = std::min(bytesLeft, nameLength);

	if (nameLength == 0) {
		LOGw("Advertisement too large for a name.");
		advData->name_type = BLE_ADVDATA_NO_NAME;
	}
	LOGd("Set name to length %u", nameLength);
	advData->short_name_len = nameLength;

	if (!allocateAdvertisementDataBuffers(asScanResponse)) {
		return;
	}

	if (asScanResponse) {
		_includeScanResponseData = true;
	}
	else {
		_includeAdvertisementData = true;
	}
}

void Advertiser::setAdvertisementData(IBeacon& beacon, bool asScanResponse) {
	LOGd("Set iBeacon data");

	ble_advdata_t* advData = &_configAdvertisementData;
	if (asScanResponse) {
		advData = &_configScanResponse;
	}

	memset(advData, 0, sizeof(*advData));
	uint8_t advertisementDataSize = 0;

	// Add the flags.
	if (!asScanResponse) {
		advData->flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		advData->include_appearance = false;
		// 1 byte AD LEN, 1 byte AD TYPE, 1 byte FLAGS.
		advertisementDataSize += 3;
	}

	// Add the iBeacon data
	memset(&_ibeaconManufData, 0, sizeof(_ibeaconManufData));
	_ibeaconManufData.company_identifier = 0x004C; // APPLE
	_ibeaconManufData.data.p_data = beacon.getArray();
	_ibeaconManufData.data.size = beacon.size();
	advData->p_manuf_specific_data = &_ibeaconManufData;
	// 1 byte AD LEN, 1 byte AD TYPE, 2 bytes UUID, N bytes payload.
	advertisementDataSize += 2 + sizeof(_crownstoneServiceData.service_uuid) + _crownstoneServiceData.data.size;

	// Add no name, there is no space for that.
	advData->name_type = BLE_ADVDATA_NO_NAME;

	if (!allocateAdvertisementDataBuffers(asScanResponse)) {
		return;
	}

	if (asScanResponse) {
		_includeScanResponseData = true;
	}
	else {
		_includeAdvertisementData = true;
	}
}


bool Advertiser::allocateAdvertisementDataBuffers(bool scanResponse) {
	LOGAdvertiserDebug("allocateAdvertisementDataBuffers");

	uint8_t** bufferPointers = _advertisementDataBuffers;
	if (scanResponse) {
		bufferPointers = _scanResponseBuffers;
	}

	LOGAdvertiserDebug("&(_advertisementDataBuffers[0])=%p &(bufferPointers[0])=%p", &(_advertisementDataBuffers[0]), &(bufferPointers[0]));

	for (uint8_t i = 0; i < _advertisementDataBufferCount; ++i) {
		if (bufferPointers[i] == nullptr) {
			bufferPointers[i] = (uint8_t*)calloc(sizeof(uint8_t), _advertisementDataBufferSize);
			if (bufferPointers[i] == nullptr) {
				// Out of memory, this shouldn't happen.
				// Instead of complicated deallocating, we just return false.
				LOGw("Could not allocate advertisement data buffer");
				return false;
			}
		}
	}
	return true;
}


void Advertiser::configureAdvertisement(__attribute__((unused))IBeacon& beacon, bool asScanResponse) {
//	configureIBeaconAdvData(beacon);
	setAdvertisementData(beacon, asScanResponse);
	updateAdvertisementData();
}

void Advertiser::configureAdvertisement(ServiceData& serviceData, bool asScanResponse) {
	setAdvertisementData(serviceData, asScanResponse);
	updateAdvertisementData();
}

/**
 * It is only possible to include TX power if the advertisement is an "extended" type.
 */
void Advertiser::configureAdvertisementParameters() {
	LOGAdvertiserDebug("set _advParams");
	_advParams.primary_phy                 = BLE_GAP_PHY_1MBPS;
	_advParams.properties.type             = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
	_advParams.properties.anonymous        = 0;
	_advParams.properties.include_tx_power = 0;
	_advParams.p_peer_addr                 = nullptr;
	_advParams.filter_policy               = BLE_GAP_ADV_FP_ANY;
	_advParams.interval                    = _advertisingInterval;
	_advParams.duration                    = 0;
}

void Advertiser::setConnectable(bool connectable) {
	LOGAdvertiserDebug("setConnectable %i", connectable);
	_wantConnectable = connectable;

	updateAdvertisementParams();
}

void Advertiser::startAdvertising() {
	LOGd("Start advertising");
	_wantAdvertising = true;
	if (!_isInitialized) {
		LOGw("Not initialized");
		return;
	}
	if (_advertising) {
		LOGAdvertiserDebug("Already advertising");
//		return NRF_SUCCESS;
	}

	uint8_t prevAdvHandle = _advHandle;

	LOGAdvertiserDebug("sd_ble_gap_adv_set_configure with params");
	uint32_t nrfCode = sd_ble_gap_adv_set_configure(&_advHandle, &_advData, &_advParams);
	/*
	 * @retval ::NRF_SUCCESS                               Advertising set successfully configured.
	 * @retval ::NRF_ERROR_INVALID_PARAM                   Invalid parameter(s) supplied:
	 *                                                      - Invalid advertising data configuration specified. See @ref ble_gap_adv_data_t.
	 *                                                      - Invalid configuration of p_adv_params. See @ref ble_gap_adv_params_t.
	 *                                                      - Use of whitelist requested but whitelist has not been set,
	 *                                                        see @ref sd_ble_gap_whitelist_set.
	 * @retval ::BLE_ERROR_GAP_INVALID_BLE_ADDR            ble_gap_adv_params_t::p_peer_addr is invalid.
	 * @retval ::NRF_ERROR_INVALID_STATE                   Invalid state to perform operation. Either:
	 *                                                     - It is invalid to provide non-NULL advertising set parameters while advertising.
	 *                                                     - It is invalid to provide the same data buffers while advertising. To update
	 *                                                       advertising data, provide new advertising buffers.
	 * @retval ::BLE_ERROR_GAP_DISCOVERABLE_WITH_WHITELIST Discoverable mode and whitelist incompatible.
	 * @retval ::BLE_ERROR_INVALID_ADV_HANDLE              The provided advertising handle was not found. Use @ref BLE_GAP_ADV_SET_HANDLE_NOT_SET to
	 *                                                     configure a new advertising handle.
	 * @retval ::NRF_ERROR_INVALID_ADDR                    Invalid pointer supplied.
	 * @retval ::NRF_ERROR_INVALID_FLAGS                   Invalid combination of advertising flags supplied.
	 * @retval ::NRF_ERROR_INVALID_DATA                    Invalid data type(s) supplied. Check the advertising data format specification
	 *                                                     given in Bluetooth Specification Version 5.0, Volume 3, Part C, Chapter 11.
	 * @retval ::NRF_ERROR_INVALID_LENGTH                  Invalid data length(s) supplied.
	 * @retval ::NRF_ERROR_NOT_SUPPORTED                   Unsupported data length or advertising parameter configuration.
	 * @retval ::NRF_ERROR_NO_MEM                          Not enough memory to configure a new advertising handle. Update an
	 *                                                     existing advertising handle instead.
	 * @retval ::BLE_ERROR_GAP_UUID_LIST_MISMATCH Invalid UUID list supplied.
	 */
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Configure advertisement failed: nrfCode=%u", nrfCode);
		printAdvertisement();
		return;
	}

	if (prevAdvHandle == BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		// Now that there's a handle, we can update the TX power.
		updateTxPower();
	}

	_advParamsChanged = false;

	// This often fails because this function is called, while the SD is already connected.
	// The on connect event is scheduled, but not processed by us yet.
	LOGAdvertiserDebug("sd_ble_gap_adv_start(_adv_handle=%u)", _advHandle);
	nrfCode = sd_ble_gap_adv_start(_advHandle, APP_BLE_CONN_CFG_TAG); // Only one advertiser may be active at any time.
	/**
	 * @retval ::NRF_SUCCESS                  The BLE stack has started advertising.
	 * @retval ::NRF_ERROR_INVALID_STATE      adv_handle is not configured or already advertising.
	 * @retval ::NRF_ERROR_CONN_COUNT         The limit of available connections has been reached; connectable advertiser cannot be started.
	 * @retval ::BLE_ERROR_INVALID_ADV_HANDLE Advertising handle not found. Configure a new adveriting handle with @ref sd_ble_gap_adv_set_configure.
	 * @retval ::NRF_ERROR_NOT_FOUND          conn_cfg_tag not found.
	 * @retval ::NRF_ERROR_INVALID_PARAM      Invalid parameter(s) supplied:
	 *                                        - Invalid configuration of p_adv_params. See @ref ble_gap_adv_params_t.
	 *                                        - Use of whitelist requested but whitelist has not been set, see @ref sd_ble_gap_whitelist_set.
	 * @retval ::NRF_ERROR_RESOURCES          Either:
	 *                                        - adv_handle is configured with connectable advertising, but the event_length parameter
	 *                                          associated with conn_cfg_tag is too small to be able to establish a connection on
	 *                                          the selected advertising phys. Use @ref sd_ble_cfg_set to increase the event length.
	 *                                        - Not enough BLE role slots available.
	 *                                          Stop one or more currently active roles (Central, Peripheral, Broadcaster or Observer) and try again.
	 *                                        - p_adv_params is configured with connectable advertising, but the event_length parameter
	 *                                          associated with conn_cfg_tag is too small to be able to establish a connection on
	 *                                          the selected advertising phys. Use @ref sd_ble_cfg_set to increase the event length.
	 * @retval ::NRF_ERROR_NOT_SUPPORTED Unsupported PHYs supplied to the call.
	 */
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Start advertising failed: nrfCode=%u", nrfCode);
		return;
	}
	_advertising = true;
}

void Advertiser::stopAdvertising() {
	LOGd("Stop advertising");
	_wantAdvertising = false;
	if (!_isInitialized) {
		LOGw("Not initialized");
		return;
	}
	if (!_advertising) {
		LOGAdvertiserDebug("Not advertising");
//		return;
	}

	// This often fails because this function is called, while the SD is already connected, thus advertising was stopped.
	// The on connect event is scheduled, but not processed by us yet.

	// 30-10-2019 However: there also seems to be the case where stopAdvertising() is called right after startAdvertising()
	// In this case, we also get the invalid state error, and advertising isn't actually stopped.
	// So we can't really trust _advertising to be correct.
	LOGAdvertiserDebug("sd_ble_gap_adv_stop(_adv_handle=%u)", _advHandle);
	uint32_t nrfCode = sd_ble_gap_adv_stop(_advHandle);
	/**
	 * @retval ::NRF_SUCCESS The BLE stack has stopped advertising.
	 * @retval ::BLE_ERROR_INVALID_ADV_HANDLE Invalid advertising handle.
	 * @retval ::NRF_ERROR_INVALID_STATE The advertising handle is not advertising.
	 */
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Stop advertising failed: nrfCode=%u", nrfCode);
		return;
	}
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
	if (!_isInitialized) {
		LOGw("Not initialized");
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

	LOGAdvertiserDebug("updateAdvertisementParams connectable=%u change=%u", connectable, _advParamsChanged);
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
	if (!_isInitialized) {
		LOGw("Not initialized");
		return;
	}
	uint32_t nrfCode;

	uint8_t bufIndex = (_advBufferInUse + 1) % _advertisementDataBufferCount;
	LOGAdvertiserVerbose("updateAdvertisementData buf=%u pointer=%p", bufIndex, _advertisementDataBuffers[bufIndex]);
	_advBufferInUse = bufIndex;
	if (_includeAdvertisementData) {
		LOGAdvertiserVerbose("include adv data");
		_advData.adv_data.len = _advertisementDataBufferSize;
		_advData.adv_data.p_data = _advertisementDataBuffers[bufIndex];
		nrfCode = ble_advdata_encode(&_configAdvertisementData, _advData.adv_data.p_data, &_advData.adv_data.len);
		/**
		 * @retval NRF_SUCCESS             If the operation was successful.
		 * @retval NRF_ERROR_INVALID_PARAM If the operation failed because a wrong parameter was provided in
		 *                                 \p p_advdata.
		 * @retval NRF_ERROR_DATA_SIZE     If the operation failed because not all the requested data could
		 *                                 fit into the provided buffer or some encoded AD structure is too
		 *                                 long and its length cannot be encoded with one octet.
		 */
		if (nrfCode != NRF_SUCCESS) {
			LOGw("Encode advertisement data failed: nrfCode=%u", nrfCode);
			return;
		}
	}
	if (_includeScanResponseData) {
		LOGAdvertiserVerbose("include scan response");
		_advData.scan_rsp_data.len = _advertisementDataBufferSize;
		_advData.scan_rsp_data.p_data = _scanResponseBuffers[bufIndex];
		nrfCode = ble_advdata_encode(&_configScanResponse, _advData.scan_rsp_data.p_data, &_advData.scan_rsp_data.len);
		/**
		 * @retval NRF_SUCCESS             If the operation was successful.
		 * @retval NRF_ERROR_INVALID_PARAM If the operation failed because a wrong parameter was provided in
		 *                                 \p p_advdata.
		 * @retval NRF_ERROR_DATA_SIZE     If the operation failed because not all the requested data could
		 *                                 fit into the provided buffer or some encoded AD structure is too
		 *                                 long and its length cannot be encoded with one octet.
		 */
		if (nrfCode != NRF_SUCCESS) {
			LOGw("Encode scan response failed: nrfCode=%u", nrfCode);
			return;
		}
	}

	if (_advHandle != BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
		LOGAdvertiserVerbose("sd_ble_gap_adv_set_configure without params");
		nrfCode = sd_ble_gap_adv_set_configure(&_advHandle, &_advData, nullptr);
		/**
		 * @retval ::NRF_SUCCESS                               Advertising set successfully configured.
		 * @retval ::NRF_ERROR_INVALID_PARAM                   Invalid parameter(s) supplied:
		 *                                                      - Invalid advertising data configuration specified. See @ref ble_gap_adv_data_t.
		 *                                                      - Invalid configuration of p_adv_params. See @ref ble_gap_adv_params_t.
		 *                                                      - Use of whitelist requested but whitelist has not been set,
		 *                                                        see @ref sd_ble_gap_whitelist_set.
		 * @retval ::BLE_ERROR_GAP_INVALID_BLE_ADDR            ble_gap_adv_params_t::p_peer_addr is invalid.
		 * @retval ::NRF_ERROR_INVALID_STATE                   Invalid state to perform operation. Either:
		 *                                                     - It is invalid to provide non-NULL advertising set parameters while advertising.
		 *                                                     - It is invalid to provide the same data buffers while advertising. To update
		 *                                                       advertising data, provide new advertising buffers.
		 * @retval ::BLE_ERROR_GAP_DISCOVERABLE_WITH_WHITELIST Discoverable mode and whitelist incompatible.
		 * @retval ::BLE_ERROR_INVALID_ADV_HANDLE              The provided advertising handle was not found. Use @ref BLE_GAP_ADV_SET_HANDLE_NOT_SET to
		 *                                                     configure a new advertising handle.
		 * @retval ::NRF_ERROR_INVALID_ADDR                    Invalid pointer supplied.
		 * @retval ::NRF_ERROR_INVALID_FLAGS                   Invalid combination of advertising flags supplied.
		 * @retval ::NRF_ERROR_INVALID_DATA                    Invalid data type(s) supplied. Check the advertising data format specification
		 *                                                     given in Bluetooth Specification Version 5.0, Volume 3, Part C, Chapter 11.
		 * @retval ::NRF_ERROR_INVALID_LENGTH                  Invalid data length(s) supplied.
		 * @retval ::NRF_ERROR_NOT_SUPPORTED                   Unsupported data length or advertising parameter configuration.
		 * @retval ::NRF_ERROR_NO_MEM                          Not enough memory to configure a new advertising handle. Update an
		 *                                                     existing advertising handle instead.
		 * @retval ::BLE_ERROR_GAP_UUID_LIST_MISMATCH Invalid UUID list supplied.
		 */
		if (nrfCode != NRF_SUCCESS) {
			LOGw("Set advertisement data failed: nrfCode=%u", nrfCode);
			return;
		}
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
			setDeviceName(std::string((char*)event.data, event.size));
			break;
		}
		case CS_TYPE::CONFIG_TX_POWER: {
			setTxPower(*(TYPIFY(CONFIG_TX_POWER)*)event.data);
			break;
		}
		case CS_TYPE::CONFIG_ADV_INTERVAL: {
			setAdvertisingInterval(*(TYPIFY(CONFIG_ADV_INTERVAL)*)event.data);
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
