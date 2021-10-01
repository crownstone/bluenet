/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <ble/cs_Nordic.h>
#include <ble/cs_iBeacon.h>
#include <ble/cs_ServiceData.h>
#include <ble/cs_Stack.h>
#include <cfg/cs_StaticConfig.h>
#include <cfg/cs_Config.h>
#include <events/cs_EventListener.h>

class Advertiser: EventListener {
private:
	Advertiser();
public:
	/**
	 * Get the static singleton instance.
	 */
	static Advertiser& getInstance() {
		static Advertiser instance;
		return instance;
	}
	Advertiser(Advertiser const&) = delete;
	void operator=(Advertiser const&) = delete;

	/**
	 * Initialize the advertiser.
	 *
	 * Can only be done after the radio has been initialized.
	 * It's best to configure the advertiser before initializing.
	 *
	 * Starts listening for events.
	 */
	void init();

	/**
	 * Set the name of this Crownstone.
	 */
	void setDeviceName(const std::string& deviceName);

	/**
	 * Set the advertising interval in 0.625 ms units.
	 */
	void setAdvertisingInterval(uint16_t advertisingInterval);

	/**
	 * Set the radio transmit power.
	 *
	 * @param[in] power      Power in dBm. Accepted values are -40, -20, -16, -12, -8, -4, 0, and 4.
	 */
	void setTxPower(int8_t power);

	/**
	 * Sets TX power to low TX power as stored in State.
	 */
	void setLowTxPower();

	/**
	 * Sets TX power to normal TX power as stored in State.
	 */
	void setNormalTxPower();

	/**
	 * Set whether to advertise being connectable.
	 *
	 * @param[in] connectable          True when connectable.
	 */
	void setConnectable(bool connectable);

	/**
	 * Configure the advertisement to hold iBeacon data.
	 * Sets and updates the advertisement data.
	 *
	 * @param[in] beacon               Object with the iBeacon parameters.
	 * @param[in] asScanResponse       Whether to set the iBeacon data in the scan response.
	 */
	void configureAdvertisement(IBeacon& beacon, bool asScanResponse = false);

	/**
	 * Configure the advertisement to hold Crownstone service data.
	 * Sets and updates the advertisement data.
	 *
	 * @param[in] serviceData          The Crownstone service data class.
	 * @param[in] asScanResponse       Whether to set the service data in the scan response.
	 */
	void configureAdvertisement(ServiceData& serviceData, bool asScanResponse = false);

	/**
	 * Start advertising.
	 *
	 * Make sure you configured an advertisement before starting to advertise.
	 */
	void startAdvertising();

	/**
	 * Stop advertising.
	 */
	void stopAdvertising();

	/**
	 * Internal usage.
	 */
	void handleEvent(event_t & event);


private:
	// Pointer to the BLE stack.
	Stack*                                      _stack = nullptr;


	////////////////////////////// State //////////////////////////////

	// Whether the advertiser has been initialized.
	bool                                        _isInitialized = false;

	// Whether currently advertising. This value might be out of sync.
	bool                                        _advertising = false;

	// Whether we should be advertising.
	bool                                        _wantAdvertising = false;

	// Advertisement handle for softdevice. Set by first call to: sd_ble_gap_adv_set_configure().
	uint8_t                                     _advHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;


	////////////////////////////// Parameters //////////////////////////////

	// Transmit power.
	int8_t                                      _txPower = 0;

	// Advertisement parameter: interval.
	uint16_t                                    _advertisingInterval = g_ADVERTISEMENT_INTERVAL;

	// Advertisement parameters for softdevice.
	ble_gap_adv_params_t                        _advParams;

	// Whether advertising parameters have been changed.
	bool                                        _advParamsChanged = false;

	// Whether the advertisement is connectable.
	bool                                        _isConnectable = false;

	// Whether we want to advertise being connectable.
	bool                                        _wantConnectable = true;


	////////////////////////////// Advertisement data fields //////////////////////////////

