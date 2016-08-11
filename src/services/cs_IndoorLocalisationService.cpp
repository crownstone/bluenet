/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/cs_IndoorLocalisationService.h>

#include <structs/buffer/cs_MasterBuffer.h>
#include <cfg/cs_UuidConfig.h>
#include <processing/cs_CommandHandler.h>
#include <structs/cs_TrackDevices.h>
#include <processing/cs_Tracker.h>

IndoorLocalizationService::IndoorLocalizationService() : EventListener(),
		_rssiCharac(NULL), _scannedDeviceListCharac(NULL),
		_trackedDeviceListCharac(NULL), _trackedDeviceCharac(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(INDOORLOCALISATION_UUID));
	setName(BLE_SERVICE_INDOOR_LOCALIZATION);
	
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)IndoorLocalizationService::staticTick);
}

void IndoorLocalizationService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_INDOOR_LOCALIZATION);

#if CHAR_RSSI==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_RSSI);
	addRssiCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_RSSI);
#endif
#if CHAR_SCAN_DEVICES==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_SCAN_DEVICES);
	addScanControlCharacteristic();
	addScannedDeviceListCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_SCAN_DEVICES);
#endif
#if CHAR_TRACK_DEVICES==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_TRACKED_DEVICE);
	addTrackedDeviceListCharacteristic();
	addTrackedDeviceCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_TRACKED_DEVICE);
#endif

	addCharacteristicsDone();
}

void IndoorLocalizationService::tick() {
//	LOGi("Tack: %d", RTC::now());

#if CHAR_RSSI==1
#ifdef PWM_ON_RSSI
//	//! Map [-90, -40] to [0, 100]
//	int16_t pwm = (_averageRssi + 90) * 10 / 5;
//	if (pwm < 0) {
//		pwm = 0;
//	}
//	if (pwm > 100) {
//		pwm = 100;
//	}
//	PWM::getInstance().setValue(0, pwm);

	if (_averageRssi > -70 && PWM::getInstance().getValue(0) < 1) {
		PWM::getInstance().setValue(0, 255);
	}

	if (_averageRssi < -80 && PWM::getInstance().getValue(0) > 0) {
		PWM::getInstance().setValue(0, 0);
	}
#endif
#endif

	scheduleNextTick();
}

void IndoorLocalizationService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(LOCALIZATION_SERVICE_UPDATE_FREQUENCY), this);
}

void IndoorLocalizationService::addRssiCharacteristic() {
	_rssiCharac = new Characteristic<int8_t>();
	addCharacteristic(_rssiCharac);

	_rssiCharac->setUUID(UUID(getUUID(), RSSI_UUID)); //! there is no BLE_UUID for rssi level(?)
	_rssiCharac->setName(BLE_CHAR_RSSI);
	_rssiCharac->setDefaultValue(1);
	_rssiCharac->setNotifies(true);
#ifdef PWM_ON_RSSI
	_averageRssi = -90; // Start with something..
#endif
}

void IndoorLocalizationService::addScanControlCharacteristic() {
	_scanControlCharac = new Characteristic<uint8_t>();
	addCharacteristic(_scanControlCharac);

	_scanControlCharac->setUUID(UUID(getUUID(), SCAN_CONTROL_UUID));
	_scanControlCharac->setName(BLE_CHAR_SCAN_CONTROL);
	_scanControlCharac->setDefaultValue(255);
	_scanControlCharac->setWritable(true);
	_scanControlCharac->onWrite([&](const uint8_t accessLevel, const uint8_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_SCAN_DEVICES, (buffer_ptr_t)&value, 1);
	});
}

void IndoorLocalizationService::addScannedDeviceListCharacteristic() {

	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;
	mb.getBuffer(buffer, maxLength);

	_scannedDeviceListCharac = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_scannedDeviceListCharac);

	_scannedDeviceListCharac->setUUID(UUID(getUUID(), LIST_DEVICE_UUID));
	_scannedDeviceListCharac->setName(BLE_CHAR_SCAN_LIST);
	_scannedDeviceListCharac->setWritable(false);
	_scannedDeviceListCharac->setNotifies(true);
	_scannedDeviceListCharac->setValue(buffer);
	_scannedDeviceListCharac->setMaxGattValueLength(maxLength);
	_scannedDeviceListCharac->setValueLength(0);
}

