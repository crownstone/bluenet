/**
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>

#include <ble/cs_Service.h>
#include <ble/cs_iBeacon.h>

#include <cfg/cs_Strings.h>
#include <util/cs_BleError.h>
#include <common/cs_Tuple.h>
#include <third/std/function.h>

// TODO: replace std::vector with a fixed, in place array of size capacity.

/* General BLE name service
 *
 * All functionality that is just general BLE functionality is encapsulated in the BLEpp namespace.
 */
namespace BLEpp {

class Service;

/* BLEStack defines a chip-agnostic Bluetooth Low-Energy stack
 *
 * Currently, this class does not leverage much of the general Bluetooth Low-Energy functionality into
 * chip-agnostic code. However, this might be recommendable in the future.
 */
class BLEStack {
public:
	virtual ~BLEStack() {};

	/* Connected?
	 *
	 * @return true if connected, false if not connected
	 */
	virtual bool connected() = 0;

	/* Handle to connection
	 *
	 * @return 16-bit value that unique identifies the connection
	 */
	virtual uint16_t getConnectionHandle() = 0;
};


/* nRF51822 specific implementation of the BLEStack
 *
 * The Nrf51822BluetoothStack class is a direct descendant from BLEStack. It is implemented as a singleton, such
 * that it can only be allocated once and it can be reached from everywhere in the code, especially in interrupt
 * handlers. However, please, if an object depends on it, try to make this dependency explicit, and use this
 * stack object as an argument w.r.t. this object. This makes dependencies traceable for the user.
 */
class Nrf51822BluetoothStack : public BLEStack {
	// Friend for BLE stack event handling
//	friend void SWI2_IRQHandler();

	// Friend for radio notification handling
//	friend void ::SWI1_IRQHandler();

private:
	/* Constructor of the BLE stack on the NRF51822
	 *
	 * The constructor sets up very little! Only enough memory is allocated. Also there are a lot of defaults set. However,
	 * the SoftDevice is not enabled yet, nor any function on the SoftDevice is called. This is done in the init()
	 * function.
	 */
	Nrf51822BluetoothStack();
	Nrf51822BluetoothStack(Nrf51822BluetoothStack const&);
	void operator=(Nrf51822BluetoothStack const &);

	/**
	 * The destructor shuts down the stack.
	 *
	 * TODO: The SoftDevice should be disabled as well.
	 */
	virtual ~Nrf51822BluetoothStack();
public:
	static Nrf51822BluetoothStack& getInstance() {
		static Nrf51822BluetoothStack instance;
		return instance;
	}
	// Format of the callback when a connection has been made
	typedef function<void(uint16_t conn_handle)>   callback_connected_t;
	// Format of the callback after a disconnection event
	typedef function<void(uint16_t conn_handle)>   callback_disconnected_t;
	// Format of the callback of any radio event
	typedef function<void(bool radio_active)>   callback_radio_t;

	// Maximum number of services (currently set to 5)
	static const uint8_t MAX_SERVICE_COUNT = 5;

	//static const uint16_t                  defaultAppearance = BLE_APPEARANCE_UNKNOWN;

	// The default BLE appearance is currently set to a Generic Keyring (576)
	static const uint16_t                  defaultAppearance = BLE_APPEARANCE_GENERIC_KEYRING;
	// The low-frequency clock, currently generated from the high frequency clock
	static const nrf_clock_lfclksrc_t      defaultClockSource = NRF_CLOCK_LFCLKSRC_SYNTH_250_PPM;
	// The default MTU (Maximum Transmission Unit), 672 bytes is the default MTU, but can range from 48 bytes to 64kB.
//	static const uint8_t                   defaultMtu = BLE_L2CAP_MTU_DEF;
	// Minimum connection interval in 1.25 ms (400*1.25=500ms)
	static const uint16_t                  defaultMinConnectionInterval_1_25_ms = 400;
	// Maximum connection interval in 1.25 ms (800*1.25=1 sec)
	static const uint16_t                  defaultMaxConnectionInterval_1_25_ms = 800;
	// Default slave latency
	static const uint16_t                  defaultSlaveLatencyCount = 0;
	// Connection timeout in 10ms (400*10=4 sec)
	static const uint16_t                  defaultConnectionSupervisionTimeout_10_ms = 400;
	// Advertising interval in 0.625 ms (40*0.625=25 ms)
	static const uint16_t                  defaultAdvertisingInterval_0_625_ms = 40;
	// Advertising timeout in seconds (180 sec)
	static const uint16_t                  defaultAdvertisingTimeout_seconds = 180;
	// Default transmission power
	static const int8_t                    defaultTxPowerLevel = -8;

protected:
	std::string                                 _device_name; // 4
	uint16_t                                    _appearance;
	fixed_tuple<Service*, MAX_SERVICE_COUNT>    _services;  // 32

