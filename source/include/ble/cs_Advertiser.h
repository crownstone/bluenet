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
	//! Gets a static singleton (no dynamic memory allocation)
	static Advertiser& getInstance() {
		static Advertiser instance;
		return instance;
	}
	Advertiser(Advertiser const&) = delete;
	void operator=(Advertiser const&) = delete;

	//! Initialize the advertiser.
	void init();

	//! Set initial appearance (for example BLE_APPEARANCE_GENERIC_TAG), not applied unless done before init.
	void setAppearance(uint16_t appearance);

	//! Set initial device name, not applied unless done before radio init.
	void setDeviceName(const std::string& deviceName);

	//! Set initial advertising interval in 0.625 ms units, not applied unless done before init.
	void setAdvertisingInterval(uint16_t advertisingInterval);

	//! Set initial advertising timeout in seconds. Set to 0 for no timeout.
	void setAdvertisingTimeoutSeconds(uint16_t advertisingTimeoutSeconds);

	//! Update the appearance (for example BLE_APPEARANCE_GENERIC_TAG).
	void updateAppearance(uint16_t appearance);

	//! Update the advertising interval in 0.625ms units.
	void updateAdvertisingInterval(uint16_t advertisingInterval);

	//! Update the device name.
	void updateDeviceName(const std::string& deviceName);

	std::string & getDeviceName();

	/**
	 * Set and update radio transmit power.
	 *
	 * @param[in] powerLevel           Power in dBm. Accepted values are -40, -20, -16, -12, -8, -4, 0, and 4.
	 */
	void updateTxPower(int8_t power);

	void changeToLowTxPower();

	void changeToNormalTxPower();

	int8_t getTxPower();

	/**
	 * Give the stack a pointer to the crownstone service data.
	 *
	 * This is used to set the advertisement data.
	 * 13-sep-2019 TODO: Get this data in some other way?
	 */
	void setServiceData(ServiceData* serviceData) {
		_serviceData = serviceData;
	}

	/**
	 * Sets and updates the advertisement data.
	 *
	 * @param[in] beacon               Object with the iBeacon parameters.
	 * @param[in] deviceType           Device type, for example DEVICE_CROWNSTONE_PLUG.
	 */
	void configureAdvertisement(IBeacon* beacon, uint8_t deviceType);

	/**
	 * Sets and updates the advertisement data.
	 *
	 * @param[in] deviceType           Device type, for example DEVICE_CROWNSTONE_PLUG.
	 */
	void configureAdvertisement(uint8_t deviceType);

	/**
	 * Start advertising.
	 *
	 * @return nrf return code.
	 */
	uint32_t startAdvertising();

	/**
	 * Stop advertising.
	 */
	void stopAdvertising();

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
	 * Set whether to advertise being connectable.
	 *
	 * Call updateAdvertisementParams() to apply.
	 *
	 * @param[in] connectable          True when connectable.
	 */
	void setConnectable(bool connectable);

	//! Handle events as EventListener
	void handleEvent(event_t & event);


	//! The default BLE appearance, for unknown reason set to generic tag.
	// See: https://devzone.nordicsemi.com/question/2973/an36-ble_appearance/
	static const uint16_t                  defaultAppearance = BLE_APPEARANCE_GENERIC_TAG;
	//! The low-frequency clock, currently generated from the high frequency clock
	static const nrf_clock_lf_cfg_t        defaultClockSource;
	//! Advertising interval in 0.625 ms.
	static const uint16_t                  defaultAdvertisingInterval_0_625_ms = g_ADVERTISEMENT_INTERVAL;
	//! Time after which advertising stops.
	static const uint16_t                  defaultAdvertisingTimeout_seconds = ADVERTISING_TIMEOUT;

