/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Stack.h>
#include <events/cs_EventListener.h>

/** Scanner scans for BLE devices.
 */
class Scanner : EventListener {

public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Scanner& getInstance() {
		static Scanner instance;
		return instance;
	}

	void setStack(Stack* stack);

	void onBleEvent(ble_evt_t* p_ble_evt);

	void manualStartScan();
	void manualStopScan();

	bool isScanning();

	static void staticTick(Scanner* ptr);

	void init();
	//! start immediately
	void start();
	//! delay start by delay ms
	void delayedStart(uint16_t delay);
	//! delay start by _scanBreakDuration ms
	void delayedStart();
	//! stop scan immediately (no results will be sent)
	void stop();

	void handleEvent(event_t& event);

private:
	enum SCAN_OP_CODE {
		SCAN_START,
		SCAN_STOP,
	};

	SCAN_OP_CODE _opCode;

	bool _scanning;
	bool _running;
	//! scan for ... ms
	TYPIFY(CONFIG_SCAN_DURATION) _scanDuration;
	//! wait ... ms before starting the next scan
	TYPIFY(CONFIG_SCAN_BREAK_DURATION) _scanBreakDuration;

	uint16_t _scanCount;

	app_timer_t _appTimerData;
	app_timer_id_t _appTimerId;

	Stack* _stack;

	Scanner();

	void executeScan();

	void onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report);
};
