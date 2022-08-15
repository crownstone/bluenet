/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 25, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Stack.h>
#include <cfg/cs_Boards.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <storage/cs_State.h>
#include <structs/cs_ScheduleEntriesAccessor.h>
#include <util/cs_BleError.h>

/**
 * File to test the state / storage.
 *
 * Since writing to flash is asynchronous, the tests consist of multiple steps.
 * Also, we need to check persistence, so the tests will continue after a reboot.
 * To make things a bit readable, the individual tests can be read from top to bottom, as that's the order of execution.
 */

#define LOGTestDebug LOGnone

// Don't know where to get this from nicely, but the NRF52 has a page size of 4kB.
//#define FLASH_PAGE_SIZE 4096
#define FLASH_PAGE_SIZE (4 * FDS_VIRTUAL_PAGE_SIZE)

enum TestResult {
	DONE,
	DONE_BUT_WAIT_FOR_WRITE,
	NOT_DONE_WAIT_FOR_WRITE,
};

class TestState : EventListener {
public:
	TestState() {
		EventDispatcher::getInstance().addListener(this);
		_state         = &State::getInstance();
		_tickTimerData = {{0}};
		_tickTimerId   = &_tickTimerData;
	}

	void assertSuccess(cs_ret_code_t retCode);
	void assertError(cs_ret_code_t retCode);
	uint16_t getResetCount();
	void setResetCount(uint16_t count);
	void waitForStore(CS_TYPE type);
	void continueTest();
	void handleEvent(event_t& event);
	static void staticTick(TestState* ptr) { ptr->tick(); }

	TestResult getAndSetUint16(uint16_t resetCount, uint32_t step);
	TestResult getAndSetInvalid(uint16_t resetCount, uint32_t step);
	TestResult getAndSetLargeStruct(uint16_t resetCount, uint32_t step);
	TestResult testGarbageCollection(uint16_t resetCount, uint32_t step);
	TestResult testMultipleWrites(uint16_t resetCount, uint32_t step);
	TestResult testDelayedSet(uint16_t resetCount, uint32_t step);
	TestResult testDuplicateRecords(uint16_t resetCount, uint32_t step);
	TestResult testCorruptedRecords(uint16_t resetCount, uint32_t step);

private:
	uint16_t _resetCount             = 0;
	uint32_t _test                   = 0;
	uint32_t _testStep               = 0;

	volatile CS_TYPE _lastStoredType = CS_TYPE::CONFIG_DO_NOT_USE;
	State* _state;

	app_timer_t _tickTimerData;
	app_timer_id_t _tickTimerId;

	void tick();
};

int main() {
	// Mostly copied from crownstone main.
	// this enabled the hard float, without it, we get a hardfault
	SCB->CPACR |= (3UL << 20) | (3UL << 22);
	__DSB();
	__ISB();

	//	atexit(on_exit);

	NRF_LOG_INIT(NULL);
	NRF_LOG_DEFAULT_BACKENDS_INIT();

	uint32_t errCode;
	boards_config_t board = {};
	errCode               = configure_board(&board);
	APP_ERROR_CHECK(errCode);

	serial_config(board.pinGpioRx, board.pinGpioTx);
	serial_init(SERIAL_ENABLE_RX_AND_TX);
	_log(SERIAL_INFO, SERIAL_CRLF);
	LOGi("");
	LOGi("##### Main #####");

	// from crownstone constructor
	EventDispatcher::getInstance();
	Stack* stack     = &Stack::getInstance();
	Timer* timer     = &Timer::getInstance();
	Storage* storage = &Storage::getInstance();
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
		LOG_FLUSH();
	}
}

