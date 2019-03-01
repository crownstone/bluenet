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

#include <ble/cs_CrownstoneManufacturer.h>
#include <ble/cs_Nordic.h>
#include <ble/cs_Service.h>
#include <ble/cs_ServiceData.h>
#include <ble/cs_iBeacon.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <common/cs_BaseClass.h>
#include <common/cs_Tuple.h>
#include <common/cs_Types.h>
#include <string>
#include <third/std/function.h>
#include <util/cs_BleError.h>

/////////////////////////////////////////////////

// @TODO: replace std::vector with a fixed, in place array of size capacity.

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
class Stack: public BaseClass<4> {
public:
	static Stack& getInstance() {
		static Stack instance;
		return instance;
	}

	//! Format of the callback when a connection has been made
	typedef function<void(uint16_t conn_handle)>   callback_connected_t;
	//! Format of the callback after a disconnection event
	typedef function<void(uint16_t conn_handle)>   callback_disconnected_t;
	//! Format of the callback of any radio event
	typedef function<void(bool radio_active)>   callback_radio_t;

	//! Maximum number of services (currently set to 5)
	static const uint8_t MAX_SERVICE_COUNT = 5;

	typedef fixed_tuple<Service*, MAX_SERVICE_COUNT> Services_t;  

	//! The default BLE appearance, for unknown reason set to generic tag. 
	// See: https://devzone.nordicsemi.com/question/2973/an36-ble_appearance/
	static const uint16_t                  defaultAppearance = BLE_APPEARANCE_GENERIC_TAG;
	//! The low-frequency clock, currently generated from the high frequency clock
	static const nrf_clock_lf_cfg_t        defaultClockSource;
	//! Advertising interval in 0.625 ms.
	static const uint16_t                  defaultAdvertisingInterval_0_625_ms = ADVERTISEMENT_INTERVAL;
	//! Time after which advertising stops.
	static const uint16_t                  defaultAdvertisingTimeout_seconds = ADVERTISING_TIMEOUT;

protected:
	enum condition_t { C_STACK_INITIALIZED, C_RADIO_INITIALIZED, C_SERVICES_INITIALIZED, C_ADVERTISING };

	std::string                                 _device_name; //! 4
	uint16_t                                    _appearance;
	bool                                        _disconnectingInProgress = false;

	// might want to change this to a linked list or something that
	// we can loop over but doesn't allocate more space than needed
	Services_t                                  _services;  

	nrf_clock_lf_cfg_t                          _clock_source;
	int8_t                                      _tx_power_level;
	ble_gap_conn_sec_mode_t                     _sec_mode;  //1
	uint16_t                                    _interval;
	uint16_t                                    _timeout;
	ble_gap_conn_params_t                       _gap_conn_params; //! 8

	//bool                                        _initializedStack;
	//bool                                        _initializedServices;
	//bool                                        _initializedRadio;
	bool                                        _advertising;
	bool                                        _advertisingConfigured;
	bool                                        _scanning;

	uint16_t                                    _conn_handle;

	callback_connected_t                        _callback_connected;  //! 16
	callback_disconnected_t                     _callback_disconnected;  //! 16
	callback_radio_t                            _callback_radio;  //! 16
	//! 0 = no notification (radio off), 1 = notify radio on, 2 = no notification (radio on), 3 = notify radio off.
	volatile uint8_t                            _radio_notify; 

	ble_user_mem_block_t 						_user_mem_block; //! used for user memory (long write)

	uint8_t                            			_passkey[BLE_GAP_PASSKEY_LEN];
	//dm_application_instance_t                   _dm_app_handle;
	bool                                        _dm_initialized;

	app_timer_t                                 _lowPowerTimeoutData;
	app_timer_id_t                              _lowPowerTimeoutId;
	app_timer_t                                 _secReqTimerData;
	app_timer_id_t                              _secReqTimerId;
	app_timer_t                                 _connectionKeepAliveTimerData;
	app_timer_id_t                              _connectionKeepAliveTimerId;

	uint8_t                                     _adv_handle; // Advertisement handle is set by first call to: sd_ble_gap_adv_set_configure()
	ble_advdata_t                               _config_advdata;
	ble_advdata_t                               _config_scanrsp;
	
	ble_gap_adv_data_t                          _adv_data;
	//ble_data_t                                  _data_advdata;
	//ble_data_t                                  _data_scanrsp;

	ble_gap_adv_params_t                        _adv_params;
	uint8_t                                     _conn_cfg_tag;

	uint8_t										_advInterleaveCounter;

	ble_advdata_manuf_data_t                    _manufac;
	// todo: make part of Crownstonefacturer (see iBeacon)
	uint8_t*                                    _adv_manuf_data;

	ble_advdata_manuf_data_t 					_manufac_apple;
	ble_advdata_service_data_t                  _service_data;

	ServiceData*                                _serviceData;

	ble_gap_addr_t                              _connectable_address;
	ble_gap_addr_t                              _nonconnectable_address;
	
	struct stack_state {
		bool advertising;
	} _stack_state;

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
	
	//! Temporarily halt the stack
	void halt();

	//! Continue running the stack
	void resume();

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

	//! Set initial appearance, not applied unless done before radio init.
	void setAppearance(uint16_t appearance);

	//! Set initial device name, not applied unless done before radio init.
	void setDeviceName(const std::string& deviceName);

	//! Set initial clock source, not applied unless done before radio init.
	void setClockSource(nrf_clock_lf_cfg_t clockSource);

