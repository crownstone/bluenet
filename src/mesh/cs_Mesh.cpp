/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 12 Apr., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "mesh/cs_Mesh.h"

extern "C" {
#include "nrf_mesh.h"
#include "mesh_config.h"
#include "mesh_stack.h"
#include "access.h"
#include "access_config.h"
#include "config_server.h"
#include "config_server_events.h"
#include "device_state_manager.h"
#include "mesh_provisionee.h"
#include "nrf_mesh_configure.h"
#include "nrf_mesh_events.h"
#include "net_state.h"
#include "uri.h"
#include "utils.h"
#include "log.h"
}

#include "cfg/cs_Boards.h"
#include "drivers/cs_Serial.h"
#include "util/cs_BleError.h"
#include "util/cs_Utils.h"
#include "ble/cs_Stack.h"
#include "events/cs_EventDispatcher.h"


//static void generic_onoff_state_get_cb(const generic_onoff_server_t * p_self,
//                                       const access_message_rx_meta_t * p_meta,
//                                       generic_onoff_status_params_t * p_out);
//static void generic_onoff_state_set_cb(const generic_onoff_server_t * p_self,
//                                       const access_message_rx_meta_t * p_meta,
//                                       const generic_onoff_set_params_t * p_in,
//                                       const model_transition_t * p_in_transition,
//                                       generic_onoff_status_params_t * p_out);
//
//static void generic_onoff_state_get_cb(const generic_onoff_server_t * p_self,
//                                       const access_message_rx_meta_t * p_meta,
//                                       generic_onoff_status_params_t * p_out)
//{
//    LOGi("msg: GET");
//
//    app_onoff_server_t   * p_server = PARENT_BY_FIELD_GET(app_onoff_server_t, server, p_self);
//
//    /* Requirement: Provide the current value of the OnOff state */
//    p_server->onoff_get_cb(p_server, &p_server->state.present_onoff);
//    p_out->present_on_off = p_server->state.present_onoff;
//    p_out->target_on_off = p_server->state.target_onoff;
//
//    /* Requirement: Always report remaining time */
//    if (p_server->state.remaining_time_ms > 0 && p_server->state.delay_ms == 0)
//    {
//        uint32_t delta = (1000ul * app_timer_cnt_diff_compute(app_timer_cnt_get(), p_server->last_rtc_counter)) / APP_TIMER_CLOCK_FREQ;
//        if (p_server->state.remaining_time_ms >= delta && delta > 0)
//        {
//            p_out->remaining_time_ms = p_server->state.remaining_time_ms - delta;
//        }
//        else
//        {
//            p_out->remaining_time_ms = 0;
//        }
//    }
//    else
//    {
//        p_out->remaining_time_ms = p_server->state.remaining_time_ms;
//    }
//}
//
//static void generic_onoff_state_set_cb(const generic_onoff_server_t * p_self,
//                                       const access_message_rx_meta_t * p_meta,
//                                       const generic_onoff_set_params_t * p_in,
//                                       const model_transition_t * p_in_transition,
//                                       generic_onoff_status_params_t * p_out)
//{
//    LOGi("msg: SET: %d\n", p_in->on_off);
//
//    app_onoff_server_t   * p_server = PARENT_BY_FIELD_GET(app_onoff_server_t, server, p_self);
//
//    /* Update internal representation of OnOff value, process timing */
//    p_server->value_updated = false;
//    p_server->state.target_onoff = p_in->on_off;
//    if (p_in_transition == NULL)
//    {
//        p_server->state.delay_ms = 0;
//        p_server->state.remaining_time_ms = 0;
//    }
//    else
//    {
//        p_server->state.delay_ms = p_in_transition->delay_ms;
//        p_server->state.remaining_time_ms = p_in_transition->transition_time_ms;
//    }
//
//    onoff_state_value_update(p_server);
//    onoff_state_process_timing(p_server);
//
//    /* Prepare response */
//    if (p_out != NULL)
//    {
//        p_out->present_on_off = p_server->state.present_onoff;
//        p_out->target_on_off = p_server->state.target_onoff;
//        p_out->remaining_time_ms = p_server->state.remaining_time_ms;
//    }
//}


static void cs_mesh_event_handler(const nrf_mesh_evt_t * p_evt) {

}
static nrf_mesh_evt_handler_t cs_mesh_event_handler_struct = {
		cs_mesh_event_handler
};

static void app_onoff_server_set_cb(const app_onoff_server_t * p_server, bool onoff);
static void app_onoff_server_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);
static bool switchVal = false;

/* Callback for updating the hardware state */
static void app_onoff_server_set_cb(const app_onoff_server_t * p_server, bool onoff)
{
	/* Resolve the server instance here if required, this example uses only 1 instance. */
	LOGi("Set switch: %d", onoff);
	switchVal = onoff;
}

/* Callback for reading the hardware state */
static void app_onoff_server_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff)
{
	*p_present_onoff = switchVal;
}


static void device_identification_start_cb(uint8_t attention_duration_s)
{
	LOGi("device identification start");
}

static void provisioning_aborted_cb(void)
{
	LOGi("provisioning aborted");
}

