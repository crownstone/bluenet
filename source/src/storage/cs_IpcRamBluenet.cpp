/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 24, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <storage/cs_IpcRamBluenet.h>
#include <ipc/cs_IpcRamData.h>

#define LogIpcRamBluenetInfo LOGi
#define LogIpcRamBluenetDebug LOGd
#define LogIpcRamBluenetVerbose LOGvv
#define LogLevelIpcRamBluenetVerbose SERIAL_VERY_VERBOSE

IpcRamBluenet::IpcRamBluenet() {

}

IpcRamBluenet& IpcRamBluenet::getInstance() {
	// Use static variant of singleton, no dynamic memory allocation
	static IpcRamBluenet instance;
	return instance;
}

void IpcRamBluenet::init() {
	// Get bluenet data from IPC ram.
	uint8_t ipcDataSize = 0;

	IpcRetCode ipcCode = getRamData(IPC_INDEX_BLUENET_TO_BLUENET, _ipcData.raw, &ipcDataSize, sizeof(_ipcData.raw));
	if (ipcCode != IPC_RET_SUCCESS) {
		LogIpcRamBluenetDebug("Failed to get IPC data: ipcCode=%i", ipcCode);
		clearData();
		return;
	}
	if (_ipcData.bluenetRebootData.ipcDataMajor != BLUENET_IPC_BLUENET_REBOOT_DATA_MAJOR) {
		LogIpcRamBluenetInfo("Different major version: major=%u expected=%u",
				_ipcData.bluenetRebootData.ipcDataMajor,
				BLUENET_IPC_BLUENET_REBOOT_DATA_MAJOR);
		clearData();
		return;
	}

	// We need to ignore this warning, it's triggered because the minimum minor is 0, making the statement always false.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
	if (_ipcData.bluenetRebootData.ipcDataMinor < BLUENET_IPC_BLUENET_REBOOT_DATA_MINOR) {
		LogIpcRamBluenetInfo("Minor version too low: minor=%u minimum=%u",
				_ipcData.bluenetRebootData.ipcDataMinor,
				BLUENET_IPC_BLUENET_REBOOT_DATA_MINOR);
		clearData();
		return;
	}
#pragma GCC diagnostic pop

	LogIpcRamBluenetInfo("Loaded bluenet IPC ram: size=%u", ipcDataSize);
	printData();

	// Always overwrite the minor version, it may have increased.
	_ipcData.bluenetRebootData.ipcDataMinor = BLUENET_IPC_BLUENET_REBOOT_DATA_MINOR;
	_isValidOnBoot = true;
}

bool IpcRamBluenet::isValidOnBoot() {
	return _isValidOnBoot;
}

void IpcRamBluenet::clearData() {
	LogIpcRamBluenetVerbose("clearData");
	memset(_ipcData.raw, 0 , sizeof(_ipcData.raw));
	_ipcData.bluenetRebootData.ipcDataMajor = BLUENET_IPC_BLUENET_REBOOT_DATA_MAJOR;
	_ipcData.bluenetRebootData.ipcDataMinor = BLUENET_IPC_BLUENET_REBOOT_DATA_MINOR;
	printData();
}

const bluenet_ipc_bluenet_data_t& IpcRamBluenet::getData() {
	return _ipcData.bluenetRebootData;
}

void IpcRamBluenet::updateData() {
	LogIpcRamBluenetVerbose("updateData");
	IpcRetCode ipcCode = setRamData(IPC_INDEX_BLUENET_TO_BLUENET, _ipcData.raw, sizeof(_ipcData.bluenetRebootData));
	if (ipcCode != IPC_RET_SUCCESS) {
		LOGw("Failed to set IPC data: ipcCode=%i", ipcCode);
	}
}

void IpcRamBluenet::updateEnergyUsed(const int64_t& energyUsed) {
	_ipcData.bluenetRebootData.energyUsedMicroJoule = energyUsed;
	updateData();
}

void IpcRamBluenet::updateMicroappData(uint8_t appIndex, const microapp_reboot_data_t& data) {
	if (appIndex > sizeof(_ipcData.bluenetRebootData.microapp) / sizeof(_ipcData.bluenetRebootData.microapp[0])) {
		LOGw("Invalid appIndex=%u", appIndex);
		return;
	}
	_ipcData.bluenetRebootData.microapp[appIndex] = data;
	updateData();
	printData();
}

void IpcRamBluenet::printData() {
	_log(LogLevelIpcRamBluenetVerbose, true, "Bluenet IPC data:");
	_log(LogLevelIpcRamBluenetVerbose, true, "  energy=%i J", static_cast<int>(_ipcData.bluenetRebootData.energyUsedMicroJoule / 1000 / 1000));
	for (unsigned int i = 0; i < sizeof(_ipcData.bluenetRebootData.microapp) / sizeof(_ipcData.bluenetRebootData.microapp[0]); ++i) {
		_log(LogLevelIpcRamBluenetVerbose, true, "  microapp %u: running=%u", i, _ipcData.bluenetRebootData.microapp[0].running);
	}
	_logArray(LogLevelIpcRamBluenetVerbose, true, _ipcData.raw, sizeof(_ipcData.raw));
}

