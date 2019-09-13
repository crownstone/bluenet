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
	enum condition_t { C_STACK_INITIALIZED, C_RADIO_INITIALIZED, C_SERVICES_INITIALIZED };

	std::string                                 _device_name; // 4
	uint16_t                                    _appearance;
	bool                                        _disconnectingInProgress = false;

	// might want to change this to a linked list or something that
	// we can loop over but doesn't allocate more space than needed
	Services_t                                  _services;

	nrf_clock_lf_cfg_t                          _clock_source;
	int8_t                                      _tx_power_level;
	ble_gap_conn_sec_mode_t                     _sec_mode;  // 1
	ble_gap_conn_params_t                       _gap_conn_params;
	uint8_t                                     _conn_cfg_tag = 1;

	//bool                                        _initializedStack;
	//bool                                        _initializedServices;
	//bool                                        _initializedRadio;
	bool                                        _scanning;

	uint16_t                                    _conn_handle;

	callback_connected_t                        _callback_connected;  // 16
	callback_disconnected_t                     _callback_disconnected;  // 16
	callback_radio_t                            _callback_radio;  // 16
	//! 0 = no notification (radio off), 1 = notify radio on, 2 = no notification (radio on), 3 = notify radio off.
	volatile uint8_t                            _radio_notify;

	ble_user_mem_block_t 						_user_mem_block; //! used for user memory (long write)

	//dm_application_instance_t                   _dm_app_handle;
	bool                                        _dm_initialized;

	app_timer_t                                 _lowPowerTimeoutData;
	app_timer_id_t                              _lowPowerTimeoutId;
	app_timer_t                                 _secReqTimerData;
	app_timer_id_t                              _secReqTimerId;
	app_timer_t                                 _connectionKeepAliveTimerData;
	app_timer_id_t                              _connectionKeepAliveTimerId;


	///////////////////// Advertising /////////////////////
	// 13-sep-2019 TODO: move advertising to separate class.

	//! Whether currently advertising.
	bool                                        _advertising = false;

	//! Whether we should be advertising.
	bool                                        _wantAdvertising = false;

	//! Advertisement handle for softdevice. Set by first call to: sd_ble_gap_adv_set_configure().
	uint8_t                                     _advHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

	//! Advertisement parameter: interval.
	uint16_t                                    _advertisingInterval = defaultAdvertisingInterval_0_625_ms;

	//! Advertisement parameter: timeout.
	uint16_t                                    _advertisingTimeout = defaultAdvertisingTimeout_seconds;

	//! Advertisement parameters for softdevice.
	ble_gap_adv_params_t                        _advParams;

	//! Whether advertising parameters have been changed.
	bool                                        _advParamsChanged = false;

	//! Whether we want to advertise being connectable.
	bool                                        _isConnectable = true;

	//! Whether the advertisement data should be used.
	bool                                        _includeAdvertisementData = false;

	//! Whether the scan response data should be used.
	bool                                        _includeScanResponseData = false;

	//! Advertisement data config.
	ble_advdata_t                               _configAdvertisementData;

	//! Scan response data config.
	ble_advdata_t                               _configScanResponse;

	//! Pointers to advertisement data buffers. Buffers will be allocated on first use.
	uint8_t*                                    _advertisementDataBuffers[2] = { NULL };

	//! Pointers to scan response data buffers. Buffers will be allocated on first use.
	uint8_t*                                    _scanResponseBuffers[2] = { NULL };

	//! Encoded advertisement data, has pointers to buffers.
	ble_gap_adv_data_t                          _advData;

	//! Which of the advertisement data buffers is in use by the softdevice.
	uint8_t                                     _advBufferInUse = 1;

	//! iBeacon data field to be encoded into advertisement data.
	ble_advdata_manuf_data_t 					_ibeaconManufData;

	//! Service data field to be encoded into advertisement data.
	ble_advdata_service_data_t                  _crownstoneServiceData;

	//! Pointer to up to data crownstone service data.
	ServiceData*                                _serviceData = NULL;

	///////////////////////////////////////////////////////


	struct stack_state {
		bool advertising;
	} _stack_state;

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

	//! Set initial appearance (for example BLE_APPEARANCE_GENERIC_TAG), not applied unless done before radio init.
	void setAppearance(uint16_t appearance);

	//! Set initial device name, not applied unless done before radio init.
	void setDeviceName(const std::string& deviceName);

	//! Set initial clock source, not applied unless done before radio init.
	void setClockSource(nrf_clock_lf_cfg_t clockSource);

	//! Set initial advertising interval in 0.625 ms units, not applied unless done before radio init.
	void setAdvertisingInterval(uint16_t advertisingInterval);

	//! Set initial advertising timeout in seconds. Set to 0 for no timeout.
	void setAdvertisingTimeoutSeconds(uint16_t advertisingTimeoutSeconds);

	//! Update the appearance (for example BLE_APPEARANCE_GENERIC_TAG).
	void updateAppearance(uint16_t appearance);

	//! Update the advertising interval in 0.625ms units.
	void updateAdvertisingInterval(uint16_t advertisingInterval);

	//! Update the device name.
	void updateDeviceName(const std::string& deviceName);

	std::string & getDeviceName();

	/**
	 * Set and update radio tranmit power.
	 *
	 * @param[in] powerLevel           Power in dBm. Accepted values are -40, -20, -16, -12, -8, -4, 0, and 4.
	 */
	void setTxPowerLevel(int8_t powerLevel);

	int8_t getTxPowerLevel();


	/** Set and update the preferred minimum connection interval in units of 1.25 ms. */
	void updateMinConnectionInterval(uint16_t connectionInterval_1_25_ms);

	/** Set and update the preferred maximum connection interval in units of 1.25 ms. */
	void updateMaxConnectionInterval(uint16_t connectionInterval_1_25_ms);

	/** Set and update the preferred slave latency count. */
	void updateSlaveLatency(uint16_t slaveLatency);

	/** Set and update the preferred connection supervision timeout in units of 10 ms. */
	void updateConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms);

	//! Set on connect callback.
	void setOnConnectCallback(const callback_connected_t& callback);

	//! Set on disconnect callback.
	void setOnDisconnectCallback(const callback_disconnected_t& callback);

	/** Add a service to the stack.
	 */
	Stack & addService(Service* svc);

	/**
	 * Give the stack a pointer to the crownstone service data.
	 *
	 * This is used to set the advertisement data.
	 * 13-sep-2019 TODO: Get this data in some other way?
	 */
	void setServiceData(ServiceData* serviceData) {
		_serviceData = serviceData;
	}

	/**
	 * Sets and updates the advertisement data.
	 *
	 * @param[in] beacon               Object with the iBeacon parameters.
	 * @param[in] deviceType           Device type, for example DEVICE_CROWNSTONE_PLUG.
	 */
	void configureAdvertisement(IBeacon* beacon, uint8_t deviceType);

	/**
	 * Start advertising.
	 *
	 * @return nrf return code.
	 */
	uint32_t startAdvertising();

	/**
	 * Stop advertising.
	 */
	void stopAdvertising();

	/**
	 * Sets and updates advertisement data.
	 *
	 * Sets _advData, and sets it at softdevice if there's a valid handle.
	 * Uses a different buffer than previous time.
	 */
	void updateAdvertisementData();

	/**
	 * Updates the advertisement parameters.
	 *
	 * When parameters are changed, advertising will be stopped and started again.
	 * Else, only the service data is updated.
	 */
	void updateAdvertisementParams();

	/**
	 * Set whether to advertise being connectable.
	 *
	 * Call updateAdvertisementParams() to apply.
	 *
	 * @param[in] connectable          True when connectable.
	 */
	void setConnectable(bool connectable);


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

	bool connected() {
		return _conn_handle != BLE_CONN_HANDLE_INVALID;
	}
	uint16_t getConnectionHandle() {
		return _conn_handle;
	}

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
	void changeToLowTxPowerMode();
	void changeToNormalTxPowerMode();