static void provisioning_complete_cb(void) {
	LOGi("provisioning complete");

	dsm_local_unicast_address_t node_address;
	dsm_local_unicast_addresses_get(&node_address);
	LOGi("Node Address: 0x%04x", node_address.address_start);
}




static void app_gen_onoff_client_publish_interval_cb(access_model_handle_t handle, void * p_self);
static void app_generic_onoff_client_status_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in);
static void app_gen_onoff_client_transaction_status_cb(access_model_handle_t model_handle,
                                                       void * p_args,
                                                       access_reliable_status_t status);

/* This callback is called periodically if model is configured for periodic publishing */
static void app_gen_onoff_client_publish_interval_cb(access_model_handle_t handle, void * p_self) {
	LOGw("Publish desired message here.");
}

/* Generic OnOff client model interface: Process the received status message in this callback */
static void app_generic_onoff_client_status_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in)
{
	LOGi("status: state=%u target=%u remaining_time=%u", p_in->present_on_off, p_in->target_on_off, p_in->remaining_time_ms);
}

/* Acknowledged transaction status callback, if acknowledged transfer fails, application can
* determine suitable course of action (e.g. re-initiate previous transaction) by using this
* callback.
*/
static void app_gen_onoff_client_transaction_status_cb(access_model_handle_t model_handle,
                                                       void * p_args,
                                                       access_reliable_status_t status)
{
	switch(status) {
	case ACCESS_RELIABLE_TRANSFER_SUCCESS:
		LOGi("Acknowledged transfer success.");
		break;
	case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
		LOGi("Acknowledged transfer timeout.");
		break;
	case ACCESS_RELIABLE_TRANSFER_CANCELLED:
		LOGi("Acknowledged transfer cancelled.");
		break;
	default:
		APP_ERROR_CHECK(NRF_ERROR_INTERNAL);
		break;
	}
}

static void config_server_evt_cb(const config_server_evt_t * p_evt) {
	if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET) {
		LOGi("----- Node reset  -----");
		/* This function may return if there are ongoing flash operations. */
		mesh_stack_device_reset();
	}
}

const generic_onoff_client_callbacks_t client_cbs =
{
		.onoff_status_cb = app_generic_onoff_client_status_cb,
		.ack_transaction_status_cb = app_gen_onoff_client_transaction_status_cb,
		.periodic_publish_cb = app_gen_onoff_client_publish_interval_cb
};

