/*
 * IndoorLocalisationService.cpp
 *
 *  Created on: Oct 21, 2014
 *      Author: dominik
 */

#include "IndoorLocalisationService.h"

IndoorLocalizationService::IndoorLocalizationService() {
	setUUID(UUID(INDOORLOCALISATION_UUID));
	//setUUID(UUID(0x3800)); // there is no BLE_UUID for indoor localization (yet)

	// we have to figure out why this goes wrong
	setName(std::string("IndoorLocalizationService"));

	_characteristic = new CharacteristicT<int8_t>();
	//.setUUID(UUID(service.getUUID(), 0x124))
	(*_characteristic).setUUID(UUID(getUUID(), 0x2201)); // there is no BLE_UUID for rssi level(?)
	(*_characteristic).setName(std::string("Received signal level"));
	(*_characteristic).setDefaultValue(1);

	addCharacteristic(_characteristic);
}

IndoorLocalizationService& IndoorLocalizationService::createService(Nrf51822BluetoothStack& stack) {
    IndoorLocalizationService* svc = new IndoorLocalizationService();
    stack.addService(svc);
    return *svc;
}

void IndoorLocalizationService::on_ble_event(ble_evt_t * p_ble_evt) {
	Service::on_ble_event(p_ble_evt);
	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED: {
		sd_ble_gap_rssi_start(p_ble_evt->evt.gap_evt.conn_handle);
		break;
	}
	case BLE_GAP_EVT_DISCONNECTED: {
		sd_ble_gap_rssi_stop(p_ble_evt->evt.gap_evt.conn_handle);
		break;
	}
	case BLE_GAP_EVT_RSSI_CHANGED: {
		onRSSIChanged(p_ble_evt->evt.gap_evt.params.rssi_changed.rssi);
		break;
	}
	default: {
	}
	}
}

void IndoorLocalizationService::onRSSIChanged(int8_t rssi) {

	// set LED here
	int sine_index = (rssi - 170) * 2;
	if (sine_index < 0) sine_index = 0;
	if (sine_index > 100) sine_index = 100;
	//			__asm("BKPT");
	//			int sine_index = (rssi % 10) *10;
	nrf_pwm_set_value(0, sin_table[sine_index]);
	nrf_pwm_set_value(1, sin_table[(sine_index + 33) % 100]);
	nrf_pwm_set_value(2, sin_table[(sine_index + 66) % 100]);
	//			counter = (counter + 1) % 100;

	// Add a delay to control the speed of the sine wave
	nrf_delay_us(8000);

	setRSSILevel(rssi);
}

void IndoorLocalizationService::setRSSILevel(int8_t RSSILevel) {
	(*_characteristic) = RSSILevel;
}

void IndoorLocalizationService::setRSSILevelHandler(func_t func) {
	_rssiHandler = func;
}


