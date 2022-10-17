#include <boards/cs_HostBoardFullyFeatured.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>

template <class T>
bool checkStateSetGet(State& state, T inval, CS_TYPE type, int line) {
	cs_ret_code_t retval;
	T outval = {};
	retval   = state.set(type, &inval, sizeof(T));

	if (retval != ERR_SUCCESS) {
		LOGw("[line %u]failed to set state");
		return false;
	}

	retval = state.get(type, &outval, sizeof(T));

	if (retval != ERR_SUCCESS) {
		LOGw("failed to get state");
		return false;
	}

	if (outval != inval) {
		LOGw("set-get didn't return same value");
		return false;
	}

	return true;
}

int main() {
	EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();
	Storage& storage                  = Storage::getInstance();
	State& state                      = State::getInstance();

	boards_config_t board;
	init(&board);
	asHostFullyFeatured(&board);

	storage.init();
	state.init(&board);

	uint8_t stateDimmingAllowed = 1;
	if (!checkStateSetGet(state, stateDimmingAllowed, CS_TYPE::CONFIG_DIMMING_ALLOWED, __LINE__)) {
		return 1;
	}

	uint8_t stateSwitchLocked = 0;
	if (!checkStateSetGet(state, stateSwitchLocked, CS_TYPE::CONFIG_SWITCH_LOCKED, __LINE__)) {
		return 1;
	}

	uint8_t stateSwitchState = 0;
	if (!checkStateSetGet(state, stateSwitchState, CS_TYPE::STATE_SWITCH_STATE, __LINE__)) {
		return 1;
	}

	return 0;
}
