/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 2, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#include <cfg/cs_AutoConfig.h>
#include <util/cs_Store.h>

#include <nrf_fstorage_sd.h>

// -------------- struct definitions -----------------

enum class FirmwareSection : uint8_t {
	Mbr = 0,
	SoftDevice,
	Bluenet,
	MicroApp,
	Ipc,
	Fds,
	Bootloader,
	MbrSettings,
	BootloaderSettings,
	Unknown = 0xff
};


// ----------------------- location object -----------------------

struct FirmwareSectionLocation {
	uint32_t _start;
	uint32_t _end;
};

template<FirmwareSection Sect>
const FirmwareSectionLocation getFirmwareSectionLocation() {
	LOGd("Location for FirmwareSection %d Unknown", static_cast<uint8_t>(Sect));
	return {0, 0};
}

// explicit specializations for implemented sections
template<> const FirmwareSectionLocation getFirmwareSectionLocation<FirmwareSection::Bluenet>();
template<> const FirmwareSectionLocation getFirmwareSectionLocation<FirmwareSection::MicroApp>();
template<> const FirmwareSectionLocation getFirmwareSectionLocation<FirmwareSection::Bootloader>();
template<> const FirmwareSectionLocation getFirmwareSectionLocation<FirmwareSection::Mbr>();
template<> const FirmwareSectionLocation getFirmwareSectionLocation<FirmwareSection::BootloaderSettings>();



// ----------------------- Info object -----------------------

/**
 * Describes a firmware sections start/end adresses and contains a pointer to an
 * fstorage object that is configured to read from this particular section.
 */
struct FirmwareSectionInfo {
	nrf_fstorage_t* _fStoragePtr;
	FirmwareSectionLocation _addr;

	FirmwareSectionInfo(nrf_fstorage_t* fStoragePtr = nullptr, FirmwareSectionLocation addr = {0,0})
		: _fStoragePtr(fStoragePtr), _addr(addr) {}
};

/**
 * Returns a firmwareSectionInfo containing the pointer to FStorage config struct,
 * and the start/end addresses of the section.
 *
 * E.g.:
 *   uint32_t start = getFirmwareSectionInfo<FirmwareSection::Bootloader>()._addr._start;
 */
template<FirmwareSection Sect>
const FirmwareSectionInfo getFirmwareSectionInfo() {
	LOGd("Info for FirmwareSection %d Unknown", static_cast<uint8_t>(Sect));
	return FirmwareSectionInfo(nullptr, {0, 0});
}

// explicit specializations for implemented sections
template<> const FirmwareSectionInfo getFirmwareSectionInfo<FirmwareSection::Bluenet>();
template<> const FirmwareSectionInfo getFirmwareSectionInfo<FirmwareSection::MicroApp>();
template<> const FirmwareSectionInfo getFirmwareSectionInfo<FirmwareSection::Bootloader>();
template<> const FirmwareSectionInfo getFirmwareSectionInfo<FirmwareSection::Mbr>();
template<> const FirmwareSectionInfo getFirmwareSectionInfo<FirmwareSection::BootloaderSettings>();

