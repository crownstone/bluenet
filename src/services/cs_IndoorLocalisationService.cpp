/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */


//#include <cmath> // try not to use this!
#include <cstdio>

#include "common/cs_Boards.h"
#include "common/cs_Config.h"
#include "cs_nRF51822.h"
#include "drivers/cs_ADC.h"
#include "drivers/cs_RTC.h"
#include "services/cs_IndoorLocalisationService.h"

//#include <common/timer.h>

using namespace BLEpp;

IndoorLocalizationService::IndoorLocalizationService(Nrf51822BluetoothStack& _stack) :
		_stack(&_stack),
		_rssiCharac(NULL), _peripheralCharac(NULL),
		_trackedDeviceListCharac(NULL), _trackedDeviceCharac(NULL), _trackIsNearby(false),
		_initialized(false), _scanMode(false),
		_scanResult(NULL), _trackedDeviceList(NULL)
{

	setUUID(UUID(INDOORLOCALISATION_UUID));
	//setUUID(UUID(0x3800)); // there is no BLE_UUID for indoor localization (yet)

	// we have to figure out why this goes wrong
	setName(std::string("IndoorLocalizationService"));
	
	characStatus.reserve(5);

	characStatus.push_back( { "Received signal level",
		RSSI_UUID,
		false,
		static_cast<addCharacteristicFunc>(&IndoorLocalizationService::addSignalStrengthCharacteristic)});
	characStatus.push_back( { "Start/Stop Scan",
		SCAN_DEVICE_UUID,
		false,
		static_cast<addCharacteristicFunc>(&IndoorLocalizationService::addScanControlCharacteristic)});
	characStatus.push_back( { "Peripherals",
		LIST_DEVICE_UUID,
		false,
		static_cast<addCharacteristicFunc>(&IndoorLocalizationService::addPeripheralListCharacteristic)});
	characStatus.push_back( { "List tracked devices",
		TRACKED_DEVICE_LIST_UUID,
		true,
		static_cast<addCharacteristicFunc>(&IndoorLocalizationService::addTrackedDeviceListCharacteristic)});
	characStatus.push_back( { "Add tracked device",
		TRACKED_DEVICE_UUID,
		true,
		static_cast<addCharacteristicFunc>(&IndoorLocalizationService::addTrackedDeviceCharacteristic)});

	_trackMode = false;
//	// set timer with compare interrupt every 10ms
//	timer_config(10);

	Storage::getInstance().getHandle(PS_ID_INDOORLOCALISATION_SERVICE, _storageHandle);
	loadPersistentStorage();
}

void IndoorLocalizationService::loadPersistentStorage() {
	Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void IndoorLocalizationService::savePersistentStorage() {
	Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void IndoorLocalizationService::tick() {

	if (!_initialized) {
		readTrackedDevices();
		if (!_trackedDeviceList->isEmpty()) {
			startTracking();
		}
		_initialized = true;
	}

	if (!_trackMode) return;

	// This function checks the counter for each device
	// If no device is nearby, turn off the light
	bool deviceNearby = false;
	if (_trackedDeviceList != NULL) {
		deviceNearby = (_trackedDeviceList->isNearby() == TDL_IS_NEARBY);
	}

	// Change PWM only on change of nearby state
	if (deviceNearby && !_trackIsNearby) {
		PWM::getInstance().setValue(0, (uint8_t)-1);
	} else if (!deviceNearby && _trackIsNearby) {
		PWM::getInstance().setValue(0, 0);
	}
	_trackIsNearby = deviceNearby;
}

void IndoorLocalizationService::addSignalStrengthCharacteristic() {
//	LOGd("create characteristic to read signal strength");
	_rssiCharac = new CharacteristicT<int8_t>();
	_rssiCharac->setUUID(UUID(getUUID(), RSSI_UUID)); // there is no BLE_UUID for rssi level(?)
	_rssiCharac->setName(std::string("Received signal level"));
	_rssiCharac->setDefaultValue(1);
	_rssiCharac->setNotifies(true);

	addCharacteristic(_rssiCharac);
}

void IndoorLocalizationService::addScanControlCharacteristic() {
	// set scanning option
//	LOGd("create characteristic to stop/start scan");
	createCharacteristic<uint8_t>()
		.setUUID(UUID(getUUID(), SCAN_DEVICE_UUID))
		.setName("scan")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
			if(value) {
				LOGi("Init scan result");
				if (_scanResult != NULL) {
					_scanResult->init();
				}
				if (!_stack->isScanning()) {
					_stack->startScanning();
				}
				_scanMode = true;
			} else {
				LOGi("Return scan result");
				if (_scanResult != NULL) {
					*_peripheralCharac = *_scanResult;
					_scanResult->print();
				}
				// Only stop scanning if we're not also tracking devices
				if (_stack->isScanning() && !_trackMode) {
					_stack->stopScanning();
				}
				_scanMode = false;
			}
		});
}

