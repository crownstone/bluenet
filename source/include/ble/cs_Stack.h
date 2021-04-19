/**
 * @file
 * Manufacturing data.
 *
 * @authors Crownstone Team, Christopher Mason
 * @copyright Crownstone B.V.
 * @date Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <ble/cs_Service.h>
#include <ble/cs_UUID.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <common/cs_BaseClass.h>
#include <common/cs_Tuple.h>
#include <common/cs_Types.h>
#include <drivers/cs_Timer.h>
#include <string>
#include <third/std/function.h>
#include <util/cs_BleError.h>

/////////////////////////////////////////////////

/** General BLE name service
 *
 * All functionality that is just general BLE functionality is encapsulated in the BLEpp namespace.
 */
class Service;

/** nRF51822 specific implementation of the BLEStack
 *
 * The Stack class is a direct descendant from BLEStack. It is implemented as a singleton, such
 * that it can only be allocated once and it can be reached from everywhere in the code, especially in interrupt
 * handlers. However, please, if an object depends on it, try to make this dependency explicit, and use this
 * stack object as an argument w.r.t. this object. This makes dependencies traceable for the user.
 */
class Stack: public BaseClass<3> {
public:
	static Stack& getInstance() {
		static Stack instance;
		return instance;
	}

	//! Maximum number of services (currently set to 5)
	static const uint8_t MAX_SERVICE_COUNT = 5;
	typedef fixed_tuple<Service*, MAX_SERVICE_COUNT> Services_t;

	enum condition_t { C_STACK_INITIALIZED, C_RADIO_INITIALIZED, C_SERVICES_INITIALIZED };
protected:

	bool                                        _disconnectingInProgress = false;

	// might want to change this to a linked list or something that
	// we can loop over but doesn't allocate more space than needed
	Services_t                                  _services;

	nrf_clock_lf_cfg_t                          _clockSource;
	ble_gap_conn_params_t                       _connectionParams;

	bool                                        _scanning = false;

	uint16_t                                    _connectionHandle = BLE_CONN_HANDLE_INVALID;
	bool                                        _connectionIsOutgoing = false;

	app_timer_t                                 _connectionKeepAliveTimerData;
	app_timer_id_t                              _connectionKeepAliveTimerId = NULL;

	uint8_t _scanBuffer[31]; // Same size as buffer in cs_stack_scan_t.
	ble_data_t _scanBufferStruct = { _scanBuffer, sizeof(_scanBuffer) };


public:

	/** Initialization of the BLE stack
	 *
	 * Performs a series of tasks:
	 *   - disables softdevice if it is currently enabled
	 *   - enables softdevice with own clock and assertion handler
	 *   - enable service changed characteristic for S110
	 *   - disable automatic address recycling for S110

	 *   - set BLE device name
	 *   - set appearance (e.g. used in GUIs to interface with BLE devices)
	 *   - set connection parameters
	 *   - set Tx power level
	 *   - set the callback for BLE events (if we use Source/sd_common/softdevice_handler.c in Nordic's SDK)
	 */
	void init();

	/** Initialization of the radio
	 *
	 */
	void initRadio();

	void initSoftdevice();

	void createCharacteristics();

	/** Start the BLE stack
	 *
	 * Start can only be called once. It starts all services. If one of these services cannot be started, there is
	 * currently no exception handling. The stack does not start the Softdevice. This needs to be done before in
	 * init().
	 */
	void initServices();

	/**
	 * In case a disconnect has been called, we cannot allow another write or we'll get an Fatal Error 8
	 */
	bool isDisconnecting();

	bool isConnected();

	bool isConnectedPeripheral();

	uint16_t getConnectionHandle() {
		return _connectionHandle;
	}

	/** Shutdown the BLE stack
	 *
	 * The function shutdown() is the counterpart of start(). It does stop all services. It does not check if these
	 * services have actually been started.
	 *
	 * It will also stop advertising. The SoftDevice will not be shut down.
	 *
	 * After a shutdown() the function start() can be called again.
	 */
	void shutdown();