void TestState::waitForStore(CS_TYPE type) {
	LOGTestDebug("waitForStore type=%u", type);
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
		case 0: result = getAndSetInvalid(_resetCount, _testStep); break;
		case 1: result = getAndSetUint16(_resetCount, _testStep); break;
		case 2: result = getAndSetLargeStruct(_resetCount, _testStep); break;
		case 3: result = testGarbageCollection(_resetCount, _testStep); break;
		case 4: result = testMultipleWrites(_resetCount, _testStep); break;
		case 5: result = testDelayedSet(_resetCount, _testStep); break;
		case 6: result = testDuplicateRecords(_resetCount, _testStep); break;
		case 7: result = testCorruptedRecords(_resetCount, _testStep); break;
		default:
			if (_resetCount == 0) {
				LOGi("");
				LOGi("##### Reboot #####");
				sd_nvic_SystemReset();
				return;
			}
			LOGi("");
			LOGi("##### All tests successful! #####");
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
		case NOT_DONE_WAIT_FOR_WRITE: _testStep++; break;
	}
}

void TestState::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_STORAGE_INITIALIZED: {
			LOGi("storage initialized");
			boards_config_t board = {};
			uint32_t errCode      = configure_board(&board);
			APP_ERROR_CHECK(errCode);
			State::getInstance().init(&board);
			State::getInstance().startWritesToFlash();

			Timer::getInstance().createSingleShot(_tickTimerId, (app_timer_timeout_handler_t)TestState::staticTick);
			Timer::getInstance().start(_tickTimerId, MS_TO_TICKS(TICK_INTERVAL_MS), this);

			LOGi("");
			LOGi("##### Start tests #####");
			_resetCount = getResetCount();
			setResetCount(_resetCount + 1);
			// Wait for reset count to be set, code continues at storage write done event.
			break;
		}
		case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
			CS_TYPE storedType = *(CS_TYPE*)event.data;
			LOGTestDebug("lastStoredType=%u storedType=%u", _lastStoredType, storedType);
			assert(_lastStoredType == storedType, "Wrong type written");
			_lastStoredType = CS_TYPE::CONFIG_DO_NOT_USE;
			continueTest();
			break;
		}
		default: break;
	}
}

void TestState::tick() {
	event_t event(CS_TYPE::EVT_TICK);
	EventDispatcher::getInstance().dispatch(event);
	Timer::getInstance().start(_tickTimerId, MS_TO_TICKS(TICK_INTERVAL_MS), this);
}

void TestState::assertSuccess(cs_ret_code_t retCode) {
	if (retCode != ERR_SUCCESS) {
		LOGe("errCode=%u", retCode);
	}
	assert(retCode == ERR_SUCCESS, "Expected ERR_SUCCESS");
}

