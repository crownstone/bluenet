/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include "ble_gap.h"
//#include "ble_gatts.h"
//
#include <common/cs_Types.h>
#include "structs/cs_BufferAccessor.h"
#include <util/cs_BleError.h>
//#include "drivers/cs_Serial.h"
//
//#include "common/cs_MasterBuffer.h"

//using namespace BLEpp;

/* The size of the header used in the scan list message
 *
 * Only one byte is used as header which defines the number of elements
 * in the list
 */
#define SR_HEADER_SIZE 1

/* The maximum number of devices stored during a scan and returned as a list
 *
 * If the maximum number is exceeded, the devices with the lowest occurrence
 * will be replaced
 */
#define SR_MAX_NR_DEVICES 10

/* Structure used to store peripheral devices detected during a scan.
 *
 * We store the bluetooth address of the device, the number of times that the
 * device was seen during scanning (occurrences) and the rssi (received signal
 * strength indication) value of the last received advertisement.
 * **note** the bluetooth address is stored in little-endian (LSB-first) so when
 * displaying it to the user we need to start at the back of the array
 * **note** struct has to be packed in order to avoid word alignment.
 */
struct __attribute__((__packed__)) peripheral_device_t {
	/* bluetooth address */
	uint8_t addr[BLE_GAP_ADDR_LEN];
	/* last rssi value */
	int8_t rssi;
	/* number of occurences (times seen during scan) */
	uint16_t occurrences;
};

/* The size in bytes needed to store the device structure after serializing
 *
 * **Note** this only works if struct ist packed
 */
#define SR_SERIALIZED_DEVICE_SIZE sizeof(peripheral_device_t)

/* Structure of the list of peripheral devices which is sent over Bluetooth
 */
struct peripheral_device_list_t {
	// number of elements in the list
	uint8_t size;
	// list of scanned devices
	peripheral_device_t list[SR_MAX_NR_DEVICES];
};


/* Result of a scan device operation
 *
 * The ScanResult class stores a list of devices which were detected during
 * a scan operation. It keeps track of how often a device was seen and with
 * which RSSI value.
 */
class ScanResult : public BufferAccessor {

private:
	/* A pointer to the list of detected devices
	 */
	peripheral_device_list_t* _buffer;

public:
	/* Default constructor
	 */
	ScanResult() : _buffer(NULL) {}

	/* Release assigned buffer
	 */
	void release() {
		LOGd("release");
		_buffer = NULL;
	}

	/* Print the list of devices for debug purposes to the UART
	 */
	void print() const;

	/* Update the list with the given device
	 * @adrs_ptr pointer to an array of length <BLE_GAP_ADDR_LEN>
	 *   which contains the bluetooth address of the device in little-endian
	 * @rssi rssi value of the advertisement package
	 *
	 * If the bluetooth address is already in the list increase the occurrence by one
	 * and update the rssi value; if it is not yet in the list, add it to the list with
	 * occurrence=1; if the list is full, remove the device with lowest occurrence and
	 * add the new device to the list with occurence=1
	 */
	void update(uint8_t * adrs_ptr, int8_t rssi);

	/* Return the number of devices already stored in the list
	 *
	 * @return number of stored devices
	 */
	uint16_t getSize() const;

	/* Clear the list of devices
	*/
	void clear();

	//////////// BufferAccessor ////////////////////////////

	/* @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		assert(sizeof(peripheral_device_list_t) <= maxLength, "buffer not large enough to hold scan device list!");
		_buffer = (peripheral_device_list_t*)buffer;
		return 0;
	}

	/* @inherit */
	uint16_t getDataLength() const {
		return SR_HEADER_SIZE + SR_SERIALIZED_DEVICE_SIZE * getSize();
	}

	/* @inherit */
	uint16_t getMaxLength() const {
		return SR_HEADER_SIZE + SR_SERIALIZED_DEVICE_SIZE * SR_MAX_NR_DEVICES;
	}

	/* @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};

