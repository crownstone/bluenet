/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 3, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <third/nrf/app_config.h>

/**
 * Overhead of encryption
 *
 * TODO: defined somewhere else.
 */
#define ENCRYPTION_OVERHEAD 20

/**
 * Size of MicroApp packet.
 *
 * TODO: encryption isn't the only overhead, is this really the optimal size?
 */
#define MICROAPP_PACKET_SIZE (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - ENCRYPTION_OVERHEAD)

/**
 * Size of MicroApp chunk (without header)
 *
 * TODO: change 8 to sizeof().
 */
#define MICROAPP_CHUNK_SIZE (MICROAPP_PACKET_SIZE - 8)

enum MICROAPP_VALIDATION {
	CS_MICROAPP_VALIDATION_NONE = 0,
	CS_MICROAPP_VALIDATION_CHECKSUM = 1,
	CS_MICROAPP_VALIDATION_ENABLED = 2,
	CS_MICROAPP_VALIDATION_DISABLED = 3,
	CS_MICROAPP_VALIDATION_BOOTS = 4,
	CS_MICROAPP_VALIDATION_FAILS = 5
};

/**
 * Struct stored in FDS to be able to run an app using only this info.
 *
 * Fields:
 *   - start_addr    The address the binary starts at.
 *   - size          The size of the binary.
 *   - checksum      The checksum calculated over the size of the binary (only padded by single zero for an odd size).
 *   - validation    Validation step by step, from checksum correct, enabled, to being able to boot. See MICROAPP_VALIDATION.
 *   - id            The app id.
 *   - offset        The entry into the binary (will assume thumb mode, offset will be incremented with one).
 */
struct __attribute__((packed)) cs_microapp_t {
	uint32_t start_addr = 0;
	uint16_t size = 0;
	uint16_t checksum = 0;
	uint8_t validation = CS_MICROAPP_VALIDATION_NONE;
	uint8_t id = 0;
	uint16_t offset = 0;
};

/**
 * Struct for MicroApp code.
 *
 *  - The protocol byte is reserved for future changes.
 *  - The app_id is an identifier for the app (in theory we can support multiple apps).
 *  - The index refers to the chunk index.
 *  - The count refers to the total number of chunks.
 *  - The data size of total program
 *  - The checksum is a checksum for this chunk.
 *
 * Two remarks:
 *  - The checksum of the last packet is the size for the COMPLETE application.
 *  - The data of the last packet has to be filled with 0xFF. It will be written like that to flash. Moreover, its
 *    checksum will be over the entire buffer, including the 0xFF values.
 *  - When the index field is 0xFF, the information in the package is on enabling/disabling the microapp.
 */
struct __attribute__((packed)) microapp_upload_packet_t {
	uint8_t protocol;
	uint8_t app_id;
	uint8_t index;
	uint8_t count;
	uint16_t size;
	uint16_t checksum;
	uint8_t data[MICROAPP_CHUNK_SIZE];
};

/**
 * Struct for MicroApp meta packet (index equal to 0xFF)
 *
 * TODO: this struct is assumed to be the same size as microapp_upload_packet_t. Make it a union instead.
 *
 *   - The opcode indicates enabling/disabling the app (or other future actions).
 *   - The param0 parameter contains e.g. an offset when opcode is "enable app".
 *   - The param1 parameter contains e.g. a checksum.
 */
struct __attribute__((packed)) microapp_upload_meta_packet_t {
	uint8_t protocol;
	uint8_t app_id;
	uint8_t index;
	uint8_t opcode;
	uint16_t param0;
	uint16_t param1;
	uint8_t data[MICROAPP_CHUNK_SIZE];
};

/**
 * Notification from the MicroApp module.
 *
 *   - The protocol byte is reserved for future struct changes.
 *   - The app_id identifies the app (for now we use only one app).
 *   - The index refers to the chunk index being written.
 *   - The repeat value is an index that goes down (notifications will be written with decreasing repeat value).
 *   - The error value.
 *
 * After getting the first notification it is already fine to start sending a new packet. Do check the index in
 * the notification though (to not accidentally treat it as an ACK for the current chunk).
 */
struct __attribute__((packed)) microapp_notification_packet_t {
	uint8_t protocol;
	uint8_t app_id;
	uint8_t index;
	uint8_t repeat;
	uint16_t error;
};
