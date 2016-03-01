/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include "drivers/cs_Storage.h"
#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include "structs/cs_AlertAccessor.h"

#define ALERT_SERVICE_UPDATE_FREQUENCY 10 //! hz

class AlertService : public BLEpp::Service {
public:
//	typedef function<int8_t()> func_t;

	/** Constructor for alert notification service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store current limit.
	 * It also initializes all characteristics.
	 */
	AlertService();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();

	new_alert_t getAlert();

	/** Set the alert that the service sends out
	 */
	void setAlert(new_alert_t alert);

	void setAlert(uint8_t type);

	/** Adds an extended alert type to existing alert
	 * See cs_AlertAccessor.h for available types
	 */
	void addAlert(uint8_t bluenetType);

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things.
	 * Urgent matters have to be resolved immediately in interrupt service handlers.
	 */
	void tick();
	void scheduleNextTick();

protected:
	//! The characteristics in this service, based on:
	//! https://developer.bluetooth.org/TechnologyOverview/Pages/ANS.aspx
//	void addSupportedNewAlertCharacteristic();
	void addNewAlertCharacteristic();
//	void addSupportedUnreadAlertCharacteristic();
//	void addUnreadAlertCharacteristic();
//	void addControlPointCharacteristic();


private:
	//! References to characteristics that need to be written from other functions
//	BLEpp::Characteristic<uint8_t> *_supportedNewAlertCharacteristic;
	BLEpp::Characteristic<uint16_t> *_newAlertCharacteristic;
//	BLEpp::Characteristic<uint8_t> *_supportedUnreadAlertCharacteristic;
//	BLEpp::Characteristic<uint16_t> *_unreadAlertCharacteristic;
//	BLEpp::Characteristic<uint16_t> *_controlPointCharacteristic;
};
