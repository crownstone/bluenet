/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 7, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

/**********************************************************************************************************************
 *
 * The Crownstone is a high-voltage (110-240V) switch. It can be used for indoor localization and building automation.
 * It is an essential building block for smart homes. Contemporary switches do come with smartphone apps. However,
 * this does not simplify their use. The user needs to unlock her phone, open the app, navigate to the right screen,
 * and click a particular icon. A truly smart home knows that the user wants the lights on. For this, indoor
 * localization on a room level is required. Now, a smart home can turn on the lights there were the user resides.
 * Indoor localization can be used to adjust lights, temperature, or even music to your presence.
 *
 * Very remarkable, our Crownstone technology is one of the first open-source Internet of Things devices entering the
 * market (2016). Our firmware dates from 2014. Please, contact us if you encounter other IoT devices that are open
 * source!
 *
 * Read more on: https://crownstone.rocks
 *
 * Crownstone uses quite a sophisticated build system. It has to account for multiple devices and multiple
 * configuration options. For that the CMake system is used in parallel. Configuration options are set in a file
 * called CMakeBuild.config. Details can be found in the following documents:
 *
 * - Development/Installation:         https://github.com/crownstone/bluenet/blob/master/docs/INSTALL.md
 * - Protocol Definition:              https://github.com/crownstone/bluenet/blob/master/docs/PROTOCOL.md
 * - Firmware Specifications:          https://github.com/crownstone/bluenet/blob/master/docs/FIRMWARE_SPECS.md
 *
 *********************************************************************************************************************/

#include <cs_Crownstone.h>
#include <cfg/cs_Git.h>
#include <cfg/cs_HardwareVersions.h>
#include <drivers/cs_Watchdog.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_CLogger.h>
#include <logging/cs_Logger.h>

extern "C" {
#include <util/cs_Syscalls.h>
}

/****************************************** Global functions ******************************************/


/**
 * If UART is enabled this will be the message printed out over a serial connection. In release mode we will not by
 * default use the UART, it will need to be turned on.
 */
void initUart(uint8_t pinRx, uint8_t pinTx) {
	serial_config(pinRx, pinTx);
	serial_init(SERIAL_ENABLE_RX_AND_TX);

	LOGi("Welcome to Bluenet!");
	LOGi(" _|_|_|    _|                                            _|     ");
	LOGi(" _|    _|  _|  _|    _|    _|_|    _|_|_|      _|_|    _|_|_|_| ");
	LOGi(" _|_|_|    _|  _|    _|  _|_|_|_|  _|    _|  _|_|_|_|    _|     ");
	LOGi(" _|    _|  _|  _|    _|  _|        _|    _|  _|          _|     ");
	LOGi(" _|_|_|    _|    _|_|_|    _|_|_|  _|    _|    _|_|_|      _|_| ");

	LOGi("Firmware version %s", g_FIRMWARE_VERSION);
	LOGi("Git hash %s", g_GIT_SHA1);
	LOGi("Compilation date: %s", g_COMPILATION_DAY);
	LOGi("Compilation time: %s", __TIME__);
	LOGi("Build type: %s", g_BUILD_TYPE);
	LOGi("Hardware version: %s", get_hardware_version());
	LOGi("Verbosity: %i", SERIAL_VERBOSITY);
	LOGi("UART binary protocol set: %d", CS_UART_BINARY_PROTOCOL_ENABLED);
#ifdef DEBUG
	LOGi("DEBUG: defined");
#else
	LOGi("DEBUG: undefined");
#endif
}

/** Overwrite the hardware version.
 *
 * The firmware is compiled with particular defaults. When a particular product comes from the factory line it has
 * by default FFFF FFFF in this UICR location. If this is the case, there are two options to cope with this:
 *   1. Create a custom firmware per device type where this field is adjusted at runtime.
 *   2. Create a custom firmware per device type with the UICR field state in the .hex file. In the latter case,
 *      if the UICR fields are already set, this might lead to a conflict.
 * There is chosen for the first option. Even if rare cases where there are devices types with FFFF FFFF in the field,
 * the runtime always tries to overwrite it with the (let's hope) proper state.
 */
void overwrite_hardware_version() {
	cs_ret_code_t retCode = writeHardwareBoard();
	LOGd("Board: %p", getHardwareBoard());
	if (retCode != ERR_SUCCESS) {
		LOGe("Failed to write hardware board: retCode=%u", retCode);
	}
}

