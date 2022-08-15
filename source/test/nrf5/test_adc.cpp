#include <drivers/cs_ADC.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>

#include "cfg/cs_Boards.h"

//#include "third/nrf/nrf_drv_config.h"

/**
 * Identified issue:
 *
 * The ADC does have two issues:
 *   1.) It seems to sample a single value from the other channel (or almost that value).
 *   2.) It seems to switch channels after a while.
 *
 * Solution directions:
 *   +:) Have this unit test call the functions in cs_ADC.h. It is not exhibiting 1 nor 2.
 *   +:) Have a lot of calculations going on. Does not make a difference, not 1 nor 2.
 *
 *   +:) With ordinary code, everything enabled. We get 1 every minute.
 *   +:) With ordinary code, disable setting characteristic value. Now we have fewer 1 (every 15+ minutes).
 *       Some kind of
 *   +:) With ordinary code, everything disabled but with RC clock. Now we have fewer 1 (every 10+ minutes).
 *       Interesting, RC does have a bad influence
 */

#define POWER_SAMPLING

#ifdef POWER_SAMPLING
#include <processing/cs_PowerSampling.h>
#endif

/*
const nrf_clock_lf_cfg_t defaultClockSource = {  .source        = NRF_CLOCK_LF_SRC_XTAL,                     \
												 .rc_ctiv       = 0,                                         \
												 .rc_temp_ctiv  = 0,                                         \
												 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
*/
static uint8_t buf_index;

void adc_test_callback(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum) {
	const int threshold = 20;
	for (int i = 0; i < size; i++) {
		if (i % 2 == 0) {
			if (buf[i] > threshold) {
				cs_write("buf: ");
				for (int j = 0; j < size; j++) {
					cs_write("%d ", buf[j]);
					if ((j + 1) % 30 == 0) cs_write("\r\n");
				}
				cs_write("\r\n");
			}
			assert(buf[i] <= threshold, "wrong pin A!");
		}
		else {
			if (buf[i] < threshold) {
				cs_write("buf: ");
				for (int j = 0; j < size; j++) {
					cs_write("%d ", buf[j]);
					if ((j + 1) % 30 == 0) cs_write("\r\n");
				}
				cs_write("\r\n");
			}
			assert(buf[i] >= threshold, "wrong pin B!");
		}
	}
#ifdef CALCULATE_A_LOT
	// calculate a lot
	double average = 0.0;
	for (int j = 0; j != 10000; j++) {
		double avg = 0.0;
		for (int i = 0; i < size; i++) {
			avg += buf[i];
		}
		avg /= size;
		average = avg;
	}
	LOGd("buf=%d size=%d bufCount=%d, avg=%f", buf, size, bufNum, average);
#else
	LOGd("buf=%d size=%d bufCount=%d", buf, size, bufNum);
#endif
	cs_write("%d %d %d ... %d\r\n", buf[0], buf[1], buf[2], buf[size - 1]);
	ADC::getInstance().releaseBuffer(buf_index);
}

int main() {
	// enabled hard float, without it, we get a hardfaults
	SCB->CPACR |= (3UL << 20) | (3UL << 22);
	__DSB();
	__ISB();

	uint32_t errCode;
	boards_config_t board = {};
	errCode               = configure_board(&board);
	APP_ERROR_CHECK(errCode);

	serial_config(board.pinGpioRx, board.pinGpioTx);
	serial_init(SERIAL_ENABLE_RX_AND_TX);

	LOGd("Test ADC in separate program");

	Timer::getInstance().init();

	uint8_t enabled;
	BLE_CALL(sd_softdevice_is_enabled, (&enabled));
	if (enabled) {
		LOGw("Softdevice running");
		BLE_CALL(sd_softdevice_disable, ());
	}

	//	nrf_clock_lf_cfg_t _clock_source = defaultClockSource;
	//	SOFTDEVICE_HANDLER_APPSH_INIT(&_clock_source, true);

#ifdef POWER_SAMPLING
	PowerSampling::getInstance().init(board);
	PowerSampling::getInstance().startSampling();
#else
	// ADC stuff
	// no pca10040 board available in config...
	// p_config->pinAinPwmTemp        = 0; // gpio2 something unused
	// p_config->pinAinCurrent        = 1; // gpio3 something unused
	// p_config->pinAinVoltage        = 2; // gpio4 something unused
	const pin_id_t pins[] = {0, 1};
	LOGd("Set ADC pins");
	ADC::getInstance().init(pins, 2);

	LOGd("Set ADC callback");

	// when actually enabled we have problem..
	// fault: due to app_sched_event_put on callback?
	ADC::getInstance().setDoneCallback(adc_test_callback);

	LOGd("Start ADC");

	ADC::getInstance().start();
#endif
	LOGd("Run");

	int i = 0;
	while (1) {

		app_sched_execute();

		BLE_CALL(sd_app_evt_wait, ());

		if (i++ % 10000 == 0) {
			LOGi("Nothing to do at t mod 1000");
		}
	}
}
