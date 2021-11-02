/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 2, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#include <cfg/cs_AutoConfig.h>
#include <util/cs_Store.h>

enum class FirmwareSection : uint8_t {
	Mbr = 0,
	SoftDevice,
	Bluenet,
	Microapp,
	Ipc,
	Fds,
	Bootloader,
	MbrSettings,
	BootloaderSettings,
	Unknown = 0xff
};

class FirmwareSectionInfo {
public:
	FirmwareSection section = FirmwareSection::Unknown;
	uint32_t startAddress = 0;
	uint32_t length = 0;
//
//	FirmwareSection id() { return section; }
//	bool isValid() { return section != FirmwareSection::Unknown; }
//	void invalidate() { section = FirmwareSection::Unknown; }
};
//
//class FirmwareInformation{
//private:
//	Store<FirmwareSectionInfo, 10> _store;
//};

static const FirmwareSectionInfo firmwareSectionInfo[] = {
		{FirmwareSection::Mbr,                0, 0},
		{FirmwareSection::SoftDevice,         0, 0},
		{FirmwareSection::Bluenet,            g_APPLICATION_START_ADDRESS, g_APPLICATION_LENGTH},
		{FirmwareSection::Microapp,           g_FLASH_MICROAPP_BASE, static_cast<uint32_t>((CS_FLASH_PAGE_SIZE * g_FLASH_MICROAPP_PAGES) - 1)},
		{FirmwareSection::Ipc,                0, 0},
		{FirmwareSection::Fds,                0, 0},
		{FirmwareSection::Bootloader,         0, 0},
		{FirmwareSection::MbrSettings,        0, 0},
		{FirmwareSection::BootloaderSettings, 0, 0},
};
