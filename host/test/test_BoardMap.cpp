/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 10, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "boards/cs_BoardMap.h"
#include "cfg/cs_Boards.h"
#include "cfg/cs_HardwareVersions.h"
#include "protocol/cs_ErrorCodes.h"

/**
 * Checks if the board config from hardware board is equal to the board config from UICR data.
 * Also prints out the uicr data for valid boards, so it can be checked against what is out in the field.
 * Does so for all non development boards, or the board given as argument.
 */

void printBoard(uint32_t board, cs_uicr_data_t& uicr) {
	auto hardwareVersion = get_hardware_version_from_uicr(&uicr);

	std::cout << "{" << std::endl
			  << "  \"board\": " << board << "," << std::endl
			  << "  \"uicr\": {" << std::endl
			  << "    \"board\": " << uicr.board << "," << std::endl
			  << "    \"hardwareMajor\": " << (int)uicr.majorMinorPatch.fields.major << "," << std::endl
			  << "    \"hardwareMinor\": " << (int)uicr.majorMinorPatch.fields.minor << "," << std::endl
			  << "    \"hardwarePatch\": " << (int)uicr.majorMinorPatch.fields.patch << "," << std::endl
			  << "    \"reserved1\": " << (int)uicr.majorMinorPatch.fields.reserved << "," << std::endl
			  << "    \"productType\": " << (int)uicr.productRegionFamily.fields.productType << "," << std::endl
			  << "    \"region\": " << (int)uicr.productRegionFamily.fields.region << "," << std::endl
			  << "    \"productFamily\": " << (int)uicr.productRegionFamily.fields.productFamily << "," << std::endl
			  << "    \"reserved2\": " << (int)uicr.productRegionFamily.fields.reserved << "," << std::endl
			  << "    \"productionYear\": " << (int)uicr.productionDateHousing.fields.year << "," << std::endl
			  << "    \"productionWeek\": " << (int)uicr.productionDateHousing.fields.week << "," << std::endl
			  << "    \"productHousing\": " << (int)uicr.productionDateHousing.fields.housing << "," << std::endl
			  << "    \"reserved3\": " << (int)uicr.productionDateHousing.fields.reserved << "," << std::endl
			  << "  }," << std::endl
			  << "  \"hardwareVersion\": " << hardwareVersion << "," << std::endl
			  << "}" << std::endl;
}

void printBoardConfig(boards_config_t& config) {
	std::cout << "Board config:" << std::endl;
	std::cout << "  hardwareBoard=" << config.hardwareBoard << std::endl;
	std::cout << "  pinDimmer=" << (int)config.pinDimmer << std::endl;
	std::cout << "  pinEnableDimmer=" << (int)config.pinEnableDimmer << std::endl;
	std::cout << "  pinRelayOn=" << (int)config.pinRelayOn << std::endl;
	std::cout << "  pinRelayOff=" << (int)config.pinRelayOff << std::endl;

	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  pinAinCurrent[" << i << "]=" << (int)config.pinAinCurrent[i] << std::endl;
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  pinAinVoltage[" << i << "]=" << (int)config.pinAinVoltage[i] << std::endl;
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  pinAinVoltageAfterLoad[" << i << "]=" << (int)config.pinAinVoltageAfterLoad[i] << std::endl;
	}

	std::cout << "  pinAinZeroRef=" << (int)config.pinAinZeroRef << std::endl;
	std::cout << "  pinAinDimmerTemp=" << (int)config.pinAinDimmerTemp << std::endl;
	std::cout << "  pinCurrentZeroCrossing=" << (int)config.pinCurrentZeroCrossing << std::endl;
	std::cout << "  pinVoltageZeroCrossing=" << (int)config.pinVoltageZeroCrossing << std::endl;
	std::cout << "  pinRx=" << (int)config.pinRx << std::endl;
	std::cout << "  pinTx=" << (int)config.pinTx << std::endl;

	for (int i = 0; i < GPIO_INDEX_COUNT; ++i) {
		std::cout << "  pinGpio[" << i << "]=" << (int)config.pinGpio[i] << std::endl;
	}
	for (int i = 0; i < BUTTON_COUNT; ++i) {
		std::cout << "  pinButton[" << i << "]=" << (int)config.pinButton[i] << std::endl;
	}
	for (int i = 0; i < LED_COUNT; ++i) {
		std::cout << "  pinLed[" << i << "]=" << (int)config.pinLed[i] << std::endl;
	}

	std::cout << "  flags:" << std::endl;
	std::cout << "    dimmerInverted=" << (int)config.flags.dimmerInverted << std::endl;
	std::cout << "    enableUart=" << (int)config.flags.enableUart << std::endl;
	std::cout << "    enableLeds=" << (int)config.flags.enableLeds << std::endl;
	std::cout << "    ledInverted=" << (int)config.flags.ledInverted << std::endl;
	std::cout << "    dimmerTempInverted=" << (int)config.flags.dimmerTempInverted << std::endl;
	std::cout << "    usesNfcPins=" << (int)config.flags.usesNfcPins << std::endl;
	std::cout << "    hasAccuratePowerMeasurement=" << (int)config.flags.hasAccuratePowerMeasurement << std::endl;
	std::cout << "    canTryDimmingOnBoot=" << (int)config.flags.canTryDimmingOnBoot << std::endl;
	std::cout << "    canDimOnWarmBoot=" << (int)config.flags.canDimOnWarmBoot << std::endl;
	std::cout << "    dimmerOnWhenPinsFloat=" << (int)config.flags.dimmerOnWhenPinsFloat << std::endl;
	std::cout << "  deviceType=" << (int)config.deviceType << std::endl;

	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  voltageMultiplier[" << i << "]=" << (int)config.voltageMultiplier[i] << std::endl;
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  voltageAfterLoadMultiplier[" << i << "]=" << (int)config.voltageAfterLoadMultiplier[i]
				  << std::endl;
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  currentMultiplier[" << i << "]=" << (int)config.currentMultiplier[i] << std::endl;
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  voltageOffset[" << i << "]=" << (int)config.voltageOffset[i] << std::endl;
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  voltageAfterLoadOffset[" << i << "]=" << (int)config.voltageAfterLoadOffset[i] << std::endl;
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		std::cout << "  currentOffset[" << i << "]=" << (int)config.currentOffset[i] << std::endl;
	}

	std::cout << "  powerOffsetMilliWatt=" << config.powerOffsetMilliWatt << std::endl;
	std::cout << "  voltageAdcRangeMilliVolt=" << config.voltageAdcRangeMilliVolt << std::endl;
	std::cout << "  currentAdcRangeMilliVolt=" << config.currentAdcRangeMilliVolt << std::endl;
	std::cout << "  minTxPower=" << (int)config.minTxPower << std::endl;
	std::cout << "  pwmTempVoltageThreshold=" << config.pwmTempVoltageThreshold << std::endl;
	std::cout << "  pwmTempVoltageThresholdDown=" << config.pwmTempVoltageThresholdDown << std::endl;
	std::cout << "  scanIntervalUs=" << config.scanIntervalUs << std::endl;
	std::cout << "  scanWindowUs=" << config.scanWindowUs << std::endl;
	std::cout << "  tapToToggleDefaultRssiThreshold=" << (int)config.tapToToggleDefaultRssiThreshold << std::endl;
}