	nrf_clock_lfclksrc_t                        _clock_source; //4
//	uint8_t                                     _mtu_size;
	int8_t                                      _tx_power_level;
	ble_gap_conn_sec_mode_t                     _sec_mode;  //1
	//ble_gap_sec_params_t                        _sec_params; //6
	//ble_gap_adv_params_t                        _adv_params; // 20
	uint16_t                                    _interval;
	uint16_t                                    _timeout;
	ble_gap_conn_params_t                       _gap_conn_params; // 8

	bool                                        _inited;
	bool                                        _started;
	bool                                        _advertising;
	bool                                        _scanning;

	uint16_t                                    _conn_handle;

	callback_connected_t                        _callback_connected;  // 16
	callback_disconnected_t                     _callback_disconnected;  // 16
	callback_radio_t                            _callback_radio;  // 16
	volatile uint8_t                            _radio_notify; // 0 = no notification (radio off), 1 = notify radio on, 2 = no notification (radio on), 3 = notify radio off.

	ble_user_mem_block_t 						_user_mem_block; // used for user memory (long write)
public:
	/* Initialization of the BLE stack
	 *
	 * Performs a series of tasks:
	 *   - disables softdevice if it is currently enabled
	 *   - enables softdevice with own clock and assertion handler
	 *   - enable service changed characteristic for S110
	 *   - disable automatic address recycling for S110
	 *   - enable IRQ (SWI2_IRQn) for the softdevice
	 *   - set BLE device name
	 *   - set appearance (e.g. used in GUIs to interface with BLE devices)
	 *   - set connection parameters
	 *   - set Tx power level
	 *   - set the callback for BLE events (if we use Source/sd_common/softdevice_handler.c in Nordic's SDK)
	 */
	void init();

	/* Start the BLE stack
	 *
	 * Start can only be called once. It starts all services. If one of these services cannot be started, there is
	 * currently no exception handling. The stack does not start the Softdevice. This needs to be done before in
	 * init().
	 */
	void startAdvertisingServices();

	/*
	 *
	 */
	void startTicking();

	/*
	 *
	 */
	void stopTicking();

	/* Shutdown the BLE stack
	 *
	 * The function shutdown() is the counterpart of start(). It does stop all services. It does not check if these
	 * services have actually been started.
	 *
	 * It will also stop advertising. The SoftDevice will not be shut down.
	 *
	 * After a shutdown() the function start() can be called again.
	 */
	void shutdown();

	void setAppearance(uint16_t appearance) {
		if (_inited) BLE_THROW(MSG_BLE_STACK_INITIALIZED);
		_appearance = appearance;
	}

	void setDeviceName(const std::string& deviceName) {
		if (_inited) BLE_THROW(MSG_BLE_STACK_INITIALIZED);
		_device_name = deviceName;
	}

	void setClockSource(nrf_clock_lfclksrc_t clockSource) {
		if (_inited) BLE_THROW(MSG_BLE_STACK_INITIALIZED);
		_clock_source = clockSource;
	}

	void setAdvertisingInterval(uint16_t advertisingInterval){
		// TODO: stop advertising?
		_interval = advertisingInterval;
	}

	void setAdvertisingTimeoutSeconds(uint16_t advertisingTimeoutSeconds) {
		// TODO: stop advertising?
		_timeout = advertisingTimeoutSeconds;
	}

	void updateAppearance(uint16_t appearance);

	/* Update device name
	 * @deviceName limited string for device name
	 *
	 * We want to change the device name halfway. This can be done through a characteristic, which is easy during
	 * development (you can separate otherwise similar devices). It is probably not functionality you want to have for
	 * the end-user.
	 */
	void updateDeviceName(const std::string& deviceName);
	std::string & getDeviceName() { return _device_name; }

	/** Set radio transmit power in dBm (accepted values are -40, -20, -16, -12, -8, -4, 0, and 4 dBm). */
	void setTxPowerLevel(int8_t powerLevel);

	int8_t getTxPowerLevel() {
		return _tx_power_level;
	}

	/** Set the minimum connection interval in units of 1.25 ms. */
	void setMinConnectionInterval(uint16_t connectionInterval_1_25_ms) {
		if (_gap_conn_params.min_conn_interval != connectionInterval_1_25_ms) {
			_gap_conn_params.min_conn_interval = connectionInterval_1_25_ms;
			if (_inited) setConnParams();
		}
	}

