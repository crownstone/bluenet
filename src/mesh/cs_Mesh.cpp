/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 20 Jan., 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_Mesh.h>

extern "C" {
#include <nrf_mesh.h>
#include <mesh_config.h>
#include <mesh_stack.h>
#include <access.h>
#include <config_server_events.h>
#include <mesh_provisionee.h>
#include <nrf_mesh_configure.h>
#include <uri.h>
}

#include <cfg/cs_Boards.h>

#include <drivers/cs_Serial.h>
#include <util/cs_BleError.h>




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

static void provisioning_complete_cb(void) {
	LOGi("Successfully provisioned");

	dsm_local_unicast_address_t node_address;
	dsm_local_unicast_addresses_get(&node_address);
	LOGi("Node Address: 0x%04x", node_address.address_start);
}








void Mesh::modelsInitCallback() {
    LOGi("Initializing and adding models");
    for (uint32_t i = 0; i < CLIENT_MODEL_INSTANCE_COUNT; ++i) {
        _clients[i].settings.p_callbacks = &client_cbs;
        _clients[i].settings.timeout = 0;
        /* Controls if the model instance should force all mesh messages to be segmented messages. */
        _clients[i].settings.force_segmented = false;
        /* Controls the MIC size used by the model instance for sending the mesh messages. */
        _clients[i].settings.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
        uint32_t retCode = generic_onoff_client_init(&_clients[i], i + 1);
        APP_ERROR_CHECK(retCode);
    }
}


Mesh::Mesh() {

}

Mesh& Mesh::getInstance() {
	// Use static variant of singleton, no dynamic memory allocation
	static Mesh instance;
	return instance;
}

void Mesh::init() {
	nrf_clock_lf_cfg_t lfclksrc;
	lfclksrc.source = NRF_SDH_CLOCK_LF_SRC;
	lfclksrc.rc_ctiv = NRF_SDH_CLOCK_LF_RC_CTIV;
	lfclksrc.rc_temp_ctiv = NRF_SDH_CLOCK_LF_RC_TEMP_CTIV;
	lfclksrc.accuracy = NRF_CLOCK_LF_ACCURACY_20_PPM;

	mesh_stack_init_params_t init_params;
	init_params.core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST;
	init_params.core.lfclksrc           = lfclksrc;
	init_params.core.p_uuid             = NULL;
	init_params.models.models_init_cb   = staticModelsInitCallback;
	init_params.models.config_server_cb = config_server_evt_cb;

	uint32_t retCode = mesh_stack_init(&init_params, &_isProvisioned);
	APP_ERROR_CHECK(retCode);
}

void Mesh::start() {
	uint32_t retCode;
	if (!_isProvisioned) {
		static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
		mesh_provisionee_start_params_t prov_start_params;
		prov_start_params.p_static_data    = static_auth_data;
		prov_start_params.prov_complete_cb = provisioning_complete_cb;
		prov_start_params.prov_device_identification_start_cb = NULL;
		prov_start_params.prov_device_identification_stop_cb = NULL;
		prov_start_params.prov_abort_cb = NULL;
		prov_start_params.p_device_uri = URI_SCHEME_EXAMPLE "URI for LS Client example";
		retCode = mesh_provisionee_prov_start(&prov_start_params);
		APP_ERROR_CHECK(retCode);
	}

//	const uint8_t *p_uuid = nrf_mesh_configure_device_uuid_get();
//	LOGi("Device UUID");
//	NRF_LOG_RAW_HEXDUMP_INFO(p_uuid, NRF_MESH_UUID_SIZE);
	retCode = mesh_stack_start();
	APP_ERROR_CHECK(retCode);
}

void Mesh::stop() {
	// TODO: implement
}