	//! Set initial advertising interval in 0.625 ms units, not applied unless done before radio init.
	void setAdvertisingInterval(uint16_t advertisingInterval);

	void setAdvertisingTimeoutSeconds(uint16_t advertisingTimeoutSeconds) {
		//! TODO: stop advertising?
		_timeout = advertisingTimeoutSeconds;
	}

	void updateAppearance(uint16_t appearance);

	/** Update device name
	 * @deviceName limited string for device name
	 *
	 * We want to change the device name halfway. This can be done through a characteristic, which is easy during
	 * development (you can separate otherwise similar devices). It is probably not functionality you want to have for
	 * the end-user.
	 */
	void updateDeviceName(const std::string& deviceName);

	std::string & getDeviceName() { return _device_name; }

	//! Update the advertising interval in 0.625ms units. Set apply to true, if change should be immediately applied.
	void updateAdvertisingInterval(uint16_t advertisingInterval, bool apply);

	void setPasskey(uint8_t* passkey);

	/** Set radio transmit power in dBm (accepted values are -40, -20, -16, -12, -8, -4, 0, and 4 dBm). */
	void setTxPowerLevel(int8_t powerLevel);

	int8_t getTxPowerLevel() {
		return _tx_power_level;
	}

	/** Set the minimum connection interval in units of 1.25 ms. */
	void setMinConnectionInterval(uint16_t connectionInterval_1_25_ms);

	/** Set the maximum connection interval in units of 1.25 ms. */
	void setMaxConnectionInterval(uint16_t connectionInterval_1_25_ms);

	/** Set the slave latency count. */
	void setSlaveLatency(uint16_t slaveLatency);

	/** Set the connection supervision timeout in units of 10 ms. */
	void setConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms);

	void onConnect(const callback_connected_t& callback);

	void onDisconnect(const callback_disconnected_t& callback);

	/** Get a service by name
	 */
	Service& getService(std::string name);

	/** Add a service to the stack.
	 */
	Stack & addService(Service* svc);

	/** Start advertising as an iBeacon
	 *
	 * @beacon the object defining the parameters for the
	 *   advertisement package. See <IBeacon> for an explanation
	 *   of the parameters and values
	 *
	 * Initiates the advertisement package and fills the manufacturing
	 * data array with the values of the <IBeacon> object, then starts
	 * advertising as an iBeacon.
	 *
	 * ** Note** : An iBeacon requires that the company identifier is
	 *   set to the Apple Company ID, otherwise it's not an iBeacon.
	 */
	void configureIBeacon(IBeacon* beacon, uint8_t deviceType);

	/** Start sending advertisement packets.
	 * This can not be called while scanning, start scanning while advertising is possible though.
	 */
	void configureBleDevice(uint8_t deviceType);

	void stopAdvertising();

	bool checkCondition(condition_t condition, bool expectation);

	void setAdvertisementData();

	void restartAdvertising();
	void startAdvertising();

	void setConnectable();

	void setNonConnectable();

	/**
	 * Updates the advertisement parameters while advertising. I.e. it stops advertising, configures the parameters
	 * and starts scanning again.
	 *
	 * if toggle is set to true, every time the updateAdvertisementParameters function is called, the advertisement
	 * toggles between connectable and non-connectable
	 * if toggle is set to false, every advertisement will be set to connectable
	 */
	void updateAdvertisement(bool toggle = true);

	void configureScanResponse(uint8_t deviceType);
	void configureBleDeviceAdvData();
	void configureIBeaconAdvData(IBeacon* beacon);

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

	/** Set radion notification interrupts
	 *
	 * Function that sets up radio notification interrupts. It sets the IRQ priority, enables it, and sets some
	 * configuration values related to distance.
	 *
	 * Currently not used.
	 */
	Stack& onRadioNotificationInterrupt(uint32_t distanceUs, callback_radio_t callback);

	bool connected() {
		return _conn_handle != BLE_CONN_HANDLE_INVALID;
	}
	uint16_t getConnectionHandle() {  //! TODO are multiple connections supported?
		return _conn_handle;
	}

	/** Not time-critical functionality can be done in the tick
	 *
	 * Every module on the system gets a tick in which it regularly gets some attention. Of course, everything that is
	 * important should be done within interrupt handlers.
	 *
	 * This function goes through the buffer and calls on_ble_evt for every BLE message in the buffer, till the buffer is
	 * empty. It then returns.
	 */
//	void tick();

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
	//void on_ble_evt(ble_evt_t * p_ble_evt, void * p_event);
	void on_ble_evt(const ble_evt_t * p_ble_evt);

	void secReqTimeoutHandler(void * p_context);

	void setServiceData(ServiceData* serviceData) {
		_serviceData = serviceData;
	}

	void setAesEncrypted(bool encrypted);
	void disconnect();
	void changeToLowTxPowerMode();
	void changeToNormalTxPowerMode();


protected:

	//! Update TX power, can be called when already initialized.
	void updateTxPowerLevel();

	//! Update connection parameters, can be called when already initialized.
	void updateConnParams();

	void updatePasskey();

	/** Connection request
	 *
	 * On a connection request send it to all services.
	 */
	void on_connected(const ble_evt_t * p_ble_evt);
	void on_disconnected(const ble_evt_t * p_ble_evt);
//	void on_advertisement(const ble_evt_t * p_ble_evt);

	void configureAdvertisementParameters();

	/** Transmission complete event
	 *
	 * Inform all services that transmission was completed in case they have notifications pending
	 */
	void onTxComplete(const ble_evt_t * p_ble_evt);

	static void lowPowerTimeout(void* p_context);

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

