/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "cs_StreamBuffer.h"

//using namespace BLEpp;

//#define SB_HEADER_SIZE 4
//
//#define SB_SUCCESS                               0
//#define SB_BUFFER_NOT_INITIALIZED                1
//#define SB_BUFFER_NOT_LARGE_ENOUGH               2

enum OpCode {
	READ_VALUE       = 0,
	WRITE_VALUE,
	NOTIFY_VALUE
};

//typedef uint8_t ERR_CODE;

/** Structure for a StreamBuffer
 *
 * Requires MASTER_BUFFER_SIZE to be set.
 */
template <typename T>
struct __attribute__((__packed__)) charac_stream_t {
	uint8_t type;
	uint8_t opCode; //! reserved for byte alignment
	uint16_t length;
	T payload[(MASTER_BUFFER_SIZE-SB_HEADER_SIZE)/sizeof(T)];
};

/** General StreamBuffer with type, length, and payload
 *
 * General class that can be used to send arrays of values over Bluetooth, of which the first byte is a type
 * or identifier byte, the second one the length of the payload (so, minus identifier and length byte itself)
 * and the next bytes are the payload itself.
 */
template <typename T>
class CharacBuffer : public StreamBuffer<T> {

public:
	/** Default constructor
	 *
	 * Constructor does not initialize the payload.
	 */
	CharacBuffer(): StreamBuffer<T>() {
	};

	/** Return the opcode assigned to the SreamBuffer
	 *
	 * @return the type, see <OpCode>
	 */
	inline uint8_t opCode() const { return ((charac_stream_t<T>*)StreamBuffer<T>::_buffer)->opCode; }

	/** Set the opcode for this stream buffer
	 *
	 * @type the type, see <OpCode>
	 */
	inline void setOpCode(uint8_t opCode) { ((charac_stream_t<T>*)StreamBuffer<T>::_buffer)->opCode = opCode; }

	/** @inherit */
//	int assign(uint8_t *buffer, uint16_t size) {
//		StreamBuffer<T>::assign(buffer, size);
//		_characBuffer = (charac_stream_t<T>*)buffer;
//		return 0;
//	}
};
