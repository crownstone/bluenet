/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_DeviceTypes.h>
#include <cfg/cs_UuidConfig.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_Scanner.h>
#include <storage/cs_State.h>

#define LOGScannerDebug LOGvv
#define LOGScannerVerbose LOGvv

Scanner::Scanner() :
	_opCode(SCAN_START),
	_scanning(false),
	_running(false),
	_scanDuration(g_SCAN_DURATION),
	_scanBreakDuration(g_SCAN_BREAK_DURATION),
	_scanCount(0),
	_appTimerId(NULL),
	_stack(NULL)
{
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;
}

void Scanner::init() {
	State& settings = State::getInstance();
	settings.get(CS_TYPE::CONFIG_SCAN_DURATION, &_scanDuration, sizeof(_scanDuration));
	settings.get(CS_TYPE::CONFIG_SCAN_BREAK_DURATION, &_scanBreakDuration, sizeof(_scanBreakDuration));

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
	_scanning = true;

	if (!_stack->isScanning()) {
		LOGScannerDebug("Start scanner");
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
		LOGScannerDebug("Stop scanner");
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
		// no need to execute scan on stop is there? we want to stop after all
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

	LOGScannerDebug("Execute Scan");
	switch (_opCode) {
		case SCAN_START: {

			// start scanning
			manualStartScan();

			// set timer to trigger in SCAN_DURATION sec, then stop again
			Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanDuration), this);

			_opCode = SCAN_STOP;
			break;
		}
		case SCAN_STOP: {

			// stop scanning
			manualStopScan();

			// Wait SCAN_SEND_WAIT ms before sending the results, so that it can listen to the mesh before sending
			Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);

			_opCode = SCAN_START;
			break;
		}
	}

}

void Scanner::handleEvent(event_t & event) {
	switch (event.type) {
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
			break;
	}
}
