#include <protocol/mesh/cs_MeshMessageState.h>

state_message::state_message(state_message_t & smt): _state_message_t(smt) {
}

state_message::iterator state_message::begin() {
	auto index = (_state_message_t.head + MAX_STATE_ITEMS) % MAX_STATE_ITEMS;
	return state_message::iterator(&_state_message_t, index);
}

state_message::iterator state_message::end() {
	auto index = _state_message_t.head + _state_message_t.size;
	return state_message::iterator(&_state_message_t, index);
}
		
void state_message::push_back(state_item_t item) {
	// decide what to do if too many items are pushed in the buffer
	// 1.) write over previous items and also update head
	// 2.) reject items until they get popped
	_state_message_t.list[_state_message_t.tail] = item;
	_state_message_t.tail = (_state_message_t.tail + 1) % MAX_STATE_ITEMS;

	if (_state_message_t.size < MAX_STATE_ITEMS)
		_state_message_t.size++;

	_state_message_t.head = (_state_message_t.tail + _state_message_t.size) % MAX_STATE_ITEMS;
}

void state_message::clear() {
	_state_message_t.head = 0;
	_state_message_t.tail = 0;
	_state_message_t.size = 0;
	for (uint32_t i = 0; i < MAX_STATE_ITEMS; ++i) {
		_state_message_t.list[i] = {0,0,0,0};
	}
}
