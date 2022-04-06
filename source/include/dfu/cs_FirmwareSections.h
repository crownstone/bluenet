/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 2, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_MemoryLayout.h>
#include <util/cs_Store.h>

#include <nrf_fstorage_sd.h>

// -------------- most common usage definitions -----------------

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

struct FirmwareSectionInfo {
	// a pointer to an fstorage object that is configured to read from this particular section.
	nrf_fstorage_t* _fStoragePtr = nullptr;

	// info about the corresponding memory section
	MemorySection _section = {};
};

/**
 * Obtain a firmware section info object about the given section.
 *
 * (runtime variant of the templated version below)
 */
const FirmwareSectionInfo getFirmwareSectionInfo(FirmwareSection sect);