void IndoorLocalizationService::addTrackedDeviceListCharacteristic() {

	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;
	mb.getBuffer(buffer, maxLength);

	_trackedDeviceListCharac = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_trackedDeviceListCharac);

	_trackedDeviceListCharac->setUUID(UUID(getUUID(), TRACKED_DEVICE_LIST_UUID));
	_trackedDeviceListCharac->setName(BLE_CHAR_TRACK_LIST);
	_trackedDeviceListCharac->setWritable(false);
	_trackedDeviceListCharac->setNotifies(false);
	_trackedDeviceListCharac->setValue(buffer);
	_trackedDeviceListCharac->setMaxGattValueLength(maxLength);
	_trackedDeviceListCharac->setValueLength(0);
}

void IndoorLocalizationService::addTrackedDeviceCharacteristic() {

	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;
	mb.getBuffer(buffer, maxLength);

	_trackedDeviceCharac = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_trackedDeviceCharac);

	_trackedDeviceCharac->setUUID(UUID(getUUID(), TRACKED_DEVICE_UUID));
	_trackedDeviceCharac->setName(BLE_CHAR_TRACK);
	_trackedDeviceCharac->setWritable(true);
	_trackedDeviceCharac->setNotifies(false);

	_trackedDeviceCharac->setValue(buffer);
	_trackedDeviceCharac->setMaxGattValueLength(maxLength);
	_trackedDeviceCharac->setValueLength(0);

	_trackedDeviceCharac->onWrite([&](const uint8_t accessLevel, const buffer_ptr_t& value) -> void {
		Tracker::getInstance().handleTrackedDeviceCommand(_trackedDeviceCharac->getValue(),
				_trackedDeviceCharac->getValueLength());
	});
}

void IndoorLocalizationService::on_ble_event(ble_evt_t * p_ble_evt) {

	Service::on_ble_event(p_ble_evt);

	switch (p_ble_evt->header.evt_id) {
#if CHAR_RSSI==1
	case BLE_GAP_EVT_CONNECTED: {

#if (NORDIC_SDK_VERSION >= 11) || \
	(SOFTDEVICE_SERIES == 130 && SOFTDEVICE_MAJOR == 1 && SOFTDEVICE_MINOR == 0) || \
	(SOFTDEVICE_SERIES == 110 && SOFTDEVICE_MAJOR == 8)
		sd_ble_gap_rssi_start(p_ble_evt->evt.gap_evt.conn_handle, 0, 0);
#else
		sd_ble_gap_rssi_start(p_ble_evt->evt.gap_evt.conn_handle);
#endif
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
#endif

	default: {
	}
	}
}

void IndoorLocalizationService::onRSSIChanged(int8_t rssi) {

#ifdef RGB_LED
	//! set LED here
	int sine_index = (rssi - 170) * 2;
	if (sine_index < 0) sine_index = 0;
	if (sine_index > 100) sine_index = 100;
	//			__asm("BKPT");
	//			int sine_index = (rssi % 10) *10;
	PWM::getInstance().setValue(0, sin_table[sine_index]);
	PWM::getInstance().setValue(1, sin_table[(sine_index + 33) % 100]);
	PWM::getInstance().setValue(2, sin_table[(sine_index + 66) % 100]);
	//			counter = (counter + 1) % 100;

	//! Add a delay to control the speed of the sine wave
	nrf_delay_us(8000);
#endif

	setRSSILevel(rssi);
}

void IndoorLocalizationService::setRSSILevel(int8_t RSSILevel) {
#ifdef MICRO_VIEW
	//! Update rssi at the display
	write("2 %i\r\n", RSSILevel);
#endif
	if (_rssiCharac) {
		*_rssiCharac = RSSILevel;
#ifdef PWM_ON_RSSI
		//! avg = 0.4*rssi + (1-0.4)*avg
		_averageRssi = (RSSILevel*4 + _averageRssi*6) / 10;
		LOGd("RSSI: %d avg: %d", RSSILevel, _averageRssi);
#endif
	}
}

void IndoorLocalizationService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_SCANNED_DEVICES: {
		_scannedDeviceListCharac->setValue((buffer_ptr_t)p_data);
		_scannedDeviceListCharac->setValueLength(length);
		_scannedDeviceListCharac->updateValue();
		break;
	}
	case EVT_TRACKED_DEVICES: {
		_trackedDeviceListCharac->setValue((buffer_ptr_t)p_data);
		_trackedDeviceListCharac->setValueLength(length);
		_trackedDeviceListCharac->updateValue();
		break;
	}
	}
}
