/*
 * IndoorLocalisationService.h
 *
 *  Created on: Oct 21, 2014
 *      Author: dominik
 */

#ifndef INDOORLOCALISATIONSERVICE_H_
#define INDOORLOCALISATIONSERVICE_H_

#include "BluetoothLE.h"
#include "function.h"

#define INDOORLOCALISATION_UUID "00002220-0000-1000-8000-00805f9b34fb"
// TODO -oDE: how did you come up with this UUID ??!!
//  if I use any other UUID, the service UUID in the advertisement package
//  is sent out as 128-bit, but with this UUID it is sent out as 16-bit ?!

using namespace BLEpp;

class IndoorLocalizationService : public GenericService {

public:
	typedef function<int8_t()> func_t;

protected:
	CharacteristicT<int8_t> *_characteristic;
	func_t _rssiHandler;

public:
	IndoorLocalizationService();

	void on_ble_event(ble_evt_t * p_ble_evt);

	void onRSSIChanged(int8_t rssi);
	void setRSSILevel(int8_t RSSILevel);
	void setRSSILevelHandler(func_t func);

	static IndoorLocalizationService& createService(Nrf51822BluetoothStack& stack);
};

#endif /* INDOORLOCALISATIONSERVICE_H_ */
