/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 2, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once




enum class FirmwareSection {
	Mbr = 0,
	SoftDevice,
	Bluenet,
	Microapp,
	Ipc,
	Fds,
	Bootloader,
	MbrSettings,
	BootloaderSettings
};

struct FirmwareSectionInfo {
	FirmwareSection name;
	void* start;
	void* end;
	uint16_t pageCount;
};

static const FirmwareSectionInfo sections[] = {
		{FirmwareSection::Mbr, nullptr, nullptr, 0},
};
