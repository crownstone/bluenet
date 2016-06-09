/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include <events/cs_EventListener.h>

//! The update frequence of the Tick routine in this service
#define LOCALIZATION_SERVICE_UPDATE_FREQUENCY 10

//#define PWM_ON_RSSI

/** The IndoorLocalizationService handles scanning, signal strengths, tracked devices, etc.
 */
class IndoorLocalizationService : public BLEpp::Service, EventListener {

protected:
	void addRssiCharacteristic();
	void addScanControlCharacteristic();
	void addScannedDeviceListCharacteristic();
	void addTrackedDeviceListCharacteristic();
	void addTrackedDeviceCharacteristic();

	void writeTrackedDevices();
	void readTrackedDevices();

	void startTracking();
	void stopTracking();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

public:
	IndoorLocalizationService();

	void tick();

	void scheduleNextTick();

	/** Initialize a IndoorLocalization object
	 * @stack Bluetooth Stack to attach this service to
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void addCharacteristics();

	/** Sets the number of ticks the rssi of a device is not above threshold before a device is considered not nearby. */
	void setNearbyTimeout(uint16_t counts);

	/** Returns the number of ticks the rssi of a device is not above threshold before a device is considered not nearby. */
	uint16_t getNearbyTimeout();

	void on_ble_event(ble_evt_t * p_ble_evt);

	void onRSSIChanged(int8_t rssi);
	void setRSSILevel(int8_t RSSILevel);

private:
	BLEpp::Characteristic<int8_t>* _rssiCharac;
	BLEpp::Characteristic<uint8_t>* _scanControlCharac;
	BLEpp::Characteristic<buffer_ptr_t>* _scannedDeviceListCharac;
	BLEpp::Characteristic<buffer_ptr_t>* _trackedDeviceListCharac;
	BLEpp::Characteristic<buffer_ptr_t>* _trackedDeviceCharac;

#ifdef PWM_ON_RSSI
	int16_t _averageRssi;
#endif

};