	//! Set initial clock source, not applied unless done before radio init.
	void setClockSource(nrf_clock_lf_cfg_t clockSource);

	/** Set and update the preferred minimum connection interval in units of 1.25 ms. */
	void updateMinConnectionInterval(uint16_t connectionInterval_1_25_ms);

	/** Set and update the preferred maximum connection interval in units of 1.25 ms. */
	void updateMaxConnectionInterval(uint16_t connectionInterval_1_25_ms);

	/** Set and update the preferred slave latency count. */
	void updateSlaveLatency(uint16_t slaveLatency);

	/** Set and update the preferred connection supervision timeout in units of 10 ms. */
	void updateConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms);

	/** Add a service to the stack.
	 */
	Stack & addService(Service* svc);

	/** Start scanning for devices
	 *
	 * Only call the following functions with a S120 or S130 device that can play a central role. The following functions
	 * are probably the ones your recognize from implementing BLE functionality on Android or iOS if you are a smartphone
	 * developer.
	 */
	void startScanning();

	/** Stop scanning for devices
	 */
	void stopScanning();

	/** Returns true if currently scanning
	 */
	bool isScanning();

	/** Function that handles BLE events
	 *
	 * A BLE event is generated, these can be connect or disconnect events. It can also be RSSI values that changed, or
	 * an authorization request. Not all event structures are exactly the same over the different SoftDevices, so there
	 * are some defines for minor changes. And of course, e.g. the S110 softdevice cannot listen to advertisements at all,
	 * so BLE_GAP_EVT_ADV_REPORT is entirely disabled.
	 *
	 * TODO: Currently we loop through every service and send e.g. BLE_GATTS_EVT_WRITE only when some handle matches. It
	 * is faster to set up maps from handles to directly the right function.
	 */
	void onBleEvent(const ble_evt_t * p_ble_evt);

	/**
	 * Function that handles BLE events on interrupt level.
	 *
	 * These need to be handled quickly and not use any class state.
	 *
	 * @param[in] p_ble_evt            The BLE event.
	 * @param[in] isInterrupt          Whether this function is actually called on interrupt level.
	 */
	void onBleEventInterrupt(const ble_evt_t * p_ble_evt, bool isInterrupt);

	void secReqTimeoutHandler(void * p_context);
	void setAesEncrypted(bool encrypted);
	void disconnect();

	bool checkCondition(condition_t condition, bool expectation);

protected:


	//! Update connection parameters, can be called when already initialized.
	void updateConnParams();

	void onConnect(const ble_evt_t * p_ble_evt);
	void onDisconnect(const ble_evt_t * p_ble_evt);
	void onGapTimeout(uint8_t src);

	void onConnectionTimeout();

	/** Connection request
	 *
	 * On a connection request send it to all services.
	 */
	void onIncomingConnected(const ble_evt_t * p_ble_evt);
	void onIncomingDisconnected(const ble_evt_t * p_ble_evt);

	void onMemoryRequest(uint16_t connectionHandle);
	void onMemoryRelease(uint16_t connectionHandle);

	void onWrite(const ble_gatts_evt_write_t& writeEvt);

	/** Transmission complete event
	 *
	 * Inform all services that transmission was completed in case they have notifications pending
	 */
	void onTxComplete(const ble_evt_t * p_ble_evt);

	void startConnectionAliveTimer();
	void stopConnectionAliveTimer();
	void resetConnectionAliveTimer();

private:
	/** Constructor of the BLE stack on the NRF5 series.
	 *
	 * The constructor sets up very little! Only enough memory is allocated. Also there are a lot of defaults set.
	 * However, the SoftDevice is not enabled yet, nor any function on the SoftDevice is called. This is done in the
	 * init() function.
	 */
	Stack();
	Stack(Stack const&);
	void operator=(Stack const &);

	/**
	 * The destructor shuts down the stack.
	 *
	 * TODO: The SoftDevice should be disabled as well.
	 */
	~Stack();

};

