/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 25, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <storage/cs_State.h>
#include <cfg/cs_Boards.h>
#include <util/cs_BleError.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <ble/cs_Stack.h>

class TestState : EventListener {
public:
	TestState() {
		EventDispatcher::getInstance().addListener(this);
		_state = &State::getInstance();
	}

	void getSet();
	void waitForStore(CS_TYPE type);
	void handleEvent(event_t & event);
private:
	CS_TYPE _lastStoredType = CS_TYPE::CONFIG_DO_NOT_USE;
	State *_state;
};

int main() {
	// from crownstone main
	SCB->CPACR |= (3UL << 20) | (3UL << 22); __DSB(); __ISB();

//	atexit(on_exit);

	NRF_LOG_INIT(NULL);
	NRF_LOG_DEFAULT_BACKENDS_INIT();
	NRF_LOG_INFO("Main");
	NRF_LOG_FLUSH();

	uint32_t errCode;
	boards_config_t board = {};
	errCode = configure_board(&board);
	APP_ERROR_CHECK(errCode);

	serial_config(board.pinGpioRx, board.pinGpioTx);
	serial_init(SERIAL_ENABLE_RX_AND_TX);
	_log(SERIAL_INFO, SERIAL_CRLF);
	LOGi("##### Main #####");

	// from crownstone constructor
	EventDispatcher::getInstance();
	Stack *stack = &Stack::getInstance();
	Timer *timer = &Timer::getInstance();
	Storage *storage = &Storage::getInstance();
	State *state = &State::getInstance();

	// from crownstone init
	LOGi("Init drivers");
	stack->init();
	timer->init();
	stack->initSoftdevice();
	storage->init();

	// Uhh, wait for storage to be initialized??
	NRF_LOG_FLUSH();

	state->init(&board);


	LOGi("##### Tests #####");
	TestState test;
	test.getSet();


	NRF_LOG_FLUSH();
}

void TestState::waitForStore(CS_TYPE type) {
	_lastStoredType = type;
	while (1) {
		app_sched_execute();
		sd_app_evt_wait();
		NRF_LOG_FLUSH();
		if (_lastStoredType == CS_TYPE::CONFIG_DO_NOT_USE) {
			break;
		}
	}
}

void TestState::handleEvent(event_t & event) {
	switch (event.type) {
	case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
		CS_TYPE storedType = *(CS_TYPE*)event.data;
		if (_lastStoredType == storedType) {
			_lastStoredType = CS_TYPE::CONFIG_DO_NOT_USE;
		}
		break;
	}
	default:
		break;
	}
}

void TestState::getSet() {
	uint32_t errCode;

	// Get and set a state var that is smaller than 4 bytes.
	TYPIFY(CONFIG_BOOT_DELAY) bootDelay;
	LOGi("get bootDelay");
	errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
	LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);

	bootDelay = 1234;
	LOGi("set boot delay to %u", bootDelay);
	errCode = _state->set(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
	LOGi("errCode=%u", errCode);

	LOGi("get bootDelay before write is done");
	errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
	LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);

	waitForStore(CS_TYPE::CONFIG_BOOT_DELAY);

	LOGi("get bootDelay after write is done");
	errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
	LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);
}
