/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 20, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/cs_Tracker.h>

#include <storage/cs_State.h>
#include <drivers/cs_Serial.h>

//#define PRINT_TRACKER_VERBOSE
//#define PRINT_DEBUG

Tracker::Tracker() : EventListener(),
		_timeoutCounts(TRACKDEVICE_DEFAULT_TIMEOUT_COUNT), _tracking(false), _trackIsNearby(false),
		_trackedDeviceList(NULL), _stack(NULL),
#if (NORDIC_SDK_VERSION >= 11)
		_appTimerId(NULL)
#else
		_appTimerId(UINT32_MAX)
#endif
{
#if (NORDIC_SDK_VERSION >= 11)
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;
#endif

	_trackedDeviceList = new TrackedDeviceList();

	//! we don't want to use the master buffer for the tracked device list
	//! because it needs to be persistent and not be overwritten by data
	//! received over BT
	_trackedDeviceList->assign(_trackBuffer, sizeof(_trackBuffer));

}

void Tracker::init() {
	readTrackedDevices();

	State::getInstance().get(CONFIG_NEARBY_TIMEOUT, &_timeoutCounts);
	_trackedDeviceList->setTimeout(_timeoutCounts);


	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) Tracker::staticTick);
}

void Tracker::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {
//	if (isTracking()) {
		if (isTracking() && _trackedDeviceList != NULL) {
			_trackedDeviceList->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
		}
//	}
}

void Tracker::setStack(Stack* stack) {
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

		//! Only send event on change of nearby state
		if (deviceNearby && !_trackIsNearby) {
			LOGd("send event: is nearby");
			EventDispatcher::getInstance().dispatch(EVT_TRACKED_DEVICE_IS_NEARBY);
		} else if (!deviceNearby && _trackIsNearby) {
			LOGd("send event: is not nearby");
			EventDispatcher::getInstance().dispatch(EVT_TRACKED_DEVICE_NOT_NEARBY);
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
	State::getInstance().set(STATE_TRACKED_DEVICES, buffer, _trackedDeviceList->getMaxLength());
}

void Tracker::readTrackedDevices() {
	buffer_ptr_t buffer;
	uint16_t length;
	_trackedDeviceList->getBuffer(buffer, length);

	State::getInstance().get(STATE_TRACKED_DEVICES, buffer, _trackedDeviceList->getMaxLength());
	_trackedDeviceList->resetTimeoutCounters();

	if (!_trackedDeviceList->isEmpty()) {
#ifdef PRINT_DEBUG
		LOGi("restored tracked devices (%d):", _trackedDeviceList->getSize());
		_trackedDeviceList->print();
#endif
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
//	if (!_stack) {
//		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
//		return;
//	}

	if (!_tracking) {
		//! Set to some value initially
		_trackIsNearby = false;
		//! Reset the counters
		_trackedDeviceList->setTimeout(_timeoutCounts);
		_tracking = true;

//		if (!_stack->isScanning()) {
//			LOGi(FMT_START, "tracking");
//			_stack->startScanning();
//		}

		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TRACKER_UPATE_FREQUENCY), this);
	} else {
		LOGi(FMT_ALREADY, "tracking");
	}
}

void Tracker::stopTracking() {
//	if (!_stack) {
//		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
//		return;
//	}

	_tracking = false;
//	if (_stack->isScanning()) {
//		LOGi(FMT_STOP, "tracking");
//		_stack->stopScanning();
//	} else {
//		LOGi(STR_ERR_ALREADY_STOPPED);
//	}
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
//	if (!_stack) {
//		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
//		return false;
//	}

//	return _tracking && _stack->isScanning();
	return _tracking;
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

#ifdef PRINT_TRACKER_VERBOSE
		LOGi("Remove tracked device");
#endif

#ifdef PRINT_DEBUG
		dev.print();
#endif

		Tracker::getInstance().removeTrackedDevice(dev);
	} else {

#ifdef PRINT_TRACKER_VERBOSE
		LOGi("Add tracked device");
#endif

#ifdef PRINT_DEBUG
		dev.print();
#endif

		Tracker::getInstance().addTrackedDevice(dev);
	}
	TrackedDeviceList* trackedDeviceList = Tracker::getInstance().getTrackedDevices();

#ifdef PRINT_DEBUG
	LOGi("currently tracking devices:");
	trackedDeviceList->print();
#endif

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
