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
#include "characteristics/ScanResult.h"

#include <drivers/nrf_adc.h>

#define INDOORLOCALISATION_UUID "00002220-0000-1000-8000-00805f9b34fb"
// TODO -oDE: how did you come up with this UUID ??!!
//  if I use any other UUID, the service UUID in the advertisement package
//  is sent out as 128-bit, but with this UUID it is sent out as 16-bit ?!

//using namespace BLEpp;

class IndoorLocalizationService : public BLEpp::GenericService {

public:
	typedef function<int8_t()> func_t;

protected:
	// TODO -oDE: are really all of these characteristics part of the
	//   indoor localisation?
	void AddSignalStrengthCharacteristic();
	void AddNumberCharacteristic();
	void AddNumber2Characteristic();
	void AddVoltageCurveCharacteristic();
	void AddScanControlCharacteristic();
	void AddPeripheralListCharacteristic();
	void AddPersonalThresholdCharacteristic();

	void SampleAdcInit();
	void SampleAdcStart();

public:
	IndoorLocalizationService(BLEpp::Nrf51822BluetoothStack& stack, ADC &adc);

	void AddSpecificCharacteristics();

	void on_ble_event(ble_evt_t * p_ble_evt);

	void onRSSIChanged(int8_t rssi);
	void setRSSILevel(int8_t RSSILevel);
	void setRSSILevelHandler(func_t func);

	void onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report);

	static IndoorLocalizationService& createService(BLEpp::Nrf51822BluetoothStack& stack, ADC &adc);
private:
	BLEpp::Nrf51822BluetoothStack* _stack;

	BLEpp::CharacteristicT<int8_t>* _rssiCharac;
	BLEpp::Characteristic<uint8_t>* _intChar;
	BLEpp::Characteristic<uint64_t>* _intChar2;
	BLEpp::Characteristic<ScanResult>* _peripheralCharac;
	
	func_t _rssiHandler;

	int _personalThresholdLevel;
	ScanResult _scanResult;

	ADC &_adc;
};

#endif /* INDOORLOCALISATIONSERVICE_H_ */
