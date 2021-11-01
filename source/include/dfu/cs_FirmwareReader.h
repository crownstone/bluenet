/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 1, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>

/**
 * This class reads pieces of the firmware that is currently running on this device
 * in order to enabling dfu-ing itself onto another (outdated) crownstone.
 */
class FirmwareReader : public EventListener {
public:
	enum class FirmwareSection {
		Mbr,
		SoftDevice,
		Bluenet,
		Microapp,
		Ipc,
		Fds,
		Bootloader,
		MbrSettings,
		BootloaderSettings
	};

	/**
	 *
	 */
	void read(FirmwareSection sect, uint16_t size, const void* data_out);

protected:
private:
public:
	virtual void handleEvent(event_t& evt) override;
};
