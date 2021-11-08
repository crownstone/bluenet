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
	Microapp,
	Ipc,
	Fds,
	Bootloader,
	MbrSettings,
	BootloaderSettings,
	Unknown = 0xff
};

struct FirmwareSectionLocation {
	uint32_t _start;
	uint32_t _end;
};

struct FirmwareSectionInfo {
	nrf_fstorage_t* _fStoragePtr;
	FirmwareSectionLocation _addr;

	FirmwareSectionInfo(nrf_fstorage_t* fStoragePtr = nullptr, FirmwareSectionLocation addr = {0,0})
		: _fStoragePtr(fStoragePtr), _addr(addr) {}
};


// ---------------------- getter methods ------------------

/**
 * Returns a firmwareSectionInfo containing the pointer to FStorage config struct,
 * and the start/end addresses of the section.
 *
 * E.g.:
 *   uint32_t start = getFirmwareSectionInfo<FirmwareSection::Bootloader>()._addr._start;
 */
template<FirmwareSection Sect>
const FirmwareSectionInfo getFirmwareSectionInfo() {
	LOGd("Section Type Unknown");
	return FirmwareSectionInfo(nullptr, {0, 0});
}

// ------------------------------- section addresses ---------------------

template<FirmwareSection Sect>
const FirmwareSectionLocation getFirmwareSectionLocation() {
	return {0, 0};
}

