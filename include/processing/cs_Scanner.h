/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 2, 2015
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Stack.h>
#include <structs/cs_ScanResult.h>

#define SCAN_DURATION  2000
#define SCAN_SEND_WAIT 1000
#define SCAN_BREAK     7000

using namespace BLEpp;

class Scanner {

public:

	Scanner(Nrf51822BluetoothStack* stack);
	virtual ~Scanner();

	void onBleEvent(ble_evt_t * p_ble_evt);

	void manualStartScan();
	void manualStopScan();

	bool isScanning();

	ScanResult* getResults();

	static void staticTick(Scanner* ptr);

	void start();
	void stop();

private:

	enum SCAN_OP_CODE {
		SCAN_START,
		SCAN_STOP,
		SCAN_SEND_RESULT
	};

	SCAN_OP_CODE _opCode;

	bool _scanning;
	bool _running;

	app_timer_id_t _appTimerId;

	Nrf51822BluetoothStack* _stack;

	uint8_t _scanBuffer[sizeof(peripheral_device_list_t)];
	ScanResult* _scanResult;

	void executeScan();
	void sendResults();

	void onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report);
};

