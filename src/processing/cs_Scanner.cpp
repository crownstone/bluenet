/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include "processing/cs_Scanner.h"

#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
#endif
#include <storage/cs_State.h>

#include <cfg/cs_DeviceTypes.h>
#include <ble/cs_CrownstoneManufacturer.h>

#include <events/cs_EventDispatcher.h>

#include <cfg/cs_UuidConfig.h>

//#define PRINT_SCANNER_VERBOSE
//#define PRINT_DEBUG

Scanner::Scanner() :
	_opCode(SCAN_START),
	_scanning(false),
	_running(false),
	_scanDuration(SCAN_DURATION),
	_scanBreakDuration(SCAN_BREAK_DURATION),
	_scanCount(0),
#if (NORDIC_SDK_VERSION >= 11)
	_appTimerId(NULL),
#else
	_appTimerId(UINT32_MAX),
#endif
	_stack(NULL)
{
#if (NORDIC_SDK_VERSION >= 11)
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;
#endif

	_scanResult = new ScanResult();

	//! [29.01.16] the scan result needs it's own buffer, not the master buffer,
	//! since it is now decoupled from writing to a characteristic.
	//! if we used the master buffer we would overwrite the scan results
	//! if we write / read from a characteristic that uses the master buffer
	//! during a scan!
	_scanResult->assign(_scanBuffer, sizeof(_scanBuffer));
}

void Scanner::init() {
	State& settings = State::getInstance();
	settings.get(CS_TYPE::CONFIG_SCAN_DURATION, &_scanDuration, PersistenceMode::STRATEGY1);
	settings.get(CS_TYPE::CONFIG_SCAN_BREAK_DURATION, &_scanBreakDuration, PersistenceMode::STRATEGY1);

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Scanner::staticTick);
}

void Scanner::setStack(Stack* stack) {
	_stack = stack;
}

void Scanner::manualStartScan() {
	if (!_stack) {
		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
		return;
	}

//	LOGi(FMT_INIT, "scan result");
	_scanResult->clear();
	_scanning = true;

	if (!_stack->isScanning()) {
#ifdef PRINT_SCANNER_VERBOSE
		LOGi(FMT_START, "Scanner");
#endif
		_stack->startScanning();
	}
}

void Scanner::manualStopScan() {
	if (!_stack) {
		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
		return;
	}

	_scanning = false;

	if (_stack->isScanning()) {
#ifdef PRINT_SCANNER_VERBOSE
		LOGi(FMT_STOP, "Scanner");
#endif
		_stack->stopScanning();
	}
}

bool Scanner::isScanning() {
	if (!_stack) {
		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
		return false;
	}

	return _scanning && _stack->isScanning();
}

ScanResult* Scanner::getResults() {
	return _scanResult;
}

void Scanner::staticTick(Scanner* ptr) {
	ptr->executeScan();
}

void Scanner::start() {
	if (!_running) {
		_running = true;
		_scanCount = 0;
		_opCode = SCAN_START;
		executeScan();
	} else {
		LOGi(FMT_ALREADY, "scanning");
	}
}

void Scanner::delayedStart(uint16_t delay) {
	if (!_running) {
		LOGi("delayed start by %d ms", delay);
		_running = true;
		_scanCount = 0;
		_opCode = SCAN_START;
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(delay), this);
	} else {
		LOGd(FMT_ALREADY, "scanning");
	}
}

void Scanner::delayedStart() {
	delayedStart(_scanBreakDuration);
}

void Scanner::stop() {
	if (_running) {
		_running = false;
		_opCode = SCAN_STOP;
		LOGi("Force STOP");
		manualStopScan();
		//! no need to execute scan on stop is there? we want to stop after all
	//	executeScan();
	//	_running = false;
	} else if (_scanning) {
		manualStopScan();
	} else {
		LOGi(STR_ERR_ALREADY_STOPPED);
	}
}

void Scanner::executeScan() {

	if (!_running) return;

#ifdef PRINT_SCANNER_VERBOSE
	LOGd("Execute Scan");
#endif

	switch(_opCode) {
	case SCAN_START: {

		//! start scanning
		manualStartScan();
		if (_filterSendFraction > 0) {
			_scanCount = (_scanCount+1) % _filterSendFraction;
		}

		//! set timer to trigger in SCAN_DURATION sec, then stop again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanDuration), this);

		_opCode = SCAN_STOP;
		break;
	}
	case SCAN_STOP: {

		//! stop scanning
		manualStopScan();

#ifdef PRINT_DEBUG
		_scanResult->print();
#endif

		//! Wait SCAN_SEND_WAIT ms before sending the results, so that it can listen to the mesh before sending
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanSendDelay), this);

		_opCode = SCAN_SEND_RESULT;
		break;
	}
	case SCAN_SEND_RESULT: {

		notifyResults();

		//! Wait SCAN_BREAK ms, then start scanning again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);

		_opCode = SCAN_START;
		break;
	}
	}

}

void Scanner::notifyResults() {

#ifdef PRINT_SCANNER_VERBOSE
	LOGd("Notify scan results");
#endif

#if BUILD_MESHING == 1
	if (State::getInstance().isSet(CS_TYPE::CONFIG_MESH_ENABLED)) {
		MeshControl::getInstance().sendScanMessage(_scanResult->getList()->list, _scanResult->getSize());
	}
#endif

	buffer_ptr_t buffer;
	uint16_t length;
	_scanResult->getBuffer(buffer, length);

	event_t event(CS_TYPE::EVT_SCANNED_DEVICES, buffer, length);
	EventDispatcher::getInstance().dispatch(event);
}

void Scanner::onBleEvent(ble_evt_t * p_ble_evt) {

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_ADV_REPORT:
		onAdvertisement(&p_ble_evt->evt.gap_evt.params.adv_report);
		break;
	}
}

bool Scanner::isFiltered(data_t* p_adv_data) {
	return false;
}

/** Advertisement handler
 *
 * We do so-called "active" scanning. This means we will get a scan response besides the advertisement packet itself
 * when the device we are scanning for supports this. For now we ignore devices that do not send a scan response.
 * The advantage is that we avoid handling a device twice (once for the advertisement and once for the scan response).
 * This is of course only useful as long as we do not care about the advertisement data.
 */
void Scanner::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {

	if (!isScanning()) return;

	event_t event(CS_TYPE::EVT_DEVICE_SCANNED, p_adv_report, sizeof(ble_gap_evt_adv_report_t));
	EventDispatcher::getInstance().dispatch(event);
	if (p_adv_report->type.scan_response) {
		data_t adv_data;

		adv_data.p_data = p_adv_report->data.p_data;
		adv_data.data_len = p_adv_report->data.len;

		if (!isFiltered(&adv_data)) {
			_scanResult->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
		}
	}
}

void Scanner::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_BLE_EVENT: {
			onBleEvent((ble_evt_t*)event.data);
			return;
		}
		case CS_TYPE::CONFIG_SCAN_DURATION: {
			_scanDuration = *(TYPIFY(CONFIG_SCAN_DURATION)*)event.data;
			break;
		}
		case CS_TYPE::CONFIG_SCAN_BREAK_DURATION: {
			_scanBreakDuration = *(TYPIFY(CONFIG_SCAN_BREAK_DURATION)*)event.data;
			break;
		}
		default:
			// no other types should be handled
			;
	}

}
