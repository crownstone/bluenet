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
#include <structs/cs_ScheduleEntriesAccessor.h>
#include <drivers/cs_Timer.h>

/**
 * File to test the state / storage.
 *
 * Since writing to flash is asynchronous, the tests consist of multiple steps.
 * Also, we need to check persistence, so the tests will continue after a reboot.
 * To make things a bit readable, the individual tests can be read from top to bottom, as that's the order of execution.
 */

enum TestResult {
	DONE,
	DONE_BUT_WAIT_FOR_WRITE,
	NOT_DONE_WAIT_FOR_WRITE,
};

class TestState : EventListener {
public:
	TestState() {
		EventDispatcher::getInstance().addListener(this);
		_state = &State::getInstance();
	}

	void assertSuccess(cs_ret_code_t retCode);
	void assertError(cs_ret_code_t retCode);
	uint16_t getResetCount();
	void setResetCount(uint16_t count);
	void waitForStore(CS_TYPE type);
	void continueTest();
	void handleEvent(event_t & event);

	TestResult getAndSetUint16(uint16_t resetCount, uint32_t step);
	TestResult getAndSetInvalid(uint16_t resetCount, uint32_t step);
	TestResult getAndSetLargeStruct(uint16_t resetCount, uint32_t step);

private:
	uint16_t _resetCount;
	uint32_t _test = 0;
	uint32_t _testStep = 0;

	volatile CS_TYPE _lastStoredType = CS_TYPE::CONFIG_DO_NOT_USE;
	State *_state;
};

int main() {
	// Mostly copied from crownstone main.
	// this enabled the hard float, without it, we get a hardfault
	SCB->CPACR |= (3UL << 20) | (3UL << 22); __DSB(); __ISB();

//	atexit(on_exit);

	NRF_LOG_INIT(NULL);
	NRF_LOG_DEFAULT_BACKENDS_INIT();

	uint32_t errCode;
	boards_config_t board = {};
	errCode = configure_board(&board);
	APP_ERROR_CHECK(errCode);

	serial_config(board.pinGpioRx, board.pinGpioTx);
	serial_init(SERIAL_ENABLE_RX_AND_TX);
	_log(SERIAL_INFO, SERIAL_CRLF);
	LOGi("");
	LOGi("##### Main #####");

	// from crownstone constructor
	EventDispatcher::getInstance();
	Stack *stack = &Stack::getInstance();
	Timer *timer = &Timer::getInstance();
	Storage *storage = &Storage::getInstance();
//	State *state = &State::getInstance();
	__attribute__((unused)) TestState test;

	// from crownstone init
	LOGi("Init drivers");
	stack->init();
	timer->init();
	stack->initSoftdevice();
	storage->init();
	// Code continues at storage initialized event.

	while (1) {
		app_sched_execute();
		sd_app_evt_wait();
		NRF_LOG_FLUSH();
	}
}

void TestState::waitForStore(CS_TYPE type) {
	LOGi("waitForStore type=%u", type);
	_lastStoredType = type;
}

/**
 * This is where the tests get called.
 *
 * Keep up at which test we are, and the step within that test.
 */
void TestState::continueTest() {
	TestResult result = DONE;
	switch (_test) {
		case 0:
			result = getAndSetInvalid(_resetCount, _testStep);
			break;
		case 1:
			result = getAndSetUint16(_resetCount, _testStep);
			break;
		case 2:
			result = getAndSetLargeStruct(_resetCount, _testStep);
			break;
		default:
			if (_resetCount == 0) {
				LOGi("");
				LOGi("##### Reboot #####");
				sd_nvic_SystemReset();
				return;
			}
			LOGi("");
			LOGi("##### Tests complete! #####");
			return;
	}
	switch (result) {
	case DONE_BUT_WAIT_FOR_WRITE:
		_test++;
		_testStep = 0;
		break;
	case DONE:
		_test++;
		_testStep = 0;
		continueTest();
		break;
	case NOT_DONE_WAIT_FOR_WRITE:
		_testStep++;
		break;
	}
}

