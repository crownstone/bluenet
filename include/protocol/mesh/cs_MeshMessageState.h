#pragma once

#include <protocol/mesh/cs_MeshMessageCommon.h>

#include <iostream>

struct __attribute__((__packed__)) state_item_t {
	id_type_t id;
	uint8_t switchState;
	uint32_t powerUsage;
	uint32_t accumulatedEnergy;
};

#define STATE_HEADER_SIZE (sizeof(uint8_t) - sizeof(uint8_t) - sizeof(uint8_t))
#define MAX_STATE_ITEMS ((MAX_MESH_MESSAGE_LENGTH - STATE_HEADER_SIZE) / sizeof(state_item_t) * 4)

/**
 * The state_message_t is a struct that is a complete circular buffer to be put into an advertisement frame.
 */
struct __attribute__((__packed__)) state_message_t {
	// the index to read the first element
	uint8_t head;
	// the index to add the next element
	uint8_t tail;
	// the number of elements in the list
	uint8_t size;
	// the list itself
	state_item_t list[MAX_STATE_ITEMS];
};


/**
 * The class state_message wraps around state_message_t to provide it with accessor methods like iterators that can
 * be used in for loops, etc.
 */
class state_message {
	private:
		//! internal reference to a state message
		state_message_t &_state_message_t;

	public:
		//! Iterator struct
		class iterator {
			private:
				friend class state_message;

				state_message_t *_smt;
				
				uint8_t _current;
				
				iterator(uint8_t current = 0): _current(current) {
				}
				
				iterator(state_message_t * smt, uint8_t current = 0): _smt(smt), _current(current) {
				}

			public:
				//! Post-increment operator
				const iterator & operator++() {
					_current = (_current + 1);
					return *this;
				}

				//! Pre-increment operator
				iterator operator++(int) {
					iterator result = *this;
					++(*this);
					return result;
				}

				bool operator==(const iterator it) {
					return _current == it._current;
				}

				bool operator!=(const iterator it) {
					return _current != it._current;
				}

				state_item_t& operator*() {
					return _smt->list[_current % MAX_STATE_ITEMS];
				}
		};

		//! Constructor requires a reference to a state_message_t buffer
		state_message(state_message_t & smt);

		//! Return pointer to first element
		iterator begin();

		/**
		 * Return pointer to last element. Note that this iterator is "equality comparable". Only random access 
		 * iterators are relationally comparable. That's why you always use a for-loop like this:
		 *
		 * for (iter = sm.begin(); iter != sm.end(); iter++) {
		 *   // something with *iter
		 * }
		 */
		iterator end();

		/**
		 * Add item to the buffer
		 */
		void push_back(state_item_t item);

		/**
		 * Set all values to zero in buffer.
		 */
		void clear();
};
