/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 20, 2016
 * License: LGPLv3+
 */

#include <processing/cs_Tracker.h>

#include <storage/cs_Settings.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_PWM.h>
#include <storage/cs_State.h>

Tracker::Tracker() : EventListener(),
		_timeoutCounts(TRACKDEVICE_DEFAULT_TIMEOUT_COUNT), _tracking(false), _trackIsNearby(false),
		_trackedDeviceList(NULL), _stack(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	_trackedDeviceList = new TrackedDeviceList();

	//! we don't want to use the master buffer for the tracked device list
	//! because it needs to be persistent and not be overwritten by data
	//! received over BT
	_trackedDeviceList->assign(_trackBuffer, sizeof(_trackBuffer));

	readTrackedDevices();

	Settings::getInstance().get(CONFIG_NEARBY_TIMEOUT, &_timeoutCounts);
	_trackedDeviceList->setTimeout(_timeoutCounts);

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) Tracker::staticTick);
}
;

void Tracker::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {
	if (isTracking()) {
		if (_tracking && _trackedDeviceList != NULL) {
			_trackedDeviceList->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
		}
	}
}

void Tracker::setStack(Nrf51822BluetoothStack* stack) {
	_stack = stack;
}

void Tracker::tick() {

	if (_tracking) {

		//! This function checks the counter for each device
		//! If no device is nearby, turn off the light
		bool deviceNearby = false;
		if (_trackedDeviceList != NULL) {
			deviceNearby = (_trackedDeviceList->isNearby() == TDL_IS_NEARBY);
		}

		//! Change PWM only on change of nearby state
		if (deviceNearby && !_trackIsNearby) {
			PWM::getInstance().setValue(0, (uint8_t) -1);
		} else if (!deviceNearby && _trackIsNearby) {
			PWM::getInstance().setValue(0, 0);
		}
		_trackIsNearby = deviceNearby;

		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TRACKER_UPATE_FREQUENCY), this);
	}

}

void Tracker::staticTick(Tracker* ptr) {
	ptr->tick();
}

void Tracker::writeTrackedDevices() {
	buffer_ptr_t buffer;
	uint16_t length;
	_trackedDeviceList->getBuffer(buffer, length);
	State::getInstance().set(STATE_TRACKED_DEVICES, buffer, length);
}

void Tracker::readTrackedDevices() {
	buffer_ptr_t buffer;
	uint16_t length;
	_trackedDeviceList->getBuffer(buffer, length);

	State::getInstance().get(STATE_TRACKED_DEVICES, buffer, length);

	if (!_trackedDeviceList->isEmpty()) {
		LOGi("restored tracked devices (%d):", _trackedDeviceList->getSize());
		_trackedDeviceList->print();

		// inform listeners (like the indoor localisation service, i.e. trackedDeviceListCharacteristic)
		EventDispatcher::getInstance().dispatch(EVT_TRACKED_DEVICES, buffer, length);
	}
}

void Tracker::setNearbyTimeout(uint16_t counts) {
	if (_trackedDeviceList != NULL) {
		_trackedDeviceList->setTimeout(counts);
	}
}

uint16_t Tracker::getNearbyTimeout() {
	if (_trackedDeviceList == NULL) {
		return 0;
	}
	return _trackedDeviceList->getTimeout();
}

void Tracker::startTracking() {
	if (!_stack) {
		LOGe("forgot to assign stack!");
		return;
	}

	if (!_tracking) {
		//! Set to some value initially
		_trackIsNearby = false;
		_tracking = true;

		if (!_stack->isScanning()) {
			LOGi("Start tracking");
			_stack->startScanning();
		}

		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TRACKER_UPATE_FREQUENCY), this);
	} else {
		LOGi("already tracking");
	}
}

void Tracker::stopTracking() {
	if (!_stack) {
		LOGe("forgot to assign stack!");
		return;
	}

	_tracking = false;
	if (_stack->isScanning()) {
		LOGi("Stop tracking");
		_stack->stopScanning();
	}
}

void Tracker::removeTrackedDevice(TrackedDevice device) {
	_trackedDeviceList->rem(device.getAddress());
	writeTrackedDevices();
	publishTrackedDevices();
}

void Tracker::addTrackedDevice(TrackedDevice device) {
	_trackedDeviceList->add(device.getAddress(), device.getRSSI());
	writeTrackedDevices();
	publishTrackedDevices();
}

TrackedDeviceList* Tracker::getTrackedDevices() {
	return _trackedDeviceList;
}

void Tracker::onBleEvent(ble_evt_t * p_ble_evt) {

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_ADV_REPORT:
		onAdvertisement(&p_ble_evt->evt.gap_evt.params.adv_report);
		break;
	}
}

bool Tracker::isTracking() {
	if (!_stack) {
		LOGe("forgot to assign stack!");
		return false;
	}

	return _tracking && _stack->isScanning();
}

void Tracker::publishTrackedDevices() {
	buffer_ptr_t buffer;
	uint16_t length;
	_trackedDeviceList->getBuffer(buffer, length);

	// inform listeners (like the indoor localisation service, i.e. trackedDeviceListCharacteristic)
	EventDispatcher::getInstance().dispatch(EVT_TRACKED_DEVICES, buffer, length);
}

void Tracker::handleTrackedDeviceCommand(buffer_ptr_t buffer, uint16_t size) {

	TrackedDevice dev;
	//TODO: should we check the result of assign() ?
	dev.assign(buffer, size);

	if (dev.getRSSI() > 0) {
		LOGi("Remove tracked device");
		dev.print();
		Tracker::getInstance().removeTrackedDevice(dev);
	} else {
		LOGi("Add tracked device");
		dev.print();
		Tracker::getInstance().addTrackedDevice(dev);
	}
	TrackedDeviceList* trackedDeviceList = Tracker::getInstance().getTrackedDevices();

	LOGi("currently tracking devices:");
	trackedDeviceList->print();

	if (trackedDeviceList->getSize() > 0) {
		Tracker::getInstance().startTracking();
	} else {
		Tracker::getInstance().stopTracking();
	}

}

void Tracker::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_BLE_EVENT: {
		onBleEvent((ble_evt_t*)p_data);
		break;
	}
	}
}
