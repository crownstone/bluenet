/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 3, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_StaticConfig.h>
#include <cs_MemoryLayout.h>

#include <cstdint>

/**
 * Max allowed chunk size when uploading a microapp.
 *
 * We could calculate this from MTU or characteristic buffer size for BLE, and UART RX buffer size for UART.
 * But let's just start with a number that fits in both.
 */
constexpr uint16_t MICROAPP_UPLOAD_MAX_CHUNK_SIZE   = 256;

/**
 * Protocol version of the communication over the command handler, the microapp command and result packets.
 */
constexpr uint8_t MICROAPP_CONTROL_COMMAND_PROTOCOL = 1;

/**
 * SDK major version of the data going back and forth between microapp and bluenet within the mutually shared buffers.
 */
constexpr uint8_t MICROAPP_SDK_MAJOR                = 1;

/**
 * SDK minor version of the data going back and forth between microapp and bluenet within the mutually shared buffers.
 */
constexpr uint8_t MICROAPP_SDK_MINOR                = 0;

//! Max flash size of a microapp, must be a multiple of flash page size.
constexpr uint16_t MICROAPP_MAX_SIZE = (g_FLASH_MICROAPP_PAGES * CS_FLASH_PAGE_SIZE);

/**
 * Header of a microapp binary.
 *
 * Assumed to be word sized (multiple of 4B).
 * Has to match section .firmware_header in linker file nrf_common.ld of the microapp repo.
 */
struct __attribute__((__packed__)) microapp_binary_header_t {
	uint8_t sdkVersionMajor;  // Similar to microapp_sdk_version_t
	uint8_t sdkVersionMinor;
	uint16_t size;  // Size of the binary, including this header.

	uint16_t checksum;        // Checksum (CRC16-CCITT) of the binary, after this header.
	uint16_t checksumHeader;  // Checksum (CRC16-CCITT) of this header, with this field set to 0.

	uint32_t appBuildVersion;  // Build version of this microapp.

	uint16_t startOffset;  // Offset in bytes of the first instruction to execute.
	uint16_t reserved;     // Reserved for future use, must be 0 for now.

	uint32_t reserved2;  // Reserved for future use, must be 0 for now.
};

struct __attribute__((packed)) microapp_ctrl_header_t {
	// Protocol of the microapp command and result packets.
	uint8_t protocol;
	// Index of the microapp on the firmware.
	uint8_t index;
};

struct __attribute__((packed)) microapp_upload_t {
	microapp_ctrl_header_t header;
	// Offset in bytes of this chunk of data. Must be a multiple of 4.
	uint16_t offset;
	// Followed by a chunk of the microapp binary.
};

struct __attribute__((packed)) microapp_ctrl_message_t {
	microapp_ctrl_header_t header;
	uint8_t payload[0];
};

/**
 * SDK version: determines the API / protocol between microapp and firmware.
 */
struct __attribute__((packed)) microapp_sdk_version_t {
	uint8_t major;
	uint8_t minor;
};

enum MICROAPP_TEST_STATE {
	MICROAPP_TEST_STATE_UNTESTED = 0,
	MICROAPP_TEST_STATE_TRYING   = 1,
	MICROAPP_TEST_STATE_FAILED   = 2,
	MICROAPP_TEST_STATE_PASSED   = 3,
};

const uint8_t MICROAPP_FUNCTION_NONE = 255;

/**
 * State of tests of a microapp, also stored in flash.
 *
 * Starts with all fields set to 0.
 */
struct __attribute__((packed)) microapp_state_t {
	// Checksum of the microapp, should be equal to the checksum field of the binary.
	uint16_t checksum;
	// Checksum of the microapp, should be equal to the checksumHeader field of the binary.
	uint16_t checksumHeader;
	// Whether the storage space of this app contains data.
	bool hasData : 1;
	// values: MICROAPP_TEST_STATE
	uint8_t checksumTest : 2;
	// Whether the microapp is enabled.
	bool enabled : 1;
	// Values: MICROAPP_TEST_STATE. Checks if the microapp starts, registers callback function in
	// IPC, and returns to firmware.
	uint8_t bootTest : 2;
	// values: ok, excessive
	uint8_t memoryUsage : 1;
	// Did reboot
	uint8_t didReboot : 1;
	// Reserved, must be 0 for now.
	uint16_t reservedTest : 8;
	// Index of registered function that didn't pass yet, and that we are calling now. MICROAPP_FUNCTION_NONE for none.
	uint8_t tryingFunction = MICROAPP_FUNCTION_NONE;
	// Index of registered function that was tried, but didn't pass. MICROAPP_FUNCTION_NONE for none.
	uint8_t failedFunction = MICROAPP_FUNCTION_NONE;
	// Bitmask of registered functions that were called and returned to firmware successfully.
	uint32_t passedFunctions;
};

/**
 * Status of a microapp.
 */
struct __attribute__((packed)) microapp_status_t {
	uint32_t buildVersion;              // Build version of this microapp.
	microapp_sdk_version_t sdkVersion;  // SDK version this microapp was built for.
	microapp_state_t state;
};

/**
 * Packet with all info required to upload a microapp, and to see the status of already uploaded microapps.
 */
struct __attribute__((packed)) microapp_info_t {
	// Protocol of this packet, and the microapp command packets.
	uint8_t protocol      = MICROAPP_CONTROL_COMMAND_PROTOCOL;
	uint8_t maxApps       = g_MICROAPP_COUNT;                // Maximum number of microapps.
	uint16_t maxAppSize   = MICROAPP_MAX_SIZE;               // Maximum binary size of a microapp.
	uint16_t maxChunkSize = MICROAPP_UPLOAD_MAX_CHUNK_SIZE;  // Maximum chunk size for uploading a microapp.
	uint16_t maxRamUsage  = g_RAM_MICROAPP_AMOUNT;           // Maximum RAM usage of a microapp.
	microapp_sdk_version_t sdkVersion;                       // SDK version the firmware supports.
	microapp_status_t appsStatus[g_MICROAPP_COUNT];          // Status of each microapp.
};

/**
 * Header of a data message from the microapp.
 */
struct __attribute__((packed)) microapp_message_out_header_t {
	uint8_t appIndex;
};