	/** Set the maximum connection interval in units of 1.25 ms. */
	void setMaxConnectionInterval(uint16_t connectionInterval_1_25_ms) {
		if (_gap_conn_params.max_conn_interval != connectionInterval_1_25_ms) {
			_gap_conn_params.max_conn_interval = connectionInterval_1_25_ms;
			if (_inited) setConnParams();
		}
	}

	/** Set the slave latency count. */
	void setSlaveLatency(uint16_t slaveLatency) {
		if ( _gap_conn_params.slave_latency != slaveLatency ) {
			_gap_conn_params.slave_latency = slaveLatency;
			if (_inited) setConnParams();
		}
	}

	/** Set the connection supervision timeout in units of 10 ms. */
	void setConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms) {
		if (_gap_conn_params.conn_sup_timeout != conSupTimeout_10_ms) {
			_gap_conn_params.conn_sup_timeout = conSupTimeout_10_ms;
			if (_inited) setConnParams();
		}
	}

	void onConnect(const callback_connected_t& callback) {
		//if (callback && _callback_connected) BLE_THROW("Connected callback already registered.");

		_callback_connected = callback;
		if (_inited) setConnParams();
	}
	void onDisconnect(const callback_disconnected_t& callback) {
		//if (callback && _callback_disconnected) BLE_THROW("Disconnected callback already registered.");

		_callback_disconnected = callback;
	}

	Service& getService(std::string name);
	void addService(Service* svc);

	/* Start advertising as an iBeacon
	 *
	 * @beacon the object defining the parameters for the
	 *   advertisement package. See <IBeacon> for an explanation
	 *   of the parameters and values
	 *
	 * Initiates the advertisement package and fills the manufacturing
	 * data array with the values of the <IBeacon> object, then starts
	 * advertising as an iBeacon.
	 *
	 * **Note**: An iBeacon requires that the company identifier is
	 *   set to the Apple Company ID, otherwise it's not an iBeacon.
	 */
	void startIBeacon(IBeacon* beacon);

	/* Start sending advertisement packets.
	 * This can not be called while scanning, start scanning while advertising is possible though.
	 */
	void startAdvertising();

	void stopAdvertising();

	bool isAdvertising();

	/* Start scanning for devices
	 *
	 * Only call the following functions with a S120 or S130 device that can play a central role. The following functions
	 * are probably the ones your recognize from implementing BLE functionality on Android or iOS if you are a smartphone
	 * developer.
	 */
	void startScanning();

	/* Stop scanning for devices
	 */
	void stopScanning();

	/* Returns true if currently scanning
	 */
	bool isScanning();

	/* Set radion notification interrupts
	 *
	 * Function that sets up radio notification interrupts. It sets the IRQ priority, enables it, and sets some
	 * configuration values related to distance.
	 *
	 * Currently not used.
	 */
//	Nrf51822BluetoothStack& onRadioNotificationInterrupt(uint32_t distanceUs, callback_radio_t callback);

	virtual bool connected() {
		return _conn_handle != BLE_CONN_HANDLE_INVALID;
	}
	virtual uint16_t getConnectionHandle() {  // TODO are multiple connections supported?
		return _conn_handle;
	}

	/* Not time-critical functionality can be done in the tick
	 *
	 * Every module on the system gets a tick in which it regularly gets some attention. Of course, everything that is
	 * important should be done within interrupt handlers.
	 *
	 * This function goes through the buffer and calls on_ble_evt for every BLE message in the buffer, till the buffer is
	 * empty. It then returns.
	 */
//	void tick();

	/* Function that handles BLE events
	 *
	 * A BLE event is generated, these can be connect or disconnect events. It can also be RSSI values that changed, or
	 * an authorization request. Not all event structures are exactly the same over the different SoftDevices, so there
	 * are some defines for minor changes. And of course, e.g. the S110 softdevice cannot listen to advertisements at all,
	 * so BLE_GAP_EVT_ADV_REPORT is entirely disabled.
	 *
	 * TODO: Currently we loop through every service and send e.g. BLE_GATTS_EVT_WRITE only when some handle matches. It
	 * is faster to set up maps from handles to directly the right function.
	 */
	void on_ble_evt(ble_evt_t * p_ble_evt);

protected:

	void setTxPowerLevel();
	void setConnParams();


	/* Connection request
	 *
	 * On a connection request send it to all services.
	 */
	void on_connected(ble_evt_t * p_ble_evt);
	void on_disconnected(ble_evt_t * p_ble_evt);
	void on_advertisement(ble_evt_t * p_ble_evt);

	/* Transmission complete event
	 *
	 * Inform all services that transmission was completed in case they have notifications pending
	 */
	void onTxComplete(ble_evt_t * p_ble_evt);

};

}
