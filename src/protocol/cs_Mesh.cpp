/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 20 Jan., 2015
 * License: LGPLv3+, Apache, or MIT, your choice
 */

#include <protocol/cs_Mesh.h>

#include <protocol/rbc_mesh_common.h>

#include <cfg/cs_Boards.h>

#include <drivers/cs_Serial.h>
#include <protocol/cs_MeshControl.h>
#include <protocol/cs_MeshMessageTypes.h>
#include <util/cs_BleError.h>

#include <util/cs_Utils.h>
#include <cfg/cs_Config.h>

#define MESH_ACCESS_ADDR 0xA541A68E
#define MESH_INTERVAL_MIN_MS 100
#define MESH_CHANNEL 38
#define MESH_CLOCK_SOURCE (CLOCK_SOURCE)

extern "C" {
//#include <protocol/led_config.h>

/**
 * Event handler on receiving a message from
 */
void rbc_mesh_event_handler(rbc_mesh_event_t* evt)
{
	TICK_PIN(28);
	//nrf_gpio_gitpin_toggle(PIN_GPIO_LED1);

	switch (evt->event_type)
	{
	case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:
		LOGd("ch: %d, conflicting value", evt->value_handle);
		break;
	case RBC_MESH_EVENT_TYPE_NEW_VAL:
		LOGd("ch: %d, new value", evt->value_handle);
		break;
	case RBC_MESH_EVENT_TYPE_UPDATE_VAL:
		LOGd("ch: %d, update value", evt->value_handle);
		break;
	case RBC_MESH_EVENT_TYPE_INITIALIZED:
		LOGd("ch: %d, initialized", evt->value_handle);
		break;
	}

	switch (evt->event_type)
	{
//		case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:
		case RBC_MESH_EVENT_TYPE_NEW_VAL:
		case RBC_MESH_EVENT_TYPE_UPDATE_VAL: {

            if (evt->value_handle > 2)
                break;

            //if (evt->data[0]) {
            LOGi("Got data ch: %i, val: %i, len: %d, orig_addr:", evt->value_handle, evt->data[0], evt->data_len);
//            BLEutil::printArray(evt->originator_address.addr, 6);
            MeshControl &meshControl = MeshControl::getInstance();
            meshControl.process(evt->value_handle, evt->data, evt->data_len);
            //}
            //led_config(evt->value_handle, evt->data[0]);
            break;
        }
        default:
            LOGi("Default: %i", evt->event_type);
            break;
	}
}

}

CMesh::CMesh() {
	MeshControl::getInstance();
}

CMesh::~CMesh() {
}

/**
 * Function to test mesh functionality. We have to figure out if we have to enable the radio first, and that kind of
 * thing.
 */
void CMesh::init() {
	LOGi("For now we are testing the meshing functionality.");
	nrf_gpio_pin_clear(PIN_GPIO_LED0);

	rbc_mesh_init_params_t init_params;

	init_params.access_addr = MESH_ACCESS_ADDR;
	init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
	init_params.channel = MESH_CHANNEL;
	init_params.lfclksrc = MESH_CLOCK_SOURCE;

	LOGi("Call rbc_mesh_init");
	uint8_t error_code;
	// checks if softdevice is enabled etc.
	error_code = rbc_mesh_init(init_params);
	APP_ERROR_CHECK(error_code);

	LOGi("Call rbc_mesh_value_enable. Todo. Do this on send() and receive()!");
	error_code = rbc_mesh_value_enable(1);
	APP_ERROR_CHECK(error_code);
	error_code = rbc_mesh_value_enable(2);
	APP_ERROR_CHECK(error_code);
}

//void CMesh::send(uint8_t handle, uint32_t value) {
//	uint8_t val[28];
//	val[0] = (uint8_t)value;
//	LOGi("Set mesh data %i to %i", val[0], handle);
//	APP_ERROR_CHECK(rbc_mesh_value_set(handle, &val[0], 1));
//}

void CMesh::send(uint8_t channel, void* p_data, uint8_t length) {
	LOGi("length: %d, MAX_MESH_MESSAGE_LEN: %d", length, MAX_MESH_MESSAGE_LEN);
	assert(length <= MAX_MESH_MESSAGE_LEN, "value too long to send");

	//LOGi("send ch: %d, len: %d", handle, length);
	//BLEutil::printArray((uint8_t*)p_data, length);
	APP_ERROR_CHECK(rbc_mesh_value_set(channel, (uint8_t*)p_data, length));
}

bool CMesh::getLastMessage(uint8_t channel, void** p_data, uint16_t& length) {
	assert(length <= MAX_MESH_MESSAGE_LEN, "value too long to send");

	APP_ERROR_CHECK(rbc_mesh_value_get(channel, (uint8_t*)*p_data, &length));
	//LOGi("recv ch: %d, len: %d", handle, length);
	return length != 0;
}

void CMesh::receive() {
    rbc_mesh_event_t evt;
	if (rbc_mesh_event_get(&evt) == NRF_SUCCESS) {
		rbc_mesh_event_handler(&evt);
		rbc_mesh_packet_release(evt.data);
	}
}

