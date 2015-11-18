/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "services/cs_IndoorLocalisationService.h"

//#include <cmath> // try not to use this!
//#include <cstdio>
//
//#include "common/cs_Boards.h"
//#include "common/cs_Config.h"
//#include "cs_nRF51822.h"
//#include "drivers/cs_ADC.h"
#include "drivers/cs_RTC.h"
#include "drivers/cs_PWM.h"
//
//#include "util/cs_Utils.h"
//
#include "structs/buffer/cs_MasterBuffer.h"
//
#include "cfg/cs_UuidConfig.h"

#include "drivers/cs_Timer.h"

#include <cfg/cs_Settings.h>

//#include <common/cs_Strings.h>

using namespace BLEpp;

IndoorLocalizationService::IndoorLocalizationService() :
		_rssiCharac(NULL), _peripheralCharac(NULL),
		_trackedDeviceListCharac(NULL), _trackedDeviceCharac(NULL), _trackIsNearby(false),
		_initialized(false), _scanMode(false),
//		_scanResult(NULL),
		_trackedDeviceList(NULL)
{

	setUUID(UUID(INDOORLOCALISATION_UUID));

	// we have to figure out why this goes wrong
	setName(BLE_SERVICE_INDOOR_LOCALIZATION);
	
	_trackMode = false;
//	// set timer with compare interrupt every 10ms
//	timer_config(10);

	Storage::getInstance().getHandle(PS_ID_INDOORLOCALISATION_SERVICE, _storageHandle);
	loadPersistentStorage();

	init();

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)IndoorLocalizationService::staticTick);
}

void IndoorLocalizationService::init() {
	LOGi("Create indoor localization service");

#if CHAR_RSSI==1
	LOGi("add Signal Strength characteristics");
	addSignalStrengthCharacteristic();
#else
	LOGi("skip Signal Strength characteristics");
#endif
#if CHAR_SCAN_DEVICES==1
	LOGi("add Scan Devices characteristics");
	addScanControlCharacteristic();
	addPeripheralListCharacteristic();
#else
	LOGi("skip Scan/Devices characteristics");
#endif
#if CHAR_TRACK_DEVICES==1
	LOGi("add Tracked Device characteristics");
	addTrackedDeviceListCharacteristic();
	addTrackedDeviceCharacteristic();
#else
	LOGi("skip Tracked Device characteristics");
#endif

}

void IndoorLocalizationService::tick() {
//	LOGi("Tack: %d", RTC::now());

	if (!_initialized) {
		if (_trackedDeviceList != NULL) {
			readTrackedDevices();
			if (!_trackedDeviceList->isEmpty()) {
				startTracking();
			}
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

	scheduleNextTick();
}

void IndoorLocalizationService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(LOCALIZATION_SERVICE_UPDATE_FREQUENCY), this);
}

void IndoorLocalizationService::loadPersistentStorage() {
	Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void IndoorLocalizationService::savePersistentStorage() {
	Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void IndoorLocalizationService::addSignalStrengthCharacteristic() {
//	LOGd("create characteristic to read signal strength");
	_rssiCharac = new Characteristic<int8_t>();
	addCharacteristic(_rssiCharac);

	_rssiCharac->setUUID(UUID(getUUID(), RSSI_UUID)); // there is no BLE_UUID for rssi level(?)
	_rssiCharac->setName(BLE_CHAR_RSSI);
	_rssiCharac->setDefaultValue(1);
	_rssiCharac->setNotifies(true);
}

void IndoorLocalizationService::addScanControlCharacteristic() {
	// set scanning option
//	LOGd("create characteristic to stop/start scan");
	_scanControlCharac = new Characteristic<uint8_t>();
	addCharacteristic(_scanControlCharac);

	_scanControlCharac->setUUID(UUID(getUUID(), SCAN_DEVICE_UUID));
	_scanControlCharac->setName(BLE_CHAR_SCAN);
	_scanControlCharac->setDefaultValue(255);
	_scanControlCharac->setWritable(true);
	_scanControlCharac->onWrite([&](const uint8_t& value) -> void {
			MasterBuffer& mb = MasterBuffer::getInstance();
			if(value) {
				LOGi("Init scan result");
				if (!mb.isLocked()) {
					mb.lock();
					_scanResult->clear();
				} else {
					LOGe("buffer already locked!");
				}

				if (!getStack()->isScanning()) {
					getStack()->startScanning();
				}
				_scanMode = true;
			} else {
				LOGi("Return scan result");
				if (mb.isLocked()) {
					_scanResult->print();

					_peripheralCharac->setDataLength(_scanResult->getDataLength());
					_peripheralCharac->notify();
					mb.unlock();
				} else {
					LOGe("buffer not locked!");
				}

				// Only stop scanning if we're not also tracking devices
				if (getStack()->isScanning() && !_trackMode) {
					getStack()->stopScanning();
				}
				_scanMode = false;
			}
		});
}

void IndoorLocalizationService::addPeripheralListCharacteristic() {
//	LOGd("create characteristic to list found peripherals");

	_scanResult = new ScanResult();

	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;
	mb.getBuffer(buffer, maxLength);
	_scanResult->assign(buffer, maxLength);

	_peripheralCharac = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_peripheralCharac);

	_peripheralCharac->setUUID(UUID(getUUID(), LIST_DEVICE_UUID));
	_peripheralCharac->setName("Devices");
	_peripheralCharac->setWritable(false);
	_peripheralCharac->setNotifies(true);

	_peripheralCharac->setValue(buffer);
	_peripheralCharac->setMaxLength(_scanResult->getMaxLength());
	_peripheralCharac->setDataLength(0);
}

void IndoorLocalizationService::addTrackedDeviceListCharacteristic() {

	_trackedDeviceList = new TrackedDeviceList();

	// we don't want to use the master buffer for the tracked device list
	// because it needs to be persistent and not be overwritten by data
	// received over BT

	//	MasterBuffer& mb = MasterBuffer::getInstance();
	//	buffer_ptr_t buffer = NULL;
	//	uint16_t maxLength = 0;
	//	mb.getBuffer(buffer, maxLength);

	// so instead allocate a separate buffer that the tracked device list can use
	uint16_t size = sizeof(tracked_device_list_t);
	buffer_ptr_t buffer = (buffer_ptr_t)calloc(size, sizeof(uint8_t));

	_trackedDeviceList->assign(buffer, size);
	_trackedDeviceList->init();

	_trackedDeviceListCharac = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_trackedDeviceListCharac);

	_trackedDeviceListCharac->setUUID(UUID(getUUID(), TRACKED_DEVICE_LIST_UUID));
	_trackedDeviceListCharac->setName(BLE_CHAR_TRACK);
	_trackedDeviceListCharac->setWritable(false);
	_trackedDeviceListCharac->setNotifies(false);

	_trackedDeviceListCharac->setValue(buffer);
	_trackedDeviceListCharac->setMaxLength(_trackedDeviceList->getMaxLength());
	_trackedDeviceListCharac->setDataLength(0);

	// Load the nearby timeout
	uint16_t counts;
	Storage::getUint16(Settings::getInstance().getConfig().nearbyTimeout, counts, TRACKDEVICE_DEFAULT_TIMEOUT_COUNT);
	_trackedDeviceList->setTimeout(counts);
}