void printNfcPins() {
	LOGd("NFC pins used as gpio: %u", canUseNfcPinsAsGpio());
}

void on_exit(void) {
	LOGf("PROGRAM TERMINATED");
}


void printBootloaderInfo() {
	bluenet_ipc_bootloader_data_t bootloaderData;
	uint8_t size = sizeof(bootloaderData);
	uint8_t dataSize;
	uint8_t* buf = (uint8_t*)&bootloaderData;
	int retCode  = getRamData(IPC_INDEX_BOOTLOADER_VERSION, buf, size, &dataSize);
	if (retCode != IPC_RET_SUCCESS) {
		LOGw("No IPC data found, error = %i", retCode);
		return;
	}
	if (size != dataSize) {
		LOGw("IPC data struct incorrect size");
		return;
	}
	LOGd("Bootloader version protocol=%u dfu_version=%u build_type=%u",
		 bootloaderData.protocol,
		 bootloaderData.dfu_version,
		 bootloaderData.build_type);
	LOGi("Bootloader version: %u.%u.%u-RC%u",
		 bootloaderData.major,
		 bootloaderData.minor,
		 bootloaderData.patch,
		 bootloaderData.prerelease);
}



/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in
 * cs_sysNrf51.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
	// this enabled the hard float, without it, we get a hardfault
	SCB->CPACR |= (3UL << 20) | (3UL << 22);
	__DSB();
	__ISB();

	atexit(on_exit);

#if CS_SERIAL_NRF_LOG_ENABLED > 0
	NRF_LOG_INIT(NULL);
	NRF_LOG_DEFAULT_BACKENDS_INIT();
	NRF_LOG_INFO("Main");
	NRF_LOG_FLUSH();
#endif

	uint32_t errCode;
	boards_config_t board;
	errCode = configure_board(&board);
	APP_ERROR_CHECK(errCode);

	// Init GPIO pins early in the process!

	if (board.flags.usesNfcPins) {
		cs_ret_code_t retCode = enableNfcPinsAsGpio();
		if (retCode != ERR_SUCCESS) {
			// Not much we can do here.
		}
	}

//	if (IS_CROWNSTONE(board.deviceType)) {
	// Turn dimmer off.
	if (board.pinDimmer != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinDimmer);
		if (board.flags.dimmerInverted) {
			nrf_gpio_pin_set(board.pinDimmer);
		}
		else {
			nrf_gpio_pin_clear(board.pinDimmer);
		}
	}

	if (board.pinEnableDimmer != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinEnableDimmer);
		nrf_gpio_pin_clear(board.pinEnableDimmer);
	}

	// Relay pins.
	if (board.pinRelayOff != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinRelayOff);
		nrf_gpio_pin_clear(board.pinRelayOff);
	}
	if (board.pinRelayOn != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinRelayOn);
		nrf_gpio_pin_clear(board.pinRelayOn);
	}
//	}

	if (board.flags.enableUart) {
		initUart(board.pinRx, board.pinTx);
		LOG_FLUSH();
	}

	// Start the watchdog early (after uart, so we can see the logs).
	Watchdog::init();
	Watchdog::start();

	printNfcPins();
	LOG_FLUSH();

	printBootloaderInfo();


//	// Make a "clicker"
//	nrf_delay_ms(1000);
//	nrf_gpio_pin_set(board.pinGpioRelayOn);
//	nrf_delay_ms(RELAY_HIGH_DURATION);
//	nrf_gpio_pin_clear(board.pinGpioRelayOn);
//	while (true) {
//		nrf_delay_ms(1 * 60 * 1000); // 1 minute on
//		nrf_gpio_pin_set(board.pinGpioRelayOff);
//		nrf_delay_ms(RELAY_HIGH_DURATION);
//		nrf_gpio_pin_clear(board.pinGpioRelayOff);
//		nrf_delay_ms(5 * 60 * 1000); // 5 minutes off
//		nrf_gpio_pin_set(board.pinGpioRelayOn);
//		nrf_delay_ms(RELAY_HIGH_DURATION);
//		nrf_gpio_pin_clear(board.pinGpioRelayOn);
//	}

	Crownstone crownstone(board);

	overwrite_hardware_version();

	// init drivers, configure(), create services and chars,
	crownstone.init(0);
	LOG_FLUSH();

	// run forever ...
	crownstone.run();

	return 0;
}