protected:

	bool checkCondition(condition_t condition, bool expectation);

	//! Update TX power, can be called when already initialized.
	void updateTxPowerLevel();

	//! Update connection parameters, can be called when already initialized.
	void updateConnParams();

	/** Connection request
	 *
	 * On a connection request send it to all services.
	 */
	void onConnected(const ble_evt_t * p_ble_evt);
	void onDisconnected(const ble_evt_t * p_ble_evt);

	/**
	 * Set advertisement parameters from member variables.
	 */
	void configureAdvertisementParameters();

	/**
	 * Sets crownstone service data as advertisement data.
	 *
	 * Writes to _config_advdata or _config_scanrsp.
	 *
	 * @param[in] deviceType           Type of device (e.g. DEVICE_CROWNSTONE_PLUG).
	 * @param[in] asScanResponse       Whether the advertisement data is scan response.
	 */
	void configureServiceData(uint8_t deviceType, bool asScanResponse);

	/**
	 * Sets iBeacon as advertisement data.
	 *
	 * Writes to _config_advdata.
	 *
	 * @param[in] beacon               The iBeacon data.
	 */
	void configureIBeaconAdvData(IBeacon* beacon);

	/**
	 * Stop and start advertising.
	 *
	 * This will make sure new advertising parameters are applied.
	 */
	void restartAdvertising();

	void printAdvertisement();

	/**
	 * Sets advertisement parameters to be connectable.
	 */
	void setConnectableAdvParams();

	/**
	 * Sets advertisement parameters to be non-connectable.
	 */
	void setNonConnectableAdvParams();

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