void TestState::assertError(cs_ret_code_t retCode) {
	if (retCode == ERR_SUCCESS) {
		LOGe("errCode=%u", retCode);
	}
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
			case 0: {
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
		errCode   = _state->set(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
		LOGi("errCode=%u", errCode);
		assertError(errCode);

		uint32_t val;
		LOGi("## get brownout");
		errCode = _state->get(CS_TYPE::EVT_BROWNOUT_IMPENDING, &val, sizeof(val));
		LOGi("errCode=%u val=%u", errCode, val);
		assertError(errCode);

		LOGi("## set brownout");
		val     = 200;
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
			case 2: {
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

TestResult TestState::testGarbageCollection(uint16_t resetCount, uint32_t step) {
	// Let's do this test only once, after reboot.
	if (resetCount != 1) {
		return DONE;
	}
	if (step == 0) {
		LOGi("");
		LOGi("##### Test garbage collection #####");
	}
	cs_ret_code_t errCode;
	TYPIFY(CONFIG_CURRENT_ADC_ZERO) currentZero = step;

	uint32_t numStepsToFillPage                 = FLASH_PAGE_SIZE / (sizeof(fds_header_t) + sizeof(currentZero));
	if (step < numStepsToFillPage) {
		errCode = _state->set(CS_TYPE::CONFIG_CURRENT_ADC_ZERO, &currentZero, sizeof(currentZero));
		assertSuccess(errCode);
		waitForStore(CS_TYPE::CONFIG_CURRENT_ADC_ZERO);
		return NOT_DONE_WAIT_FOR_WRITE;
	}
	return DONE;
}

TestResult TestState::testMultipleWrites(uint16_t resetCount, uint32_t step) {
	// Let's do this test only once, after reboot.
	if (resetCount != 1) {
		return DONE;
	}
	if (step == 0) {
		LOGi("");
		LOGi("##### Test multiple writes without wait #####");
	}
	cs_ret_code_t errCode;
	TYPIFY(CONFIG_CURRENT_ADC_ZERO) currentZero = 0;

	switch (step) {
		case 0: {
			uint32_t numSteps = 10;
			for (uint32_t i = 0; i < numSteps; ++i) {
				currentZero = i;
				errCode     = _state->set(CS_TYPE::CONFIG_CURRENT_ADC_ZERO, &currentZero, sizeof(currentZero));
				assertSuccess(errCode);
			}
			waitForStore(CS_TYPE::CONFIG_CURRENT_ADC_ZERO);
			return NOT_DONE_WAIT_FOR_WRITE;
		}
		case 1: {
			// Another write done event is expected.
			waitForStore(CS_TYPE::CONFIG_CURRENT_ADC_ZERO);
			return DONE_BUT_WAIT_FOR_WRITE;
		}
	}
	return DONE;
}

TestResult TestState::testDelayedSet(uint16_t resetCount, uint32_t step) {
	if (step == 0) {
		LOGi("");
		LOGi("##### Test delayed set #####");
	}
	cs_ret_code_t errCode;
	TYPIFY(CONFIG_VOLTAGE_ADC_ZERO) voltageZero = 0;
	if (resetCount == 0) {
		cs_ret_code_t errCode;
		voltageZero = -1234;
		cs_state_data_t data(CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO, (uint8_t*)&voltageZero, sizeof(voltageZero));
		errCode = _state->setDelayed(data, 3);
		assertSuccess(errCode);
		waitForStore(CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO);
		return DONE_BUT_WAIT_FOR_WRITE;
	}
	else {
		errCode = _state->get(CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO, (uint8_t*)&voltageZero, sizeof(voltageZero));
		LOGi("errCode=%u voltageZero=%i", errCode, voltageZero);
		assertSuccess(errCode);
		assert(voltageZero == -1234, "Expected -1234");
		return DONE;
	}
}

/**
 * First write the same type multiple times to FDS. Uses raw FDS functions in order to do so.
 * Then set via state, and get after a reboot.
 *
 * Use a word sized state variable for this so we don't need any padding.
 * Allocate a data pointer, as a variable on stack will not last long enough for the write action, and result in CRC
 * failures. The allocation leaks some memory, but we don't care about this for this test.
 */
TestResult TestState::testDuplicateRecords(uint16_t resetCount, uint32_t step) {
	if (step == 0) {
		LOGi("");
		LOGi("##### Test duplicate records #####");
	}
	cs_ret_code_t errCode;

	uint32_t pwmPeriod = 0;
	fds_record_t record;
	if (resetCount == 0) {
		switch (step) {
			case 0:
			case 1:
			case 2: {
				uint32_t* data           = (uint32_t*)malloc(sizeof(pwmPeriod));
				pwmPeriod                = 10000 + step;
				*data                    = pwmPeriod;
				record.file_id           = FILE_CONFIGURATION;
				record.key               = to_underlying_type(CS_TYPE::CONFIG_PWM_PERIOD);
				record.data.p_data       = data;
				record.data.length_words = 1;
				uint32_t fdsRet          = fds_record_write(NULL, &record);
				assert(fdsRet == NRF_SUCCESS, "Expected NRF_SUCCESS");
				waitForStore(CS_TYPE::CONFIG_PWM_PERIOD);
				return NOT_DONE_WAIT_FOR_WRITE;
			}
			case 3: {
				errCode = _state->get(CS_TYPE::CONFIG_PWM_PERIOD, &pwmPeriod, sizeof(pwmPeriod));
				LOGi("errCode=%u pwmPeriod=%u", errCode, pwmPeriod);
				assertSuccess(errCode);
				assert(pwmPeriod == 10002, "Expected 10002");

				pwmPeriod = 1234567890;
				errCode   = _state->set(CS_TYPE::CONFIG_PWM_PERIOD, &pwmPeriod, sizeof(pwmPeriod));
				assertSuccess(errCode);
				waitForStore(CS_TYPE::CONFIG_PWM_PERIOD);
				return DONE_BUT_WAIT_FOR_WRITE;
			}
		}
	}
	else {
		errCode = _state->get(CS_TYPE::CONFIG_PWM_PERIOD, &pwmPeriod, sizeof(pwmPeriod));
		LOGi("errCode=%u pwmPeriod=%u", errCode, pwmPeriod);
		assertSuccess(errCode);
		assert(pwmPeriod == 1234567890, "Expected 1234567890");
		return DONE;
	}
	return DONE;
}

/**
 * Corrupt a record by changing the value in ram while FDS is writing it to flash.
 * This hopefully leads to corrupt data in flash, and should lead to a CRC mismatch.
 *
 * Use a word sized state variable for this so we don't need any padding.
 * Allocate a data pointer, variable on stack will not last long enough for the write action.
 * The allocation leaks some memory, but we don't care about this for this test.
 */
TestResult TestState::testCorruptedRecords(uint16_t resetCount, uint32_t step) {
	if (step == 0) {
		LOGi("");
		LOGi("##### Test corrupted records #####");
	}
	cs_ret_code_t errCode;
	int32_t powerZero  = 0;
	uint16_t recordKey = to_underlying_type(CS_TYPE::CONFIG_POWER_ZERO);
	fds_record_t record;
	if (resetCount == 0) {
		switch (step) {
			case 0: {
				int32_t* data = (int32_t*)malloc(sizeof(powerZero));
				LOGi("Write to flash");
				powerZero                = 123456;
				*data                    = powerZero;
				record.file_id           = FILE_CONFIGURATION;
				record.key               = recordKey;
				record.data.p_data       = data;
				record.data.length_words = 1;
				uint32_t fdsRet          = fds_record_write(NULL, &record);
				// Modify data while FDS is writing.
				for (uint32_t i = 0; i < 100000; ++i) {
					*data = i;
				}
				assert(fdsRet == NRF_SUCCESS, "Expected NRF_SUCCESS");
				waitForStore(CS_TYPE::CONFIG_POWER_ZERO);
				return NOT_DONE_WAIT_FOR_WRITE;
			}
			case 1: {
				LOGi("Read from flash");
				fds_flash_record_t flash_record;
				fds_record_desc_t record_desc;
				fds_find_token_t ftok;
				memset(&ftok, 0x00, sizeof(fds_find_token_t));
				uint32_t fdsRet = fds_record_find(FILE_CONFIGURATION, recordKey, &record_desc, &ftok);
				assert(fdsRet == NRF_SUCCESS, "Expected NRF_SUCCESS");
				fdsRet = fds_record_open(&record_desc, &flash_record);
				assert(fdsRet == FDS_ERR_CRC_CHECK_FAILED, "Expected FDS_ERR_CRC_CHECK_FAILED");

				LOGi("Get from state");
				powerZero = 0xDEADBEEF;
				errCode   = _state->get(CS_TYPE::CONFIG_POWER_ZERO, &powerZero, sizeof(powerZero));
				LOGi("errCode=%u powerZero=%u", errCode, powerZero);
				assertSuccess(errCode);
				assert(powerZero == CONFIG_POWER_ZERO_DEFAULT, "Expected default");

				return DONE;
			}
		}
	}
	else {
		return DONE;
	}
	return DONE;
}
