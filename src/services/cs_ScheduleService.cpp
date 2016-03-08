/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "services/cs_ScheduleService.h"
//
#include "cfg/cs_Boards.h"
#include "cfg/cs_UuidConfig.h"
#include "cfg/cs_Settings.h"
#include "drivers/cs_RTC.h"

using namespace BLEpp;

ScheduleService::ScheduleService() :
		_currentTimeCharacteristic(NULL)
{

	setUUID(UUID(SCHEDULE_UUID));

	setName(BLE_SERVICE_SCHEDULE);

	init();

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)ScheduleService::staticTick);
}

void ScheduleService::init() {
	LOGi("Create schedule service");

	LOGi("add current time characteristic");
	addCurrentTimeCharacteristic();
}

uint32_t ScheduleService::getTime() {
	return *_currentTimeCharacteristic;
}

void ScheduleService::setTime(uint32_t time) {
	LOGi("Set time to %i", time);
	*_currentTimeCharacteristic = time;
	_rtcTimeStamp = RTC::getCount();
}

void ScheduleService::tick() {
	//! RTC can overflow every 16s
	uint32_t tickDiff = RTC::difference(RTC::getCount(), _rtcTimeStamp);

	//! If more than 1s elapsed since last rtc timestamp:
	//! add 1s to posix time and substract 1s from tickDiff, by increasing the rtc timestamp 1s
	if (_currentTimeCharacteristic && *_currentTimeCharacteristic && tickDiff > RTC::msToTicks(1000)) {
		(*_currentTimeCharacteristic)++;
		_rtcTimeStamp += RTC::msToTicks(1000);
		LOGd("posix time = %i", (uint32_t)(*_currentTimeCharacteristic));
	}
	scheduleNextTick();
}

void ScheduleService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(SCHEDULE_SERVICE_UPDATE_FREQUENCY), this);
}

void ScheduleService::addCurrentTimeCharacteristic() {
	_currentTimeCharacteristic = new Characteristic<uint32_t>();
	addCharacteristic(_currentTimeCharacteristic);
	_currentTimeCharacteristic->setUUID(UUID(getUUID(), CURRENT_TIME_UUID));
	_currentTimeCharacteristic->setName(BLE_CHAR_CURRENT_TIME);
	_currentTimeCharacteristic->setDefaultValue(0);
	_currentTimeCharacteristic->setWritable(true);
	_currentTimeCharacteristic->onWrite([&](const uint32_t& value) -> void {
		setTime(value);
	});
}