void TestState::handleEvent(event_t & event) {
	switch (event.type) {
	case CS_TYPE::EVT_STORAGE_INITIALIZED: {
		LOGi("storage initialized");
		boards_config_t board = {};
		uint32_t errCode = configure_board(&board);
		APP_ERROR_CHECK(errCode);
		State::getInstance().init(&board);

		LOGi("");
		LOGi("##### Start tests #####");
		_resetCount = getResetCount();
		setResetCount(_resetCount + 1);
		// Wait for reset count to be set, code continues at storage write done event.
		break;
	}
	case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
		CS_TYPE storedType = *(CS_TYPE*)event.data;
		LOGi("lastStoredType=%u storedType=%u", _lastStoredType, storedType);
		assert(_lastStoredType == storedType, "Wrong type written");
		_lastStoredType = CS_TYPE::CONFIG_DO_NOT_USE;
		continueTest();
		break;
	}
	default:
		break;
	}
}

void TestState::assertSuccess(cs_ret_code_t retCode) {
	assert(retCode == ERR_SUCCESS, "Expected ERR_SUCCESS");
}

void TestState::assertError(cs_ret_code_t retCode) {
	assert(retCode != ERR_SUCCESS, "Expected error");
}

uint16_t TestState::getResetCount() {
	LOGi("##### Get reset count #####");
	cs_ret_code_t errCode;
	TYPIFY(STATE_RESET_COUNTER) resetCount;
	LOGi("get reset count");
	errCode = _state->get(CS_TYPE::STATE_RESET_COUNTER, &resetCount, sizeof(resetCount));
	LOGi("errCode=%u resetCount=%u", errCode, resetCount);
	assertSuccess(errCode);
	return resetCount;
}

void TestState::setResetCount(uint16_t count) {
	LOGi("##### Set reset count #####");
	cs_ret_code_t errCode;
	TYPIFY(STATE_RESET_COUNTER) resetCount = count;
	LOGi("set reset count to %u", resetCount);
	errCode = _state->set(CS_TYPE::STATE_RESET_COUNTER, &resetCount, sizeof(resetCount));
	LOGi("errCode=%u", errCode);
	assertSuccess(errCode);
	waitForStore(CS_TYPE::STATE_RESET_COUNTER);
}

TestResult TestState::getAndSetUint16(uint16_t resetCount, uint32_t step) {
	if (step == 0) {
		LOGi("");
		LOGi("##### Get and set uint16 #####");
	}
	cs_ret_code_t errCode;
	TYPIFY(CONFIG_BOOT_DELAY) bootDelay;

	if (resetCount == 0) {
		switch (step) {
		case 0:{
			LOGi("## get default boot delay");
			errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
			LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);
			assertSuccess(errCode);
			assert(bootDelay == CONFIG_BOOT_DELAY_DEFAULT, "Expected CONFIG_BOOT_DELAY_DEFAULT");

			bootDelay = 1234;
			LOGi("## set boot delay to %u", bootDelay);
			errCode = _state->set(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
			LOGi("errCode=%u", errCode);
			assertSuccess(errCode);

			LOGi("## get boot delay before write is done");
			errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
			LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);
			assertSuccess(errCode);
			assert(bootDelay == 1234, "Expected 1234");

			waitForStore(CS_TYPE::CONFIG_BOOT_DELAY);
			return NOT_DONE_WAIT_FOR_WRITE;
		}
		case 1: {
			LOGi("## get bootDelay after write is done");
			errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
			LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);
			assertSuccess(errCode);
			assert(bootDelay == 1234, "Expected 1234");

			bootDelay = 65432;
			LOGi("## set boot delay to %u", bootDelay);
			errCode = _state->set(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
			LOGi("errCode=%u", errCode);
			assertSuccess(errCode);

			waitForStore(CS_TYPE::CONFIG_BOOT_DELAY);
			return NOT_DONE_WAIT_FOR_WRITE;
		}
		case 2: {
			LOGi("## get boot delay after write is done");
			errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
			LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);
			assertSuccess(errCode);
			assert(bootDelay == 65432, "Expected 65432");
			return DONE;
		}
		}
	}
	else {
		LOGi("## get boot delay after reset");
		errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
		LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);
		assertSuccess(errCode);
		assert(bootDelay == 65432, "Expected 65432");
		return DONE;
	}
	return DONE;
}