static void scan_cb(const nrf_mesh_adv_packet_rx_data_t *p_rx_data) {
	switch (p_rx_data->p_metadata->source) {
	case NRF_MESH_RX_SOURCE_SCANNER:{
//	    timestamp_t timestamp; /**< Device local timestamp of the start of the advertisement header of the packet in microseconds. */
//	    uint32_t access_addr; /**< Access address the packet was received on. */
//	    uint8_t  channel; /**< Channel the packet was received on. */
//	    int8_t   rssi; /**< RSSI value of the received packet. */
//	    ble_gap_addr_t adv_addr; /**< Advertisement address in the packet. */
//	    uint8_t adv_type;  /**< BLE GAP advertising type. */

//		const uint8_t* addr = p_rx_data->p_metadata->params.scanner.adv_addr.addr;
//		const uint8_t* p = p_rx_data->p_payload;
//		if (p[0] == 0x15 && p[1] == 0x16 && p[2] == 0x01 && p[3] == 0xC0 && p[4] == 0x05) {
////		if (p[1] == 0xFF && p[2] == 0xCD && p[3] == 0xAB) {
////		if (addr[5] == 0xE7 && addr[4] == 0x09 && addr[3] == 0x62) { // E7:09:62:02:91:3D
//			LOGd("Mesh scan: address=%02X:%02X:%02X:%02X:%02X:%02X type=%u rssi=%i chan=%u", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], p_rx_data->p_metadata->params.scanner.adv_type, p_rx_data->p_metadata->params.scanner.rssi, p_rx_data->p_metadata->params.scanner.channel);
//			LOGd("  adv_type=%u len=%u data=", p_rx_data->adv_type, p_rx_data->length);
//			BLEutil::printArray(p_rx_data->p_payload, p_rx_data->length);
//		}

		scanned_device_t scan;
		memcpy(scan.address, p_rx_data->p_metadata->params.scanner.adv_addr.addr, sizeof(scan.address)); // TODO: check addr_type and addr_id_peer
		scan.addressType = (p_rx_data->p_metadata->params.scanner.adv_addr.addr_type & 0x7F) & ((p_rx_data->p_metadata->params.scanner.adv_addr.addr_id_peer & 0x01) << 7);
		scan.rssi = p_rx_data->p_metadata->params.scanner.rssi;
		scan.channel = p_rx_data->p_metadata->params.scanner.channel;
		scan.dataSize = p_rx_data->length;
		scan.data = (uint8_t*)(p_rx_data->p_payload);
		event_t event(CS_TYPE::EVT_DEVICE_SCANNED, (void*)&scan, sizeof(scan));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case NRF_MESH_RX_SOURCE_GATT:

		break;
	case NRF_MESH_RX_SOURCE_FRIEND:

		break;
	case NRF_MESH_RX_SOURCE_LOW_POWER:

		break;
	case NRF_MESH_RX_SOURCE_INSTABURST:

		break;
	case NRF_MESH_RX_SOURCE_LOOPBACK:
		break;
	}
}






void Mesh::modelsInitCallback() {
    LOGi("Initializing and adding models");
    uint32_t retCode;
    uint8_t elementIndex = 0;
    for (uint32_t i = 0; i < CLIENT_MODEL_INSTANCE_COUNT; ++i) {
        _clients[i].settings.p_callbacks = &client_cbs;
        _clients[i].settings.timeout = 0;
        /* Controls if the model instance should force all mesh messages to be segmented messages. */
        _clients[i].settings.force_segmented = false;
        /* Controls the MIC size used by the model instance for sending the mesh messages. */
        _clients[i].settings.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
        retCode = generic_onoff_client_init(&_clients[i], elementIndex++);
        APP_ERROR_CHECK(retCode);
    }

    // Server
    retCode = app_onoff_init(&_server, elementIndex++);
    APP_ERROR_CHECK(retCode);
    LOGi("Server handle: %d", _server.server.model_handle);
}


Mesh::Mesh() {
	_serverTimerData = { {0} };
	_serverTimerId = &_serverTimerData;

	_server.server.settings.force_segmented = false;
	_server.server.settings.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	_server.p_timer_id = &_serverTimerId;
	_server.onoff_set_cb = app_onoff_server_set_cb;
	_server.onoff_get_cb = app_onoff_server_get_cb;
}

Mesh& Mesh::getInstance() {
	// Use static variant of singleton, no dynamic memory allocation
	static Mesh instance;
	return instance;
}

void Mesh::init() {
#if CS_SERIAL_NRF_LOG_ENABLED == 1
	__LOG_INIT(LOG_SRC_APP | LOG_SRC_PROV | LOG_SRC_ACCESS | LOG_SRC_BEARER, LOG_LEVEL_DBG3, LOG_CALLBACK_DEFAULT);
	__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Mesh init -----\n");
#endif
	nrf_clock_lf_cfg_t lfclksrc;
	lfclksrc.source = NRF_SDH_CLOCK_LF_SRC;
	lfclksrc.rc_ctiv = NRF_SDH_CLOCK_LF_RC_CTIV;
	lfclksrc.rc_temp_ctiv = NRF_SDH_CLOCK_LF_RC_TEMP_CTIV;
	lfclksrc.accuracy = NRF_CLOCK_LF_ACCURACY_20_PPM;

	mesh_stack_init_params_t init_params;
	init_params.core.irq_priority       = NRF_MESH_IRQ_PRIORITY_THREAD; // See mesh_interrupt_priorities.md
	init_params.core.lfclksrc           = lfclksrc;
	init_params.core.p_uuid             = NULL;
	init_params.core.relay_cb           = NULL;
	init_params.models.models_init_cb   = staticModelsInitCallback;
	init_params.models.config_server_cb = config_server_evt_cb;

	uint32_t retCode = mesh_stack_init(&init_params, &_isProvisioned);
	APP_ERROR_CHECK(retCode);
	LOGi("Mesh isProvisioned=%u", _isProvisioned);

	nrf_mesh_evt_handler_add(&cs_mesh_event_handler_struct);

	nrf_mesh_rx_cb_set(scan_cb);
//	EventDispatcher::getInstance().addListener(this);
}

void Mesh::start() {
	uint32_t retCode;
	if (!_isProvisioned) {
		static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
		mesh_provisionee_start_params_t prov_start_params;
		prov_start_params.p_static_data    = static_auth_data;
		prov_start_params.prov_complete_cb = provisioning_complete_cb;
		prov_start_params.prov_device_identification_start_cb = device_identification_start_cb;
		prov_start_params.prov_device_identification_stop_cb = NULL;
		prov_start_params.prov_abort_cb = provisioning_aborted_cb;
//		prov_start_params.p_device_uri = URI_SCHEME_EXAMPLE "URI for LS Client example";
		prov_start_params.p_device_uri = URI_SCHEME_EXAMPLE "URI for LS Server example";
		retCode = mesh_provisionee_prov_start(&prov_start_params);
		APP_ERROR_CHECK(retCode);
	}

	const uint8_t *uuid = nrf_mesh_configure_device_uuid_get();
	LOGi("Device UUID:");
	BLEutil::printArray(uuid, NRF_MESH_UUID_SIZE);
	retCode = mesh_stack_start();
	APP_ERROR_CHECK(retCode);

	EventDispatcher::getInstance().addListener(this);
}

void Mesh::stop() {
	// TODO: implement
}

void Mesh::handleEvent(event_t & event) {
	switch (event.type) {
	case CS_TYPE::EVT_TICK:
		if (Stack::getInstance().isScanning()) {
//			Stack::getInstance().stopScanning();
		}
		else {
			Stack::getInstance().startScanning();
		}
		break;
	default:
		break;
	}
}