protected:
	///////////////////// Advertising /////////////////////
	// 13-sep-2019 TODO: move advertising to separate class.

	//! Whether the advertiser has been initialized.
	bool                                        _isInitialized = false;

	//! Pointer to the stack.
	Stack*                                      _stack;

	//! Whether currently advertising.
	bool                                        _advertising = false;

	//! Whether we should be advertising.
	bool                                        _wantAdvertising = false;

	//! Advertisement handle for softdevice. Set by first call to: sd_ble_gap_adv_set_configure().
	uint8_t                                     _advHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

	//! Advertised name
	std::string                                 _deviceName = "none";

	//! Advertised appearance.
	uint16_t                                    _appearance = BLE_APPEARANCE_GENERIC_TAG;

	//! Transmit power.
	int8_t                                      _txPower = 0;

	//! Advertisement parameter: interval.
	uint16_t                                    _advertisingInterval = defaultAdvertisingInterval_0_625_ms;

	//! Advertisement parameter: timeout.
	uint16_t                                    _advertisingTimeout = defaultAdvertisingTimeout_seconds;

	//! Advertisement parameters for softdevice.
	ble_gap_adv_params_t                        _advParams;

	//! Whether advertising parameters have been changed.
	bool                                        _advParamsChanged = false;

	//! Whether we want to advertise being connectable.
	bool                                        _wantConnectable = true;

	//! Whether the advertisement is connectable.
	bool                                        _isConnectable = false;

	//! Whether the advertisement data should be used.
	bool                                        _includeAdvertisementData = false;

	//! Whether the scan response data should be used.
	bool                                        _includeScanResponseData = false;

	//! Advertisement data config.
	ble_advdata_t                               _configAdvertisementData;

	//! Scan response data config.
	ble_advdata_t                               _configScanResponse;

	//! Pointers to advertisement data buffers. Buffers will be allocated on first use.
	uint8_t*                                    _advertisementDataBuffers[2] = { NULL };

	//! Pointers to scan response data buffers. Buffers will be allocated on first use.
	uint8_t*                                    _scanResponseBuffers[2] = { NULL };

	//! Encoded advertisement data, has pointers to buffers.
	ble_gap_adv_data_t                          _advData;

	//! Which of the advertisement data buffers is in use by the softdevice.
	uint8_t                                     _advBufferInUse = 1;

	//! iBeacon data field to be encoded into advertisement data.
	ble_advdata_manuf_data_t 					_ibeaconManufData;

	//! Service data field to be encoded into advertisement data.
	ble_advdata_service_data_t                  _crownstoneServiceData;

	//! Pointer to up to data crownstone service data.
	ServiceData*                                _serviceData = NULL;

	/**
	 * Set advertisement parameters from member variables.
	 */
	void configureAdvertisementParameters();

	/**
	 * Sets crownstone service data as advertisement data.
	 *
	 * Writes to _config_advdata or _config_scanrsp.
	 *
	 * @param[in] deviceType           Type of device (e.g. DEVICE_CROWNSTONE_PLUG).
	 * @param[in] asScanResponse       Whether the advertisement data is scan response.
	 */
	void configureServiceData(uint8_t deviceType, bool asScanResponse);

	/**
	 * Sets iBeacon as advertisement data.
	 *
	 * Writes to _config_advdata.
	 *
	 * @param[in] beacon               The iBeacon data.
	 */
	void configureIBeaconAdvData(IBeacon* beacon);

	/**
	 * Stop and start advertising.
	 *
	 * This will make sure new advertising parameters are applied.
	 */
	void restartAdvertising();

	void printAdvertisement();

	/**
	 * Sets advertisement parameters to be connectable.
	 */
	void setConnectableAdvParams();

	/**
	 * Sets advertisement parameters to be non-connectable.
	 */
	void setNonConnectableAdvParams();

	/**
	 * Updates the TX power.
	 */
	void updateTxPower();

	/**
	 * Check if advertiser is initialized.
	 */
	bool isInitialized(bool expected=true);

	void onConnect();

	void onDisconnect();

	void onConnectOutgoing();
};
