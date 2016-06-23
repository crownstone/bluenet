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

void start_stop_mesh(void * p_event_data, uint16_t event_size) {
	if (*(bool*)p_event_data) {
		LOGi("mesh start");
		rbc_mesh_start();
	} else {
		LOGi("mesh stop");
		rbc_mesh_stop();
	}
}


Mesh::Mesh() : _appTimerId(-1), started(false) {
	MeshControl::getInstance();
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Mesh::staticTick);
}

Mesh::~Mesh() {

}

void Mesh::start() {
	started = true;
	startTicking();
	app_sched_event_put(&started, sizeof(started), start_stop_mesh);
}

void Mesh::stop() {
	started = false;
	stopTicking();
	app_sched_event_put(&started, sizeof(started), start_stop_mesh);
}

void Mesh::restart() {
	rbc_mesh_restart();
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

/**
 * Function to test mesh functionality. We have to figure out if we have to enable the radio first, and that kind of
 * thing.
 */
void Mesh::init() {
//	nrf_gpio_pin_clear(PIN_GPIO_LED0);
	LOGi("Initializing mesh");

	rbc_mesh_init_params_t init_params;

	init_params.access_addr = MESH_ACCESS_ADDR;
	init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
	init_params.channel = MESH_CHANNEL;
	init_params.lfclksrc = MESH_CLOCK_SOURCE;

	uint8_t error_code;
	//! checks if softdevice is enabled etc.
	error_code = rbc_mesh_init(init_params);
	APP_ERROR_CHECK(error_code);

	for (int i = 0; i < MESH_NUM_OF_CHANNELS; ++i) {
		error_code = rbc_mesh_value_enable(i+1);
		APP_ERROR_CHECK(error_code);
		_first[i] = true;
	}
//	error_code = rbc_mesh_value_enable(1);
//	APP_ERROR_CHECK(error_code);
//	error_code = rbc_mesh_value_enable(2);
//	APP_ERROR_CHECK(error_code);

	_mesh_init_time = RTC::now();

	// do not automatically start meshing, wait for the start command
//	rbc_mesh_stop();

	if (!Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		LOGi("mesh not enabled");
		rbc_mesh_stop();
		// [16.06.16] need to execute app scheduler, otherwise pstorage
		// events will get lost ... maybe need to check why that actually happens??
		app_sched_execute();
	}

}

void Mesh::send(uint8_t handle, void* p_data, uint8_t length) {
	assert(length <= MAX_MESH_MESSAGE_LEN, "value too long to send");

//	LOGd("send ch: %d, len: %d", handle, length);
	//BLEutil::printArray((uint8_t*)p_data, length);
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		APP_ERROR_CHECK(rbc_mesh_value_set(handle, (uint8_t*)p_data, length));
	}
}

bool Mesh::getLastMessage(uint8_t channel, void** p_data, uint16_t& length) {
	assert(length <= MAX_MESH_MESSAGE_LEN, "value too long to send");

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

	if (evt->value_handle > MESH_NUM_OF_CHANNELS) {
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
			uint32_t ts = RTC::now() - _mesh_init_time;
			if (ts < BOOT_TIME) {
				LOGi("t: %d: ch: %d, skipping message within boot-up time!", ts, evt.value_handle);
				rbc_mesh_packet_release(evt.data);
				continue;
			} else {
				LOGi("first received at: %d", ts);
			}
		}

		//! handle the message
		handleMeshMessage(&evt);

		//! then free associated memory
		rbc_mesh_packet_release(evt.data);
	}
}