	// iBeacon data field to be encoded into advertisement data.
	ble_advdata_manuf_data_t 					_ibeaconManufData;

	// Service data field to be encoded into advertisement data.
	ble_advdata_service_data_t                  _crownstoneServiceData;

	// Pointer to up to data crownstone service data.
	ServiceData*                                _serviceData = nullptr;

	// Advertised name
	std::string                                 _deviceName = "none";


	////////////////////////////// Advertisement data //////////////////////////////

	/**
	 * Advertisement data config, filled by one or more advertisement data fields.
	 * In between step to be encoded into an advertisement data buffer.
	 */
	ble_advdata_t                               _configAdvertisementData;

	/**
	 * Scan response data config, filled by one or more advertisement data fields.
	 * In between step to be encoded into a scan response data buffer.
	 */
	ble_advdata_t                               _configScanResponse;

	/**
	 * Whether the advertisement data config has been filled.
	 * When true, the advertisement data buffers are also allocated.
	 */
	bool                                        _includeAdvertisementData = false;

	/**
	 * Whether the scan response data config has been filled.
	 * When true, the scan response data buffers are also allocated.
	 */
	bool                                        _includeScanResponseData = false;

	// Number of buffers for advertisement / scan response data.
	const static uint8_t                        _advertisementDataBufferCount = 2;

	// Size of advertisement / scan response data buffers.
	const static uint8_t                        _advertisementDataBufferSize = BLE_GAP_ADV_SET_DATA_SIZE_MAX;

	// Pointers to advertisement data buffers.
	uint8_t*                                    _advertisementDataBuffers[_advertisementDataBufferCount] = { nullptr };

	// Pointers to scan response data buffers.
	uint8_t*                                    _scanResponseBuffers[_advertisementDataBufferCount] = { nullptr };

	// Pointers to the currently advertised advertisement and scan response data buffer.
	ble_gap_adv_data_t                          _advData;

	// Which of the advertisement and scan response data buffers are currently being advertised..
	uint8_t                                     _advBufferInUse = 1;



	/**
	 * Set advertisement parameters from member variables.
	 */
	void configureAdvertisementParameters();

	/**
	 * Sets the advertisement data.
	 *
	 * @param[in] serviceData          The Crownstone service data class.
	 * @param[in] asScanResponse       Whether to set the service data in the scan response.
	 */
	void setAdvertisementData(ServiceData& serviceData, bool asScanResponse);

	/**
	 * Sets the advertisement data.
	 *
	 * Writes to _config_advdata.
	 *
	 * @param[in] beacon               The iBeacon data.
	 * @param[in] asScanResponse       Whether to set the iBeacon data in the scan response.
	 */
	void setAdvertisementData(IBeacon& beacon, bool asScanResponse);

	/**
	 * Allocate the advertisement data buffers.
	 *
	 * Returns true on success.
	 */
	bool allocateAdvertisementDataBuffers(bool scanResponse);

	/**
	 * Sets and updates advertisement data.
	 *
	 * Sets _advData, and sets it at softdevice if there's a valid handle.
	 * Uses a different buffer than previous time.
	 */
	void updateAdvertisementData();

	/**
	 * Updates the advertisement parameters.
	 *
	 * When parameters are changed, advertising will be stopped and started again.
	 * Else, only the service data is updated.
	 */
	void updateAdvertisementParams();

	/**
	 * Set the cached TX power at the softdevice.
	 *
	 * Can only be done once the advHandle is set.
	 */
	void updateTxPower();

	/**
	 * Stop and start advertising.
	 *
	 * This will make sure new advertising parameters are applied.
	 */
	void restartAdvertising();

	/**
	 * Sets advertisement parameters to be connectable.
	 */
	void setConnectableAdvParams();

	/**
	 * Sets advertisement parameters to be non-connectable.
	 */
	void setNonConnectableAdvParams();


	void onConnect();

	void onDisconnect();

	void onConnectOutgoing();

	void printAdvertisement();
};
