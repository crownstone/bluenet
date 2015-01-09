/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef INDOORLOCALISATIONSERVICE_H_
#define INDOORLOCALISATIONSERVICE_H_

#include "BluetoothLE.h"
#include <util/function.h>
#include <characteristics/ScanResult.h>
#include <characteristics/TrackDevices.h>
#include <characteristics/charac_config.h>

#define INDOORLOCALISATION_UUID "7e170000-429c-41aa-83d7-d91220abeb33"

class IndoorLocalizationService : public BLEpp::GenericService {

public:
	typedef function<int8_t()> func_t;

protected:
	// TODO -oDE: are really all of these characteristics part of the
	//   indoor localisation?
	void addSignalStrengthCharacteristic();
	void addScanControlCharacteristic();
	void addPeripheralListCharacteristic();
	void addTrackedDeviceListCharacteristic();
	void addTrackedDeviceCharacteristic();

	void addDeviceTypeCharactersitic();
	void addRoomCharacteristic();

public:
	IndoorLocalizationService(BLEpp::Nrf51822BluetoothStack& stack);

//	void addSpecificCharacteristics();

	void on_ble_event(ble_evt_t * p_ble_evt);

	void onRSSIChanged(int8_t rssi);
	void setRSSILevel(int8_t RSSILevel);
	void setRSSILevelHandler(func_t func);

#if(SOFTDEVICE_SERIES != 110)
	void onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report);
#endif
	static IndoorLocalizationService& createService(BLEpp::Nrf51822BluetoothStack& stack);
private:
	BLEpp::Nrf51822BluetoothStack* _stack;

	BLEpp::CharacteristicT<int8_t>* _rssiCharac;
	BLEpp::Characteristic<ScanResult>* _peripheralCharac;
	BLEpp::Characteristic<TrackedDeviceList>* _trackedDeviceListCharac;
	BLEpp::Characteristic<TrackedDevice>* _trackedDeviceCharac;
	
	func_t _rssiHandler;

	ScanResult* _scanResult;
	TrackedDeviceList _trackedDeviceList;

};

#endif /* INDOORLOCALISATIONSERVICE_H_ */
