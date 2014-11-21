/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */


//#include <cmath> // try not to use this!
#include <cstdio>

#include <services/IndoorLocalisationService.h>
#include <common/config.h>
#include <common/boards.h>
#include <drivers/nrf_adc.h>
#include <drivers/nrf_rtc.h>

#include "nRF51822.h"

//#include <common/timer.h>

using namespace BLEpp;

IndoorLocalizationService::IndoorLocalizationService(Nrf51822BluetoothStack& _stack) :
		_stack(&_stack),
		_rssiCharac(NULL), _intChar(NULL), _intChar2(NULL), _peripheralCharac(NULL),
		_personalThresholdLevel(0) {

	setUUID(UUID(INDOORLOCALISATION_UUID));
	//setUUID(UUID(0x3800)); // there is no BLE_UUID for indoor localization (yet)

	// we have to figure out why this goes wrong
//	setName(std::string("IndoorLocalizationService"));

//	// set timer with compare interrupt every 10ms
//	timer_config(10);
}

void IndoorLocalizationService::addSpecificCharacteristics() {
	addSignalStrengthCharacteristic();
//	addNumberCharacteristic();
//	addNumber2Characteristic();
	addScanControlCharacteristic();
	addPeripheralListCharacteristic();
	addPersonalThresholdCharacteristic();
	addDeviceTypeCharactersitic();
	addRoomCharacteristic();
}

void IndoorLocalizationService::addSignalStrengthCharacteristic() {
//	LOGd("create characteristic to read signal strength");
	_rssiCharac = new CharacteristicT<int8_t>();
	_rssiCharac->setUUID(UUID(getUUID(), RSSI_UUID)); // there is no BLE_UUID for rssi level(?)
	_rssiCharac->setName(std::string("RSSI"));
	_rssiCharac->setDefaultValue(1);
	_rssiCharac->setNotifies(true);

	addCharacteristic(_rssiCharac);
}

void IndoorLocalizationService::addNumberCharacteristic() {
	// create a characteristic of type uint8_t (unsigned one byte integer).
	// this characteristic is by default read-only (for the user)
	// note that in the next characteristic this variable intchar is set!
//	log(DEBUG, "create characteristic to read a number for debugging");
	//Characteristic<uint8_t>&
	_intChar = createCharacteristicRef<uint8_t>();
	(*_intChar)
		.setUUID(UUID(getUUID(), 0x125))  // based off the uuid of the service.
		.setDefaultValue(66)
		.setName("number");
}

void IndoorLocalizationService::addNumber2Characteristic() {
	_intChar2 = createCharacteristicRef<uint64_t>();
	(*_intChar2)
		.setUUID(UUID(getUUID(), 0x121))  // based off the uuid of the service.
		.setDefaultValue(66)
		.setName("number2");
}

void IndoorLocalizationService::addScanControlCharacteristic() {
	// set scanning option
//	log(DEBUG, "create characteristic to stop/start scan");
	createCharacteristic<uint8_t>()
		.setUUID(UUID(getUUID(), SCAN_DEVICE_UUID))
		.setName("scan")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
			if(value) {
//				log(INFO,"crown: start scanning");
				if (!_stack->isScanning()) {
//					_scanResult.reset();
					_scanResult.init();
					_stack->startScanning();
				}
			} else {
//				log(INFO,"crown: stop scanning");
				if (_stack->isScanning()) {
					_stack->stopScanning();
					*_peripheralCharac = _scanResult;
					_scanResult.print();
				}
			}
		});
}

void IndoorLocalizationService::addPeripheralListCharacteristic() {
	// get scan result
//	log(DEBUG, "create characteristic to list found peripherals");

	_peripheralCharac = createCharacteristicRef<ScanResult>();
	_peripheralCharac->setUUID(UUID(getUUID(), LIST_DEVICE_UUID));
	_peripheralCharac->setName("Devices");
	_peripheralCharac->setWritable(false);
	_peripheralCharac->setNotifies(true);
}

void IndoorLocalizationService::addPersonalThresholdCharacteristic() {
	// set threshold level
//	log(DEBUG, "create characteristic to write personal threshold level");
	createCharacteristic<uint8_t>()
		.setUUID(UUID(getUUID(), PERSONAL_THRESHOLD_UUID))
		.setName("Threshold")
		.setDefaultValue(0)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
			nrf_pwm_set_value(0, value);
//			nrf_pwm_set_value(1, value);
//			nrf_pwm_set_value(2, value);
//			log(INFO, "set personal_threshold_value to %i", value);
			_personalThresholdLevel = value;
		});
}

void IndoorLocalizationService::addDeviceTypeCharactersitic() {
//	LOGd("create characteristic to read/write device type");
	createCharacteristic<std::string>()
		.setUUID(UUID(getUUID(), DEVICE_TYPE_UUID))
		.setName("Device Type")
		.setDefaultValue("Unknown")
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
//			LOGi("set device type to: %s", value.c_str());
			// TODO: impelement persistent storage of device type
		});
}

void IndoorLocalizationService::addRoomCharacteristic() {
//	LOGd("create characteristic to read/write room");
	createCharacteristic<std::string>()
		.setUUID(UUID(getUUID(), ROOM_UUID))
		.setName("Room")
		.setDefaultValue("Unknown")
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
//			LOGi("set room to: %s", value.c_str());
			// TODO: impelement persistent storage of room
		});
}


IndoorLocalizationService& IndoorLocalizationService::createService(Nrf51822BluetoothStack& _stack) {
//	LOGd("Create indoor localisation service");
	IndoorLocalizationService* svc = new IndoorLocalizationService(_stack);
	_stack.addService(svc);
	svc->addSpecificCharacteristics();
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

#if(SOFTDEVICE_SERIES != 110)
	case BLE_GAP_EVT_ADV_REPORT:
		onAdvertisement(&p_ble_evt->evt.gap_evt.params.adv_report);
		break;
#endif

	default: {
	}
	}
}

void IndoorLocalizationService::onRSSIChanged(int8_t rssi) {

#ifdef RGB_LED
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
#endif

	setRSSILevel(rssi);
}

void IndoorLocalizationService::setRSSILevel(int8_t RSSILevel) {
	if (_rssiCharac) {
		*_rssiCharac = RSSILevel;
	}
}

void IndoorLocalizationService::setRSSILevelHandler(func_t func) {
	_rssiHandler = func;
}

void IndoorLocalizationService::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {
	if (_stack->isScanning()) {
		_scanResult.update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
	}
}
