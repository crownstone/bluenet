/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 15, 2016
 * License: LGPLv3+
 */
#pragma once

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

/* Get pointer to the next element in the list
 *
 * This returns the pointer to the next element to write to. If the
 * buffer is full it will overwrite the oldest element
 *
 * @return the pointer to the next element
 */
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

/* Clears the buffer
 *
 * The buffer is cleared by setting head and tail
 * to the beginning of the buffer. The array itself
 * doesn't have to be cleared
 */
void nb_clear();
