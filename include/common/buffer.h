/**
 * Author: Anne van Rossum
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef CS_BUFFER_H
#define CS_BUFFER_H

#include <stdlib.h> // malloc, free
#include <stdint.h> // uint32_t
#include <drivers/serial.h>

// can only be used from C++

// define buffer
// TODO: current implementation is crap
struct buffer {
	uint32_t *buffer;
	uint32_t *ptr;
	uint16_t size;

	// pushes till end of buffer
	void push(uint32_t value) {
		if (full()) return;
		*ptr = value;
		ptr++;
		log(DEBUG, "Added item number %u", (uint8_t)(ptr-buffer));
	}

	// pops till beginning... 0xdeafabba is an error code
	uint32_t pop() {
		uint32_t value = *ptr;
		if (empty()) return 0xdeafabba;
		return value;
	}

	bool empty() {
		return (ptr == buffer);
	}

	bool full() {
		return ((uint16_t)(ptr - buffer) == size);
	}

	void clear() {
		ptr = buffer;
	}

	uint16_t count() {
		log(DEBUG, "Current count: %u", ptr-buffer);
		return (uint16_t)(ptr - buffer);
	}
};

// define alias for struct buffer
typedef struct buffer buffer_t;

#endif // CS_BUFFER_H
