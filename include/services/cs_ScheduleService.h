/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include <events/cs_EventListener.h>

/** ScheduleService organizes ticks for all components for non-urgent timing.
 */
class ScheduleService : public Service, EventListener {
public:
//	typedef function<int8_t()> func_t;

	/** Constructor for alert notification service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store current limit.
	 * It also initializes all characteristics.
	 */
	ScheduleService();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

protected:

	void addCurrentTimeCharacteristic();
	void addWriteScheduleEntryCharacteristic();
	void addListScheduleEntriesCharacteristic();

private:
	//! References to characteristics that need to be written from other functions
	Characteristic<uint32_t> *_currentTimeCharacteristic;
	Characteristic<uint8_t*> *_writeScheduleEntryCharacteristic;
	Characteristic<uint8_t*> *_listScheduleEntriesCharacteristic;

};
