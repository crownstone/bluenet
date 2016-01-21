/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 2, 2015
 * License: LGPLv3+
 */
#include "processing/cs_Scanner.h"

#include <protocol/cs_MeshControl.h>
#include <cfg/cs_Settings.h>

Scanner::Scanner(Nrf51822BluetoothStack* stack) :
_opCode(SCAN_START),
_scanning(false),
_running(false),
_scanDuration(SCAN_DURATION),
_scanSendDelay(SCAN_SEND_DELAY),
_scanBreakDuration(SCAN_BREAK_DURATION),
_stack(stack) {

	_scanResult = new ScanResult();

//	MasterBuffer& mb = MasterBuffer::getInstance();
//	buffer_ptr_t buffer = NULL;
//	uint16_t maxLength = 0;
//	mb.getBuffer(buffer, maxLength);

	_scanResult->assign(_scanBuffer, sizeof(_scanBuffer));

	ps_configuration_t cfg = Settings::getInstance().getConfig();
	Storage::getUint16(cfg.scanDuration, _scanDuration, SCAN_DURATION);
	Storage::getUint16(cfg.scanSendDelay, _scanSendDelay, SCAN_SEND_DELAY);
	Storage::getUint16(cfg.scanBreakDuration, _scanBreakDuration, SCAN_BREAK_DURATION);

	EventDispatcher::getInstance().addListener(this);

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Scanner::staticTick);
}

Scanner::~Scanner() {
	// TODO Auto-generated destructor stub
}

void Scanner::manualStartScan() {

	LOGi("Init scan result");
	_scanResult->clear();
	_scanning = true;

	if (!_stack->isScanning()) {
		LOGi("start scanning...");
		_stack->startScanning();
	}
}

void Scanner::manualStopScan() {

	_scanning = false;

	if (_stack->isScanning()) {
		LOGi("stop scanning...");
		_stack->stopScanning();
	}
}

bool Scanner::isScanning() {
	return _scanning && _stack->isScanning();
}

ScanResult* Scanner::getResults() {
	return _scanResult;
}

void Scanner::staticTick(Scanner* ptr) {
	ptr->executeScan();
}

void Scanner::start() {
	_running = true;
	_opCode = SCAN_START;
	//executeScan();
	Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);
}

void Scanner::stop() {
	_opCode = SCAN_STOP;
	executeScan();
	_running = false;
}

void Scanner::executeScan() {

	if (!_running) return;

	LOGi("executeScan");
	switch(_opCode) {
	case SCAN_START: {
		LOGi("START");

		// start scanning
		manualStartScan();

		// set timer to trigger in SCAN_DURATION sec, then stop again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanDuration), this);

		_opCode = SCAN_STOP;
		break;
	}
	case SCAN_STOP: {
		LOGi("STOP");

		// stop scanning
		manualStopScan();

		_scanResult->print();

		// Wait SCAN_SEND_WAIT ms before sending the results, so that it can listen to the mesh before sending
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanSendDelay), this);

		_opCode = SCAN_SEND_RESULT;
		break;
	}
	case SCAN_SEND_RESULT: {
		sendResults();

		// Wait SCAN_BREAK ms, then start scanning again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);

		_opCode = SCAN_START;
		break;
	}
	}

}

void Scanner::sendResults() {
#if CHAR_MESHING==1
	MeshControl::getInstance().sendScanMessage(_scanResult->getList()->list, _scanResult->getSize());
#endif
}

void Scanner::onBleEvent(ble_evt_t * p_ble_evt) {

	switch (p_ble_evt->header.evt_id) {
		case BLE_GAP_EVT_ADV_REPORT:
		onAdvertisement(&p_ble_evt->evt.gap_evt.params.adv_report);
		break;
	}
}

void Scanner::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {

	if (isScanning()) {
		_scanResult->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
	}
//	else {
//		LOGw("why are we getting advertisements if scan is not running?");
//	}
}

void Scanner::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case CONFIG_SCAN_DURATION:
		_scanDuration = *(uint32_t*)p_data;
		break;
	case CONFIG_SCAN_SEND_DELAY:
		_scanSendDelay = *(uint32_t*)p_data;
		break;
	case CONFIG_SCAN_BREAK_DURATION:
		_scanBreakDuration = *(uint32_t*)p_data;
		break;
	}

}
