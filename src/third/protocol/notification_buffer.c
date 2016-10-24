/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 15, 2016
 * License: LGPLv3+
 */

#include <notification_buffer.h>

/* Index of the head (next element to be removed, oldest element) */
uint16_t _head;

/* Index of the tail (first free element) */
uint16_t _tail;

/* Number of elements stored in the buffer */
uint16_t _contentsSize;

waiting_notification_t _array[MAX_PENDING_NOTIFICATIONS];

/* Increases the tail.
 *
 * Increases the contentsSize and the index of the tail. It also
 * wraps the tail around if the end of the array is reached.
 */
void nb_inc_tail() {
	++_tail;
	_tail %= MAX_PENDING_NOTIFICATIONS;
	++_contentsSize;
}

/* Increases the head.
 *
 * Decreases the contentsSize and increases the index of the head.
 * It also wraps around the head if the end of the array is reached.
 */
void nb_inc_head() {
	++_head;
	_head %= MAX_PENDING_NOTIFICATIONS;
	--_contentsSize;
}

/* Returns the number of elements stored
 *
 * @return the number of elements stored in the buffer
 */
uint16_t nb_size() {
	return _contentsSize;
}

/* Checks if the buffer is empty
 *
 * @return true if empty, false otherwise
 */
bool nb_empty() {
	return nb_size() == 0;
}

/* Checks if the buffer is full
 *
 * @return true if full, false otherwise
 */
bool nb_full() {
	return nb_size() == MAX_PENDING_NOTIFICATIONS;
}

/* Returns a pointer to the next writable position in the
 * buffer.
 *
 * Note: This being a circular buffer, be sure to check
 * first nb_full if you don't want to overwrite the oldest
 * position once it wraps around!
 *
 */
waiting_notification_t* nb_next() {
	nb_inc_tail();
	if (_contentsSize > MAX_PENDING_NOTIFICATIONS) {
		nb_inc_head();
	}
	return &_array[_tail];
}

/* Peek at the oldest element without removing it
 *
 * This returns the value of the oldest element without
 * removing the element from the buffer. Use <CircularBuffer>>pop()>
 * to get the value and remove it at the same time
 *
 * @return the value of the oldest element
 */
waiting_notification_t* nb_peek() {
	return &_array[_head];
}

/* Get the oldest element
 *
 * This returns the value of the oldest element and
 * removes it from the buffer
 *
 * @return the value of the oldest element
 */
waiting_notification_t nb_pop() {
	waiting_notification_t res = *nb_peek();
	nb_inc_head();
	return res;
}

/* Clears the buffer
 *
 * The buffer is cleared by setting head and tail
 * to the beginning of the buffer. The array itself
 * doesn't have to be cleared
 */
void nb_clear() {
	_head = 0;
	_tail = -1;
	_contentsSize = 0;
}

