/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 20 Jan., 2015
 * License: LGPLv3+, Apache, or MIT, your choice
 */

#include <mesh/cs_Mesh.h>

#include <protocol/rbc_mesh_common.h>
#include <protocol/version_handler.h>

#include <cfg/cs_Boards.h>

#include <drivers/cs_Serial.h>
#include <mesh/cs_MeshControl.h>
#include <protocol/cs_MeshMessageTypes.h>
#include <util/cs_BleError.h>

#include <util/cs_Utils.h>
#include <cfg/cs_Config.h>

#include <drivers/cs_RTC.h>

#include <storage/cs_Settings.h>
#include <cfg/cs_Strings.h>

//#define PRINT_MESH_VERBOSE

void start_stop_mesh(void * p_event_data, uint16_t event_size) {
	if (*(bool*)p_event_data) {
		LOGi(FMT_START, "Mesh");
		uint32_t err_code = rbc_mesh_start();
		if (err_code != NRF_SUCCESS) {
			LOGe(FMT_FAILED_TO, "start mesh", err_code);
		}
	} else {
		LOGi(FMT_STOP, "Mesh");
		uint32_t err_code = rbc_mesh_stop();
		if (err_code != NRF_SUCCESS) {
			LOGe(FMT_FAILED_TO, "stop mesh", err_code);
		}
	}
}


Mesh::Mesh() :
#if (NORDIC_SDK_VERSION >= 11)
		_appTimerData({ {0}}),
		_appTimerId(NULL),
#else
		_appTimerId(UINT32_MAX),
#endif
	_started(false)

{
#if (NORDIC_SDK_VERSION >= 11)
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;
#endif
}

Mesh::~Mesh() {

}

void Mesh::start() {
	_mesh_start_time = RTC::now();
	if (!_started) {
		_started = true;
#ifdef PRINT_MESH_VERBOSE
		LOGi("start mesh");
#endif
		startTicking();
		app_sched_event_put(&_started, sizeof(_started), start_stop_mesh);
	}
}

void Mesh::stop() {
	if (_started) {
		_started = false;
#ifdef PRINT_MESH_VERBOSE
		LOGi("stop mesh");
#endif
		stopTicking();
		app_sched_event_put(&_started, sizeof(_started), start_stop_mesh);
	}
}

void Mesh::restart() {
	if (_started) {
		uint32_t err_code = rbc_mesh_restart();
		if (err_code != NRF_SUCCESS) {
			LOGe(FMT_FAILED_TO, "restart mesh", err_code);
		}
	}
}

void Mesh::tick() {
	checkForMessages();
	scheduleNextTick();
}

void Mesh::scheduleNextTick() {
//	LOGi("Mesh::scheduleNextTick");
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(MESH_UPDATE_FREQUENCY), this);
}

void Mesh::startTicking() {
	Timer::getInstance().start(_appTimerId, APP_TIMER_TICKS(1, APP_TIMER_PRESCALER), this);
}

void Mesh::stopTicking() {
	Timer::getInstance().stop(_appTimerId);
}


#if (NORDIC_SDK_VERSION >= 11) 
const nrf_clock_lf_cfg_t Mesh::meshClockSource = {  .source        = NRF_CLOCK_LF_SRC_RC,   \
                                                    .rc_ctiv       = 16,                    \
                                                    .rc_temp_ctiv  = 2,                     \
                                                    .xtal_accuracy = 0};
#endif

/**
 * Function to test mesh functionality. We have to figure out if we have to enable the radio first, and that kind of
 * thing.
 */
void Mesh::init() {
	MeshControl::getInstance();

	//nrf_gpio_pin_clear(PIN_GPIO_LED0);
	LOGi(FMT_INIT, "Mesh");

	// setup the timer
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Mesh::staticTick);

	rbc_mesh_init_params_t init_params;

	// the access address is a configurable parameter so we get it from the settings.
	Settings::getInstance().get(CONFIG_MESH_ACCESS_ADDRESS, &init_params.access_addr);
	init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
	init_params.channel = MESH_CHANNEL;
#if (NORDIC_SDK_VERSION >= 11) 
	init_params.lfclksrc = meshClockSource;
#else
	init_params.lfclksrc = MESH_CLOCK_SOURCE;
#endif

	uint8_t error_code;
	//! checks if softdevice is enabled etc.
	error_code = rbc_mesh_init(init_params);
	APP_ERROR_CHECK(error_code);

	for (int i = 0; i < MESH_NUM_HANDLES; ++i) {
		error_code = rbc_mesh_value_enable(i+1);
		APP_ERROR_CHECK(error_code);
		_first[i] = true;
	}
