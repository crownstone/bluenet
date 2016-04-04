/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "services/cs_AlertService.h"
//
#include "cfg/cs_Boards.h"
#include "cfg/cs_UuidConfig.h"
#include "cfg/cs_Settings.h"

using namespace BLEpp;

AlertService::AlertService() :
//		_supportedNewAlertCharacteristic(NULL),
		_newAlertCharacteristic(NULL)
//		_supportedUnreadAlertCharacteristic(NULL),
//		_unreadAlertCharacteristic(NULL),
//		_controlPointCharacteristic(NULL),
{

	setUUID(UUID(ALERT_UUID));

	setName(BLE_SERVICE_ALERT);

	init();

	// enable again if the tick function needs to do something
//	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)AlertService::staticTick);
}

void AlertService::init() {
	LOGi("Create alert service");

	LOGi("add new alert characteristic");
	addNewAlertCharacteristic();
}

void AlertService::tick() {
	scheduleNextTick();
}

void AlertService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(ALERT_SERVICE_UPDATE_FREQUENCY), this);
}

new_alert_t AlertService::getAlert() {
	if (_newAlertCharacteristic != NULL) {
		uint16_t val = *_newAlertCharacteristic;
		return *((new_alert_t*)&val);
	}
	else {
		new_alert_t alert;
		return alert;
	}
}

void AlertService::setAlert(new_alert_t alert) {
	if (_newAlertCharacteristic != NULL) {
		*_newAlertCharacteristic = *((uint16_t*)&alert);
	}
}

void AlertService::setAlert(uint8_t type) {
	if (_newAlertCharacteristic != NULL) {
		new_alert_t alert;
		alert.type = type;
		alert.num = 1;
		uint16_t* val;
		val = (uint16_t*)&alert;
		*_newAlertCharacteristic = *val;
	}
}

void AlertService::addAlert(uint8_t bluenetType) {
	if (_newAlertCharacteristic != NULL && bluenetType < 7) {
		uint16_t val = *_newAlertCharacteristic;
		new_alert_t alert = *((new_alert_t*)&val);
		alert.type |= ALERT_BLUENET_BASE;
		alert.type |= 1 << bluenetType;
		alert.num++;
		val = *((uint16_t*)&alert);
		*_newAlertCharacteristic = val;
	}
}

void AlertService::addNewAlertCharacteristic() {
	_newAlertCharacteristic = new Characteristic<uint16_t>();
	addCharacteristic(_newAlertCharacteristic);
	_newAlertCharacteristic->setUUID(UUID(getUUID(), NEW_ALERT_UUID));
	_newAlertCharacteristic->setName(BLE_CHAR_NEW_ALERT);
	_newAlertCharacteristic->setDefaultValue(0);
	_newAlertCharacteristic->setWritable(true);
	_newAlertCharacteristic->onWrite([&](const uint16_t& value) -> void {
		LOGd("Resetting Alerts");
	});
}