TestResult TestState::getAndSetInvalid(uint16_t resetCount, uint32_t step) {
	if (step == 0) {
		LOGi("");
		LOGi("##### Get and set invalid types and sizes #####");
	}
	cs_ret_code_t errCode;

	if (resetCount == 0) {
		uint8_t bootDelay;

		LOGi("## get boot delay with wrong size");
		errCode = _state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
		LOGi("errCode=%u bootDelay=%u", errCode, bootDelay);
		assertError(errCode);

		LOGi("## set boot delay with wrong size");
		bootDelay = 200;
		errCode = _state->set(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
		LOGi("errCode=%u", errCode);
		assertError(errCode);

		uint32_t val;
		LOGi("## get brownout");
		errCode = _state->get(CS_TYPE::EVT_BROWNOUT_IMPENDING, &val, sizeof(val));
		LOGi("errCode=%u val=%u", errCode, val);
		assertError(errCode);

		LOGi("## set brownout");
		val = 200;
		errCode = _state->set(CS_TYPE::EVT_BROWNOUT_IMPENDING, &val, sizeof(val));
		LOGi("errCode=%u", errCode);
		assertError(errCode);
	}

	return DONE;
}

TestResult TestState::getAndSetLargeStruct(uint16_t resetCount, uint32_t step) {
	if (step == 0) {
		LOGi("");
		LOGi("##### Get and set a large struct #####");
	}
	cs_ret_code_t errCode;
	bool printSchedules = false;

	ScheduleList scheduleAccessor;
	TYPIFY(STATE_SCHEDULE) schedule;
	scheduleAccessor.assign((uint8_t*)&schedule, sizeof(schedule));

	if (resetCount == 0) {
		switch (step) {
		case 0: {
			LOGi("## get default schedule");
			errCode = _state->get(CS_TYPE::STATE_SCHEDULE, &schedule, sizeof(schedule));
			LOGi("errCode=%u", errCode);
			assertSuccess(errCode);
			if (printSchedules) scheduleAccessor.print();

			schedule.list[3].nextTimestamp = 1234567;
			LOGi("## set schedule");
			errCode = _state->set(CS_TYPE::STATE_SCHEDULE, &schedule, sizeof(schedule));
			LOGi("errCode=%u", errCode);
			assertSuccess(errCode);

			waitForStore(CS_TYPE::STATE_SCHEDULE);
			return NOT_DONE_WAIT_FOR_WRITE;
		}
		case 1: {
			LOGi("## get schedule after write is done");
			errCode = _state->get(CS_TYPE::STATE_SCHEDULE, &schedule, sizeof(schedule));
			LOGi("errCode=%u", errCode);
			assertSuccess(errCode);
			if (printSchedules) scheduleAccessor.print();
			assert(schedule.list[3].nextTimestamp == 1234567, "Expected 1234567");

			schedule.list[3].nextTimestamp = 7654321;
			LOGi("## set schedule");
			errCode = _state->set(CS_TYPE::STATE_SCHEDULE, &schedule, sizeof(schedule));
			LOGi("errCode=%u", errCode);
			assertSuccess(errCode);

			waitForStore(CS_TYPE::STATE_SCHEDULE);
			return NOT_DONE_WAIT_FOR_WRITE;
		}
		case 2:{
			LOGi("## get schedule after write is done");
			errCode = _state->get(CS_TYPE::STATE_SCHEDULE, &schedule, sizeof(schedule));
			LOGi("errCode=%u", errCode);
			assertSuccess(errCode);
			if (printSchedules) scheduleAccessor.print();
			assert(schedule.list[3].nextTimestamp == 7654321, "Expected 7654321");

			return DONE;
		}
		}
	}
	else {
		LOGi("## get schedule after reset");
		errCode = _state->get(CS_TYPE::STATE_SCHEDULE, &schedule, sizeof(schedule));
		LOGi("errCode=%u", errCode);
		assertSuccess(errCode);
		if (printSchedules) scheduleAccessor.print();
		assert(schedule.list[3].nextTimestamp == 7654321, "Expected 7654321");
	}
	return DONE;
}


