/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 19, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

// typename uint8_t median_filter_data_t;

typedef struct node {
	value_id_t index;
	struct node* next;
} node_t;

MedianFilter::MedianFilter() {
	// list of node_t elements
	_nodes = calloc(sizeof(node_t), SLIDING_WINDOW_SIZE);
	// start with a sequential order
	for (uint8_t i = 0; i < SLIDING_WINDOW_SIZE - 1; ++i) {
		_nodes[i].next  = _nodes[j];
		_nodes[i].index = i;
	}
}

MedianFilter::~MedianFilter() {}

void MedianFilter::init(uint8_t sliding_window_size) {
	// create linked list
	for (uint8_t i = 0; i < sliding_window_size - 1; ++i) {
	}
}

/**
 * Use an insert operation that has some state information.
 */
void MedianFilter::sorted_insert(value_id_t value_id, value_t value, bool& start) {
	value_t pvalue = _buffer.getValue(buffer_id, channel_id, _nodes[0].index);
	if (value > pvalue) {
		value_id_t pvalue_id = _nodes[0].index;
	}
	// we can have a hypothesis about where to insert the next element because we expect a sine wave
	// if the sine wave is descending the next item should be inserted in the front
	// if it is ascending the next item should be inserted in the back
	// if it is switching from descending to ascending the insert gradually shifts from front to back and other way
	// around
}

/**
 * Set buffer. We're assuming several things about this buffer:
 *   - Values before and after the buffer can be accessed as well and used as padding for the median filter.
 *   - The buffer is assumed
 */
void MedianFilter::setBuffer(InterleavedBuffer* buffer) {
	_buffer = buffer;
}

void MedianFilter::setPadding(value_id_t padding) {
	assert(padding > 0, "Padding should be larger than 0");
	_padding = padding;
}

void MedianFilter::run(buffer_id_t buffer_id, channel_id_t channel_id) {
	_buffer.getBuffer(buffer_id);

	bool start = true;
	value_t value;
	for (value_id_t i = -_padding; i < 0; ++i) {
		value = _buffer.getValue(buffer_id, channel_id, i);
		sorted_insert(i, value, start)
	}

	for (value_id_t i = -_padding; i < _buffer.getBufferLength() - 1 + _padding; ++i) {
		_buffer.getValue(buffer_id, channel_id, i);
	}
}
