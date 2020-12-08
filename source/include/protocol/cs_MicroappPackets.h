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
 * Opcodes for microapp commands.
 */
enum MICROAPP_OPCODE {
	CS_MICROAPP_OPCODE_UPLOAD   = 0x01,
	CS_MICROAPP_OPCODE_VALIDATE = 0x02,
	CS_MICROAPP_OPCODE_ENABLE   = 0x03,
	CS_MICROAPP_OPCODE_DISABLE  = 0x04,
	CS_MICROAPP_OPCODE_REQUEST  = 0x05,
};


/**
 * Overhead of encryption
 *
 * TODO: Bart: defined somewhere else.
 *       Anne: Yes, this is just one variable for all protocol overhead. Feel free to make it less hard-coded.
 *             It is concerning all bytes before `commandData` in 
 *               `CommandHandler::handleMicroappCommand(cs_data_t commandData, ...)`
 *             Following the code this must be e.g.:
 *               + ... from standard overhead
 *               + 5 bytes from sizeof(control_packet_header_t)
 */
#define ENCRYPTION_OVERHEAD 20

/**
 * Size of Microapp packet.
 *
 * TODO: Bart: Encryption isn't the only overhead, is this really the optimal size?
 */
#define MICROAPP_PACKET_SIZE (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - ENCRYPTION_OVERHEAD)

/*
 * Header part of upload packet. Only used to define sizeof(microapp_upload_header_packet_t) = 8 below.
 */
struct __attribute__((packed)) microapp_upload_header_packet_t {
	uint8_t  protocol;
	uint8_t  app_id;
	uint8_t  opcode;
	uint8_t  index;
	uint8_t  count;
	uint16_t size;
	uint16_t checksum;
};

/*
 * Throw away that part of the chunk that cannot be used. If NRF_SDH_BLE_GATT_MAX_MTU_SIZE changes, this has to
 * be adjusted as well. 
 */
#define MICROAPP_CHUNK_RESERVED 0

/**
 * Size of Microapp chunk (without header)
 */
#define MICROAPP_CHUNK_SIZE (MICROAPP_PACKET_SIZE - sizeof(microapp_upload_header_packet_t) - MICROAPP_CHUNK_RESERVED)

/**
 * TODO: Anne @Bart. This would be my proposal.
 *
 * We can dynamically adjust it with a change of the actual MTU size used. Then the sending party should know.
 * We can introduce a command CS_MICROAPP_OPCODE_REQUEST that sends a request for uploading a new microapp.
 * The return packet can indicate if the area is free (or requires an erase first) and indicate the mtu size to 
 * use (and thus the chunk size). In that way, we can make this all dynamic.
 */
static_assert (MICROAPP_CHUNK_SIZE % 4 == 0, "Microapp chunk size is misaligned with memory");

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
 *   - validation    Validation state: checksum correct, enabled, to being able to boot. See MICROAPP_VALIDATION.
 *   - id            The app id.
 *   - offset        The entry into the binary (will assume thumb mode, offset will be incremented with one).
 */
struct __attribute__((packed)) cs_microapp_t {
	uint32_t start_addr = 0;
	uint16_t size = 0;
	uint16_t checksum = 0;
	uint8_t  validation = CS_MICROAPP_VALIDATION_NONE;
	uint8_t  id = 0;
	uint16_t offset = 0;
};

/**
 * Header for all packets.
 */
struct __attribute__((packed)) microapp_packet_header_t {
	uint8_t  protocol;
	uint8_t  app_id;
	uint8_t  opcode;
};

/**
 * Struct for microapp upload packet.
 *
 *  - The protocol byte is reserved for future changes.
 *  - The app_id is an identifier for the app (in theory we can support multiple apps).
 *  - The opcode should be `CS_MICROAPP_OPCODE_UPLOAD`.
 *
 *  - The index refers to the chunk index.
 *  - The checksum is a checksum for this chunk.
 *  - The data itself.
 *
 * Two remarks:
 *  - The data of the last packet has to be filled with `0xFF`. It will be written like that to flash.
 *  - The data of the last packet should be over this entire buffer (including the `0xFF` values).
 */
struct __attribute__((packed)) microapp_upload_packet_t {
	//microapp_packet_header_t header;
	uint8_t  protocol;
	uint8_t  app_id;
	uint8_t  opcode;
	uint8_t  index;
	uint16_t checksum;
	uint8_t  data[MICROAPP_CHUNK_SIZE];
};

/**
 * Struct for microapp request. If for example th chunk size does not match with what is expected, the
 * response indicates the chunk size to use. 
 *
 *   - The size of the total program (without `0xFF` padding).
 *   - The number of chunks to be expected (count * chunk_size =< size).
 *   - The chunk size to be used.
 */
struct __attribute__((packed)) microapp_request_packet_t {
	//microapp_packet_header_t header;
	uint8_t  protocol;
	uint8_t  app_id;
	uint8_t  opcode;
	uint16_t size;
	uint8_t  count;
	uint8_t  chunk_size;
};

/**
 * Struct for the microapp enable packet.
 *
 *   - The opcode should be `CS_MICROAPP_OPCODE_ENABLE` or `CS_MICROAPP_OPCODE_DISABLE`.
 */
struct __attribute__((packed)) microapp_enable_packet_t {
	uint8_t  protocol;
	uint8_t  app_id;
	uint8_t  opcode;
	uint16_t offset;
};

/**
 * Struct for the microapp validate packet.
 *
 *   - The size parameter is about the complete program (but without the `0xFF` padding).
 */
struct __attribute__((packed)) microapp_validate_packet_t {
	uint8_t  protocol;
	uint8_t  app_id;
	uint8_t  opcode;
	uint16_t size;
	uint16_t checksum;
};

/**
 * Struct for the microapp erase packet.
 *
 *   - The opcode is `CS_MICROAPP_OPCODE_ERASE`.
 *
 * This currently erases anything without taking into account size and without asking for permissions to erase.
 * In this way it can always be used to restore the microapp region to the original state.
 */
struct __attribute__((packed)) microapp_erase_packet_t {
	uint8_t protocol;
	uint8_t app_id;
	uint8_t opcode;
};

/**
 * Notification from the microapp module.
 *
 *   - The protocol byte is reserved for forward-compatibility.
 *   - The app_id identifies the app (for now we use only one app).
 *   - The opcode (often the opcode of the writing action).
 *   - The payload (on upload refers to index being written).
 *   - The repeat value is an index that goes down (notifications will be written with decreasing repeat value).
 *   - The error value.
 *
 * After getting the first notification it is already fine to start sending a new packet. Do check the index in
 * the notification though (to not accidentally treat it as an ACK for the current chunk).
 */
struct __attribute__((packed)) microapp_notification_packet_t {
	uint8_t  protocol;
	uint8_t  app_id;
	uint8_t  opcode;
	uint8_t  payload;
	uint8_t  repeat;
	uint16_t error;
};

