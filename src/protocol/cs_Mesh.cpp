/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 20 Jan., 2015
 * License: LGPLv3+, Apache, or MIT, your choice
 */

#include <protocol/cs_Mesh.h>

#include <protocol/rbc_mesh_common.h>
#include <protocol/version_handler.h>

#include <cfg/cs_Boards.h>

#include <drivers/cs_Serial.h>
#include <protocol/cs_MeshControl.h>
#include <protocol/cs_MeshMessageTypes.h>
#include <util/cs_BleError.h>

#include <util/cs_Utils.h>
#include <cfg/cs_Config.h>

#include <drivers/cs_RTC.h>

CMesh::CMesh() : _appTimerId(-1) {
	MeshControl::getInstance();
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)CMesh::staticTick);
}

CMesh::~CMesh() {

}

void CMesh::tick() {
	checkForMessages();
	scheduleNextTick();
}

void CMesh::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(MESH_UPDATE_FREQUENCY), this);
}

void CMesh::startTicking() {
	Timer::getInstance().start(_appTimerId, APP_TIMER_TICKS(1, APP_TIMER_PRESCALER), this);
}

void CMesh::stopTicking() {
	Timer::getInstance().stop(_appTimerId);
}

/**
 * Function to test mesh functionality. We have to figure out if we have to enable the radio first, and that kind of
 * thing.
 */
void CMesh::init() {
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
}

void CMesh::send(uint8_t handle, void* p_data, uint8_t length) {
	assert(length <= MAX_MESH_MESSAGE_LEN, "value too long to send");

//	LOGd("send ch: %d, len: %d", handle, length);
	//BLEutil::printArray((uint8_t*)p_data, length);
	APP_ERROR_CHECK(rbc_mesh_value_set(handle, (uint8_t*)p_data, length));
}

bool CMesh::getLastMessage(uint8_t channel, void** p_data, uint16_t& length) {
	assert(length <= MAX_MESH_MESSAGE_LEN, "value too long to send");

	APP_ERROR_CHECK(rbc_mesh_value_get(channel, (uint8_t*)*p_data, &length));
	//LOGi("recv ch: %d, len: %d", handle, length);
	return length != 0;
}

void CMesh::handleMeshMessage(rbc_mesh_event_t* evt)
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

void CMesh::checkForMessages() {
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

