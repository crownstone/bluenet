/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 15, 2016
 * License: LGPLv3+
 */
#pragma once

//#include <common/cs_Types.h>

//#include <cstddef>
//#include <cstdint>

//#include <stdlib.h> // malloc, free
//#include <stdint.h> // uint32_t
//
//#include "drivers/cs_Serial.h"

/* Circular Buffer implementation
 *
 * Elements are added at the back and removed from the front. If the capacity
 * of the buffer is reached, the oldest element will be overwritten.
 */

#include <rbc_mesh.h>

typedef struct {
	uint8_t data[RBC_MESH_VALUE_MAX_LEN];
	uint8_t length;
	uint8_t offset;
	rbc_mesh_value_handle_t handle;
} waiting_notification_t;

#define MAX_PENDING_NOTIFICATIONS 20

/* Returns the number of elements stored
 *
 * @return the number of elements stored in the buffer
 */
uint16_t nb_size();

/* Checks if the buffer is empty
 *
 * @return true if empty, false otherwise
 */
bool nb_empty();

/* Checks if the buffer is full
 *
 * @return true if full, false otherwise
 */
bool nb_full();

waiting_notification_t* nb_next();

/* Peek at the oldest element without removing it
 *
 * This returns the value of the oldest element without
 * removing the element from the buffer. Use <CircularBuffer>>pop()>
 * to get the value and remove it at the same time
 *
 * @return the value of the oldest element
 */
waiting_notification_t* nb_peek();

/* Get the oldest element
 *
 * This returns the value of the oldest element and
 * removes it from the buffer
 *
 * @return the value of the oldest element
 */
waiting_notification_t nb_pop();

/* Add an element to the end of the buffer
 *
 * @value the element to be added
 *
 * Elements are added at the end of the buffer and
 * removed from the beginning. If the buffer is full
 * the oldest element will be overwritten.
 */
//void nb_push(waiting_notification_t value);

/* Clears the buffer
 *
 * The buffer is cleared by setting head and tail
 * to the beginning of the buffer. The array itself
 * doesn't have to be cleared
 */
void nb_clear();

//	/* Returns the Nth value, starting from oldest element
//	 *
//	 * Does NOT check if you reached the end, make sure you read no more than size().
//	 */
//	T operator[](uint16_t idx) const {
//		return _array[(_head+idx)%_capacity];
//	}