void IndoorLocalizationService::writeTrackedDevices() {
	buffer_ptr_t buffer;
	uint16_t length;
	_trackedDeviceList->getBuffer(buffer, length);
	Storage::setArray(buffer, _storageStruct.trackedDevices.list, length);
}

void IndoorLocalizationService::readTrackedDevices() {
	buffer_ptr_t buffer;
	uint16_t length;
	_trackedDeviceList->getBuffer(buffer, length);
	length = _trackedDeviceList->getMaxLength();

	Storage::getArray(_storageStruct.trackedDevices.list, buffer, (buffer_ptr_t)NULL, length);

	if (!_trackedDeviceList->isEmpty()) {
		LOGi("restored tracked devices (%d):", _trackedDeviceList->getSize());
		_trackedDeviceList->print();

		_trackedDeviceListCharac->setDataLength(_trackedDeviceList->getDataLength());
		_trackedDeviceListCharac->notify();
	}
}

void IndoorLocalizationService::setNearbyTimeout(uint16_t counts) {
	if (_trackedDeviceList != NULL) {
		_trackedDeviceList->setTimeout(counts);
	}
}

uint16_t IndoorLocalizationService::getNearbyTimeout() {
	if (_trackedDeviceList == NULL) {
		return 0;
	}
	return _trackedDeviceList->getTimeout();
}

void IndoorLocalizationService::startTracking() {
	if (!_trackMode) {
		// Set to some value initially
		_trackIsNearby = false;
	}
	_trackMode = true;
	if (!getStack()->isScanning()) {
		LOGi("Start tracking");
		getStack()->startScanning();
	}
}

void IndoorLocalizationService::stopTracking() {
	_trackMode = false;
	if (getStack()->isScanning()) {
		LOGi("Stop tracking");
		getStack()->stopScanning();
	}
}

void IndoorLocalizationService::addTrackedDeviceCharacteristic() {

	buffer_ptr_t buffer = MasterBuffer::getInstance().getBuffer();

	_trackedDeviceCharac = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_trackedDeviceCharac);

	_trackedDeviceCharac->setUUID(UUID(getUUID(), TRACKED_DEVICE_UUID));
	_trackedDeviceCharac->setName("Add tracked device");
	_trackedDeviceCharac->setWritable(true);
	_trackedDeviceCharac->setNotifies(false);
	_trackedDeviceCharac->onWrite([&](const buffer_ptr_t& value) -> void {
		TrackedDevice dev;
		dev.assign(_trackedDeviceCharac->getValue(), _trackedDeviceCharac->getValueLength());

		if (dev.getRSSI() > 0) {
			LOGi("Remove tracked device");
			dev.print();
			_trackedDeviceList->rem(dev.getAddress());
		} else {
			LOGi("Add tracked device");
			dev.print();
			_trackedDeviceList->add(dev.getAddress(), dev.getRSSI());
		}
		_trackedDeviceListCharac->setDataLength(_trackedDeviceList->getDataLength());
		_trackedDeviceListCharac->notify();

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

	_trackedDeviceCharac->setValue(buffer);
	_trackedDeviceCharac->setMaxLength(TRACKDEVICES_SERIALIZED_SIZE);
	_trackedDeviceCharac->setDataLength(0);

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
#ifdef MICRO_VIEW
	// Update rssi at the display
	write("2 %i\r\n", RSSILevel);
#endif
	if (_rssiCharac) {
		*_rssiCharac = RSSILevel;
	}
}

//void IndoorLocalizationService::setRSSILevelHandler(func_t func) {
//	_rssiHandler = func;
//}

#if(SOFTDEVICE_SERIES != 110)
void IndoorLocalizationService::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {
	if (getStack()->isScanning()) {
		if (_scanMode) {
//			ScanResult& result = _peripheralCharac->getValue();
			_scanResult->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
		}
		if (_trackMode && _trackedDeviceList != NULL) {
			_trackedDeviceList->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
		}
	}
}
#endif