void IndoorLocalizationService::addPeripheralListCharacteristic() {
	// get scan result
//	LOGd("create characteristic to list found peripherals");
	_scanResult = new ScanResult();
	_peripheralCharac = createCharacteristicRef<ScanResult>();
	_peripheralCharac->setUUID(UUID(getUUID(), LIST_DEVICE_UUID));
	_peripheralCharac->setName("Devices");
	_peripheralCharac->setWritable(false);
	_peripheralCharac->setNotifies(true);
}

void IndoorLocalizationService::addTrackedDeviceListCharacteristic() {
	
	_trackedDeviceList = new TrackedDeviceList();

	_trackedDeviceListCharac = createCharacteristicRef<TrackedDeviceList>();
	_trackedDeviceListCharac->setUUID(UUID(getUUID(), TRACKED_DEVICE_LIST_UUID));
	_trackedDeviceListCharac->setName("List tracked devices");
	_trackedDeviceListCharac->setWritable(false);
	_trackedDeviceListCharac->setNotifies(false);
	
	// Initialize before adding tracked devices!
	_trackedDeviceList->init();

	// Load the nearby timeout
	uint16_t counts;
	Storage::getUint16(_storageStruct.nearbyTimeout, counts, TRACKDEVICE_DEFAULT_TIMEOUT_COUNT);
	_trackedDeviceList->setTimeout(counts);
}

void IndoorLocalizationService::writeTrackedDevices() {
	uint16_t length = _trackedDeviceList->getSerializedLength();
	uint8_t* buffer = (uint8_t*)calloc(length, sizeof(uint8_t));
	_trackedDeviceList->serialize(buffer, length);
	Storage::setArray(buffer, _storageStruct.trackedDevices.list, length);
}

void IndoorLocalizationService::readTrackedDevices() {
	uint16_t length = _trackedDeviceList->getMaxLength();
	uint8_t* buffer = (uint8_t*)calloc(length, sizeof(uint8_t));
	Storage::getArray(_storageStruct.trackedDevices.list, buffer, (uint8_t*)NULL, length);
	_trackedDeviceList->deserialize(buffer, length);

	if (!_trackedDeviceList->isEmpty()) {
		LOGi("restored tracked devices (%d):", _trackedDeviceList->getSize());
		_trackedDeviceList->print();
	}

	*_trackedDeviceListCharac = *_trackedDeviceList;
}

void IndoorLocalizationService::setNearbyTimeout(uint16_t counts) {
	if (_trackedDeviceList != NULL) {
		_trackedDeviceList->setTimeout(counts);
	}
	Storage::setUint16(counts, _storageStruct.nearbyTimeout);
}

uint16_t IndoorLocalizationService::getNearbyTimeout() {
	if (_trackedDeviceList == NULL) {
		return 0;
	}
	loadPersistentStorage();
	return _trackedDeviceList->getTimeout();
}

void IndoorLocalizationService::startTracking() {
	if (!_trackMode) {
		// Set to some value initially
		_trackIsNearby = false;
	}
	_trackMode = true;
	if (!_stack->isScanning()) {
		LOGi("Start tracking");
		_stack->startScanning();
	}
}

void IndoorLocalizationService::stopTracking() {
	_trackMode = false;
	if (_stack->isScanning()) {
		LOGi("Stop tracking");
		_stack->stopScanning();
	}
}

void IndoorLocalizationService::addTrackedDeviceCharacteristic() {
	_trackedDeviceCharac = createCharacteristicRef<TrackedDevice>();
	_trackedDeviceCharac->setUUID(UUID(getUUID(), TRACKED_DEVICE_UUID));
	_trackedDeviceCharac->setName("Add tracked device");
	_trackedDeviceCharac->setWritable(true);
	_trackedDeviceCharac->setNotifies(false);
//	_trackedDeviceCharac->setDefaultValue();
	_trackedDeviceCharac->onWrite([&](const TrackedDevice& value) -> void {
		if (value.getRSSI() > 0) {
			LOGi("Remove tracked device");
			value.print();
			_trackedDeviceList->rem(value.getAddress());
		}
		else {
			LOGi("Add tracked device");
			value.print();
			_trackedDeviceList->add(value.getAddress(), value.getRSSI());
		}
		// Update list
		*_trackedDeviceListCharac = *_trackedDeviceList;

		LOGi("currently tracking devices:");
		_trackedDeviceList->print();

		// store in persistent memory
		writeTrackedDevices();
		savePersistentStorage();

		if (_trackedDeviceList->getSize() > 0) {
			startTracking();
		}
		else {
			stopTracking();
		}
	});
}

IndoorLocalizationService& IndoorLocalizationService::createService(Nrf51822BluetoothStack& _stack) {
	LOGd("Create indoor localisation service");
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
	PWM::getInstance().setValue(0, sin_table[sine_index]);
	PWM::getInstance().setValue(1, sin_table[(sine_index + 33) % 100]);
	PWM::getInstance().setValue(2, sin_table[(sine_index + 66) % 100]);
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

#if(SOFTDEVICE_SERIES != 110)
void IndoorLocalizationService::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {
	if (_stack->isScanning()) {
		if (_scanMode && _scanResult != NULL) {
			_scanResult->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
		}
		if (_trackMode && _trackedDeviceList != NULL) {
			_trackedDeviceList->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
		}
	}
}
#endif