enum class BoardConfigCompare { EQUAL = 0, DIFFERENT = 1, NOT_FOUND = 2 };

BoardConfigCompare compareBoardConfig(uint32_t board, cs_uicr_data_t& uicr) {
	boards_config_t config1;
	boards_config_t config2;
	bool ret1;
	bool ret2;

	// Make sure both configs start with the same values, but a value that is likely to be set.
	memset(&config1, 234, sizeof(config1));
	memset(&config2, 234, sizeof(config2));

	ret1 = configure_board_from_hardware_board(board, &config1) == ERR_SUCCESS;
	ret2 = configure_board_from_uicr(&uicr, &config2) == ERR_SUCCESS;
	if (!ret1 && !ret2) {
		return BoardConfigCompare::NOT_FOUND;
	}
	if (!(ret1 && ret2)) {
		std::cout << "ret1=" << ret1 << ", ret2=" << ret2 << std::endl;
		return BoardConfigCompare::DIFFERENT;
	}

	int diff = memcmp(&config1, &config2, sizeof(config1));
	if (diff != 0) {
		printBoardConfig(config1);
		printBoardConfig(config2);
		std::cout << "difference at byte " << std::abs(diff) << std::endl;
		return BoardConfigCompare::DIFFERENT;
	}
	return BoardConfigCompare::EQUAL;
}

int check(uint32_t board, cs_uicr_data_t& uicr, bool showNotFound) {
	auto result = compareBoardConfig(board, uicr);
	switch (result) {
		case BoardConfigCompare::NOT_FOUND:
			if (showNotFound) {
				std::cout << "Could not find board config" << std::endl;
			}
			break;
		case BoardConfigCompare::EQUAL: {
			printBoard(board, uicr);
			break;
		}
		case BoardConfigCompare::DIFFERENT:
		default: {
			std::cout << "Different config from board compared to from uicr" << std::endl;
			printBoard(board, uicr);
			return 1;
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	uint32_t board;
	int result;
	if (argc > 1) {
		std::string str(argv[1]);
		board     = std::stoul(str);
		auto uicr = mapBoardToUicrData(board);
		return check(board, uicr, true);
	}

	std::cout << "Skipping dev boards, starting at board 100" << std::endl;
	for (board = 100; board < 2000; ++board) {
		//		std::cout << "board=" << board << std::endl;
		auto uicr = mapBoardToUicrData(board);
		result    = check(board, uicr, false);
		if (result) {
			break;
		}
	}
	//	assert(result == 0, "Board failed");
	return result;
}