//	error_code = rbc_mesh_value_enable(1);
//	APP_ERROR_CHECK(error_code);
//	error_code = rbc_mesh_value_enable(2);
//	APP_ERROR_CHECK(error_code);

	_mesh_start_time = RTC::now();

	// do not automatically start meshing, wait for the start command
//	rbc_mesh_stop();

//	if (!Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
//		LOGi("mesh not enabled");
		rbc_mesh_stop();
		// [16.06.16] need to execute app scheduler, otherwise pstorage
		// events will get lost ... maybe need to check why that actually happens??
		app_sched_execute();
//	}

}

void Mesh::send(uint8_t handle, void* p_data, uint8_t length) {
	assert(length <= MAX_MESH_MESSAGE_LEN, STR_ERR_VALUE_TOO_LONG);

//	LOGd("send ch: %d, len: %d", handle, length);
	//BLEutil::printArray((uint8_t*)p_data, length);
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		APP_ERROR_CHECK(rbc_mesh_value_set(handle, (uint8_t*)p_data, length));
	}
}

bool Mesh::getLastMessage(uint8_t channel, void** p_data, uint16_t& length) {
	assert(length <= MAX_MESH_MESSAGE_LEN, STR_ERR_VALUE_TOO_LONG);

	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		APP_ERROR_CHECK(rbc_mesh_value_get(channel, (uint8_t*)*p_data, &length));
	}
	//LOGi("recv ch: %d, len: %d", handle, length);
	return length != 0;
}

void Mesh::handleMeshMessage(rbc_mesh_event_t* evt)
{
	TICK_PIN(28);
//	nrf_gpio_gitpin_toggle(PIN_GPIO_LED1);

#ifdef PRINT_MESH_VERBOSE
	switch (evt->event_type)
	{
	case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:
		LOGd("ch: %d, conflicting value", evt->value_handle);
		break;
	case RBC_MESH_EVENT_TYPE_NEW_VAL:
		LOGd("ch: %d, new value", evt->value_handle);
		break;
	case RBC_MESH_EVENT_TYPE_UPDATE_VAL:
//		LOGd("ch: %d, update value", evt->value_handle);
		break;
	case RBC_MESH_EVENT_TYPE_INITIALIZED:
		LOGd("ch: %d, initialized", evt->value_handle);
		break;
	case RBC_MESH_EVENT_TYPE_TX:
		LOGd("ch: %d, transmitted", evt->value_handle);
		break;
	}
#endif

	if (evt->value_handle > MESH_NUM_HANDLES) {
//	if (evt->value_handle != 1 && evt->value_handle != 2) {
//	if (evt->value_handle == 1 || evt->value_handle > 4) {
		rbc_mesh_value_disable(evt->value_handle);
	}

	switch (evt->event_type)
	{
//		case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:
		case RBC_MESH_EVENT_TYPE_NEW_VAL:
		case RBC_MESH_EVENT_TYPE_UPDATE_VAL: {

//            LOGi("Got data ch: %i, val: %i, len: %d, orig_addr:", evt->value_handle, evt->data[0], evt->data_len);
//            BLEutil::printArray(evt->originator_address.addr, 6);

            MeshControl &meshControl = MeshControl::getInstance();
            meshControl.process(evt->value_handle, evt->data, evt->data_len);

//			led_config(evt->value_handle, evt->data[0]);
            break;
        }
        default:
//            LOGi("Default: %i", evt->event_type);
            break;
	}
}

void Mesh::checkForMessages() {
	rbc_mesh_event_t evt;

	//! check if there are new messages
	while (rbc_mesh_event_get(&evt) == NRF_SUCCESS) {

		if (_first[evt.value_handle -1]) {
			_first[evt.value_handle -1] = false;

			//! ignore the first message received on each channel if it arrives within a certain
			//! time limit after boot up. This to prevent reading old command messages after a
			//! reboot
			uint32_t ts = RTC::now() - _mesh_start_time;
			if (ts < MESH_BOOT_TIME) {
#ifdef PRINT_MESH_VERBOSE
				LOGi("t: %d: ch: %d, skipping message within boot-up time!", ts, evt.value_handle);
#endif
				rbc_mesh_packet_release(evt.data);
				continue;
			} else {
#ifdef PRINT_MESH_VERBOSE
				LOGi("first received at: %d", ts);
#endif
			}
		}

		//! handle the message
		handleMeshMessage(&evt);

		//! then free associated memory
		rbc_mesh_packet_release(evt.data);
	}
}

