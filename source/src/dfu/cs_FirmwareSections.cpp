/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <dfu/cs_FirmwareSections.h>
#include <logging/cs_Logger.h>
#include <components/libraries/bootloader/dfu/nrf_dfu_types.h>
#include <cs_MemoryLayout.h>



// ---------------------------------------------------------------
// local templates to reduce code duplications
// ---------------------------------------------------------------

/**
 * returns MemorySection based on template enum parameter.
 */
template <FirmwareSection Sect>
const MemorySection getMemorySection() {
	LOGw("Location for FirmwareSection %d Unknown", static_cast<uint8_t>(Sect));
	return {};
}

// specializations computing the address values of the known firmware sections

template <>
const MemorySection getMemorySection<FirmwareSection::Mbr>() {
	return softdeviceFlashSection;
}

template <>
const MemorySection getMemorySection<FirmwareSection::Bluenet>() {
	return bluenetFlashSection;
}

template <>
const MemorySection getMemorySection<FirmwareSection::MicroApp>() {
	return microappFlashSection;
}

template <>
const MemorySection getMemorySection<FirmwareSection::Ipc>() {
	return p2pDfuFlashSection;
}

template <>
const MemorySection getMemorySection<FirmwareSection::Fds>() {
	return fdsFlashSection;
}

template <>
const MemorySection getMemorySection<FirmwareSection::Bootloader>() {
	return bootloaderFlashSection;
}

template <>
const MemorySection getMemorySection<FirmwareSection::BootloaderSettings>() {
	return bootloaderSettingsFlashSection;
}


// --------------------------------------------------------------------------------
// -------------------- Memory region definitions for fstorage --------------------
// --------------------------------------------------------------------------------

/**
 * callback for F Storage
 */
void fsEventHandler(nrf_fstorage_evt_t * p_evt)  {
	LOGd("firmware reader fs event handler called");
}


// ------- Bluenet -------
NRF_FSTORAGE_DEF(nrf_fstorage_t fsInstanceBluenet) = {
		.p_api        = &nrf_fstorage_sd,
		.p_flash_info = nullptr,
		.evt_handler  = fsEventHandler,
		.start_addr   = getMemorySection<FirmwareSection::Bluenet>()._start,
		.end_addr     = getMemorySection<FirmwareSection::Bluenet>()._end,
};

// ------- MicroApp -------
NRF_FSTORAGE_DEF(nrf_fstorage_t fsInstanceMicroApp) = {
		.p_api        = &nrf_fstorage_sd,
		.p_flash_info = nullptr,
		.evt_handler  = fsEventHandler,
		.start_addr   = getMemorySection<FirmwareSection::MicroApp>()._start,
		.end_addr     = getMemorySection<FirmwareSection::MicroApp>()._end,
};

// ------- Bootloader -------
NRF_FSTORAGE_DEF(nrf_fstorage_t fsInstanceBootloader) = {
		.p_api        = &nrf_fstorage_sd,
		.p_flash_info = nullptr,
		.evt_handler  = fsEventHandler,
		.start_addr   = getMemorySection<FirmwareSection::Bootloader>()._start,
		.end_addr     = getMemorySection<FirmwareSection::Bootloader>()._end,
};

// ------- BootloaderSettings -------

NRF_FSTORAGE_DEF(nrf_fstorage_t fsInstanceBootloaderSettings) = {
		.p_api        = &nrf_fstorage_sd,
		.p_flash_info = nullptr,
		.evt_handler  = fsEventHandler,
		.start_addr   = getMemorySection<FirmwareSection::BootloaderSettings>()._start,
		.end_addr     = getMemorySection<FirmwareSection::BootloaderSettings>()._end,
};

// -------------------------------------------------------------------
// ------------------------- fstorage getter -------------------------
// -------------------------------------------------------------------

/**
 * returns fstorage pointer based on template enum parameter.
 */
template<FirmwareSection Sect>
nrf_fstorage_t* getFStoragePointer() {
	LOGw("FStorage pointer for section %d not allocated", static_cast<uint8_t>(Sect));
	return nullptr;
}

template<> nrf_fstorage_t* getFStoragePointer<FirmwareSection::Bluenet>() {
	return &fsInstanceBluenet;
}
template<> nrf_fstorage_t* getFStoragePointer<FirmwareSection::MicroApp>() {
	return &fsInstanceMicroApp;
}
template<> nrf_fstorage_t* getFStoragePointer<FirmwareSection::Bootloader>() {
	return &fsInstanceBootloader;
}
template<> nrf_fstorage_t* getFStoragePointer<FirmwareSection::BootloaderSettings>() {
	return &fsInstanceBootloaderSettings;
}

// ---------------------------------------------------------------
// implementation of public methods in the header
// ---------------------------------------------------------------

/**
 * Returns a firmwareSectionInfo containing the info object for the given FirmwareSection.
 *
 * E.g.:
 *   uint32_t start = getFirmwareSectionInfo<FirmwareSection::Bootloader>()._section._start;
 */
template<FirmwareSection Sect>
const FirmwareSectionInfo getFirmwareSectionInfo() {
	return {getFStoragePointer<Sect>(), getMemorySection<Sect>()};
}


const FirmwareSectionInfo getFirmwareSectionInfo(FirmwareSection section) {
	switch(section) {
		case FirmwareSection::Bluenet: return getFirmwareSectionInfo<FirmwareSection::Bluenet>();
		case FirmwareSection::MicroApp: return getFirmwareSectionInfo<FirmwareSection::MicroApp>();
		case FirmwareSection::Bootloader: return getFirmwareSectionInfo<FirmwareSection::Bootloader>();
		case FirmwareSection::BootloaderSettings: return getFirmwareSectionInfo<FirmwareSection::BootloaderSettings>();
		default: return getFirmwareSectionInfo<FirmwareSection::Unknown>();
	}
}
