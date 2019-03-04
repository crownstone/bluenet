/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Stack.h>
#include <structs/cs_ScanResult.h>
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

	void onBleEvent(ble_evt_t * p_ble_evt);

	void manualStartScan();
	void manualStopScan();

	bool isScanning();

	ScanResult* getResults();

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

//	uint32_t getInterval() { return (uint32_t)_scanDuration + _scanSendDelay + _scanBreakDuration; }

	void handleEvent(event_t & event);

private:

	enum SCAN_OP_CODE {
		SCAN_START,
		SCAN_STOP,
		SCAN_SEND_RESULT
	};

	SCAN_OP_CODE _opCode;

	bool _scanning;
	bool _running;
	//! scan for ... ms
	uint16_t _scanDuration;
	//! wait ... ms before sending the scan result
	uint16_t _scanSendDelay;
	//! wait ... ms before starting the next scan
	uint16_t _scanBreakDuration;
	//! filter out devices based on mask
	uint8_t _scanFilter;
	//! Filtered out devices are still sent once every N scan intervals
	//! Set to 0 to not send them ever
	uint16_t _filterSendFraction;

	uint16_t _scanCount;

#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _appTimerData;
	app_timer_id_t           _appTimerId;
#else
	uint32_t                 _appTimerId;
#endif

	Stack* _stack;

	uint8_t _scanBuffer[sizeof(peripheral_device_list_t)];
	ScanResult* _scanResult;

	Scanner();

	bool isFiltered(data_t* p_adv_data);

	void executeScan();
	void notifyResults();

	void onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report);
};

