/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 1, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <dfu/cs_FirmwareSections.h>
#include <util/cs_Coroutine.h>

/**
 * This class reads pieces of the firmware that is currently running on this device
 * in order to enabling dfu-ing itself onto another (outdated) crownstone.
 */
class FirmwareReader : public EventListener {
public:



	FirmwareReader();

	/**
	 *
	 */
	void read(FirmwareSection sect, uint16_t startIndex, uint16_t size, const void* data_out);


	cs_ret_code_t init();

protected:
private:
public:
	virtual void handleEvent(event_t& evt) override;

	Coroutine firmwarePrinter;
	uint32_t printRoutine();
};
