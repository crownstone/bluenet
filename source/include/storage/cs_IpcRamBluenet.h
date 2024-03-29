/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 24, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ipc/cs_IpcRamDataContents.h>

/**
 * Class to operate on bluenet to bluenet IPC ram.
 *
 * An alternative to this class would be to add a method to the IpcRamData class to update a single field.
 */
class IpcRamBluenet {
public:
	/**
	 * Get the singleton instance.
	 */
	static IpcRamBluenet& getInstance();

	/**
	 * Reads the IPC ram.
	 *
	 * If it's not valid, the data will be cleared.
	 * After init, the data is valid.
	 */
	void init();

	/**
	 * Whether the IPC ram data was valid before init.
	 */
	bool isValidOnBoot();

	/**
	 * Get the current data.
	 *
	 * Don't forget to first check whether the data is valid on boot.
	 */
	const bluenet_ipc_bluenet_data_t& getData();

	/**
	 * Update the energy field.
	 *
	 * Will write the data to IPC ram.
	 */
	void updateEnergyUsed(const int64_t& energyUsed);

	/**
	 * Update a microapp field.
	 *
	 * Will write the data to IPC ram.
	 */
	void updateMicroappData(uint8_t appIndex, const microapp_reboot_data_t& data);

private:
	//! Constructor, singleton, thus made private
	IpcRamBluenet();

	//! Copy constructor, singleton, thus made private
	IpcRamBluenet(IpcRamBluenet const&)            = delete;

	//! Assignment operator, singleton, thus made private
	IpcRamBluenet& operator=(IpcRamBluenet const&) = delete;

	//! Cache the data in IPC.
	bluenet_ipc_data_payload_t _ipcData;

	bool _isValidOnBoot = false;

	//! Clear the cached data.
	void clearData();

	//! Copy cache to IPC ram.
	void updateData();

	void printData();
};
