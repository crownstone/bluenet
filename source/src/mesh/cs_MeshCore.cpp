/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshCore.h>
#include <storage/cs_State.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

extern "C" {
#include "access.h"
#include "access_config.h"
#include "access_internal.h"
#include "ad_type_filter.h"
#include "config_server.h"
#include "config_server_events.h"
#include "device_state_manager.h"
#include "flash_manager_defrag.h"
#include "log.h"
#include "mesh_config.h"
#include "mesh_opt_core.h"
#include "mesh_provisionee.h"
#include "mesh_stack.h"
#include "net_state.h"
#include "nrf_mesh.h"
#include "nrf_mesh_configure.h"
#include "nrf_mesh_events.h"
#include "scanner.h"
#include "transport.h"
#include "uri.h"
#include "utils.h"
}

#if NRF_MESH_KEY_SIZE != ENCRYPTION_KEY_LENGTH
#error "Mesh key size doesn't match encryption key size"
#endif

#define ENABLE_MESH_PROVISIONING 0

static void meshEventHandler(const nrf_mesh_evt_t* p_evt);

/*
 * This struct must be statically allocated. The event handler can be set statically. The node and is_removed fields
 * are set internally.
 */
static nrf_mesh_evt_handler_t meshEventHandlerStruct = {.evt_cb = meshEventHandler, nullptr, false};

#if MESH_PERSISTENT_STORAGE == 2

/**
 * Copied from mest_opt_net_state.h
 */
enum {
	MESH_OPT_NET_STATE_SEQ_NUM_BLOCK_LEGACY_RECORD = 1,
	MESH_OPT_NET_STATE_IV_INDEX_LEGACY_RECORD,
	MESH_OPT_NET_STATE_SEQ_NUM_BLOCK_RECORD,
	MESH_OPT_NET_STATE_IV_INDEX_RECORD
};

static CS_TYPE cs_mesh_get_type_from_handle(uint16_t handle) {
	switch (handle) {
		case MESH_OPT_NET_STATE_SEQ_NUM_BLOCK_LEGACY_RECORD: return CS_TYPE::STATE_MESH_SEQ_NUMBER;
		case MESH_OPT_NET_STATE_IV_INDEX_LEGACY_RECORD: return CS_TYPE::STATE_MESH_IV_INDEX;
		case MESH_OPT_NET_STATE_SEQ_NUM_BLOCK_RECORD: return CS_TYPE::STATE_MESH_SEQ_NUMBER_V5;
		case MESH_OPT_NET_STATE_IV_INDEX_RECORD: return CS_TYPE::STATE_MESH_IV_INDEX_V5;
		default: return CS_TYPE::CONFIG_DO_NOT_USE;
	}
}

static uint32_t cs_mesh_write_cb(uint16_t handle, void* data_ptr, uint16_t data_size) {
	assert(CsUtils::getInterruptLevel() == 0, "Invalid interrupt level");
	CS_TYPE type          = cs_mesh_get_type_from_handle(handle);
	cs_ret_code_t retCode = State::getInstance().set(type, data_ptr, data_size);
	_log(SERIAL_DEBUG, false, "cs_mesh_write_cb handle=%u retCode=%u data=", handle, retCode);
	_logArray(SERIAL_DEBUG, true, (uint8_t*)data_ptr, data_size);
	switch (retCode) {
		case ERR_SUCCESS: {
			[[fallthrough]];
		}
		case ERR_SUCCESS_NO_CHANGE: {
			return NRF_SUCCESS;
		}
		default: {
			return retCode;
		}
	}
}

static uint32_t cs_mesh_read_cb(uint16_t handle, void* data_ptr, uint16_t data_size) {
	assert(CsUtils::getInterruptLevel() == 0, "Invalid interrupt level");
	CS_TYPE type          = cs_mesh_get_type_from_handle(handle);
	cs_ret_code_t retCode = State::getInstance().get(type, data_ptr, data_size);
	_log(SERIAL_DEBUG, false, "cs_mesh_read_cb handle=%u retCode=%u data=", handle, retCode);
	_logArray(SERIAL_DEBUG, true, (uint8_t*)data_ptr, data_size);
	switch (retCode) {
		case ERR_SUCCESS: {
			return NRF_SUCCESS;
		}
		default: {
			return retCode;
		}
	}
}

static uint32_t cs_mesh_erase_cb(uint16_t handle) {
	assert(CsUtils::getInterruptLevel() == 0, "Invalid interrupt level");
	CS_TYPE type          = cs_mesh_get_type_from_handle(handle);
	cs_ret_code_t retCode = State::getInstance().remove(type, 0);
	LOGd("cs_mesh_erase_cb handle=%u retCode=%u", handle, retCode);
	switch (retCode) {
		case ERR_SUCCESS: {
			return NRF_SUCCESS;
		}
		default: {
			return retCode;
		}
	}
}
#endif  // MESH_PERSISTENT_STORAGE == 2

static void meshEventHandler(const nrf_mesh_evt_t* p_evt) {
	switch (p_evt->type) {
		case NRF_MESH_EVT_MESSAGE_RECEIVED: {
			LOGMeshVerbose("NRF_MESH_EVT_MESSAGE_RECEIVED");
			_log(SERIAL_VERY_VERBOSE, false, "src=%u data: {", p_evt->params.message.p_metadata->source);
			_logArray(SERIAL_VERY_VERBOSE, true, p_evt->params.message.p_buffer, p_evt->params.message.length);
			break;
		}
		case NRF_MESH_EVT_TX_COMPLETE: {
			LOGMeshVerbose("NRF_MESH_EVT_TX_COMPLETE");
			break;
		}
		case NRF_MESH_EVT_IV_UPDATE_NOTIFICATION: {
			LOGMeshVerbose("NRF_MESH_EVT_IV_UPDATE_NOTIFICATION");
			break;
		}
		case NRF_MESH_EVT_KEY_REFRESH_NOTIFICATION: {
			LOGMeshVerbose("NRF_MESH_EVT_KEY_REFRESH_NOTIFICATION");
			break;
		}
		case NRF_MESH_EVT_NET_BEACON_RECEIVED: {
			LOGMeshVerbose("NRF_MESH_EVT_NET_BEACON_RECEIVED");
			break;
		}
		case NRF_MESH_EVT_HB_MESSAGE_RECEIVED: {
			LOGMeshVerbose("NRF_MESH_EVT_HB_MESSAGE_RECEIVED");
			break;
		}
		case NRF_MESH_EVT_HB_SUBSCRIPTION_CHANGE: {
			LOGMeshVerbose("NRF_MESH_EVT_HB_SUBSCRIPTION_CHANGE");
			break;
		}
		case NRF_MESH_EVT_DFU_REQ_RELAY: {
			LOGMeshVerbose("NRF_MESH_EVT_DFU_REQ_SOURCE");
			break;
		}
		case NRF_MESH_EVT_DFU_REQ_SOURCE: {
			LOGMeshVerbose("NRF_MESH_EVT_DFU_REQ_SOURCE");
			break;
		}
		case NRF_MESH_EVT_DFU_START: {
			LOGMeshVerbose("NRF_MESH_EVT_DFU_START");
			break;
		}
		case NRF_MESH_EVT_DFU_END: {
			LOGMeshVerbose("NRF_MESH_EVT_DFU_END");
			break;
		}
		case NRF_MESH_EVT_DFU_BANK_AVAILABLE: {
			LOGMeshVerbose("NRF_MESH_EVT_DFU_BANK_AVAILABLE");
			break;
		}
		case NRF_MESH_EVT_DFU_FIRMWARE_OUTDATED: {
			LOGMeshVerbose("NRF_MESH_EVT_DFU_FIRMWARE_OUTDATED");
			break;
		}
		case NRF_MESH_EVT_DFU_FIRMWARE_OUTDATED_NO_AUTH: {
			LOGMeshVerbose("NRF_MESH_EVT_DFU_FIRMWARE_OUTDATED_NO_AUTH");
			break;
		}
		case NRF_MESH_EVT_FLASH_STABLE: {
			LOGMeshVerbose("NRF_MESH_EVT_FLASH_STABLE");
			MeshCore::getInstance().factoryResetDone();
			break;
		}
		case NRF_MESH_EVT_RX_FAILED: {
			LOGMeshVerbose("NRF_MESH_EVT_RX_FAILED");
			break;
		}
		case NRF_MESH_EVT_SAR_FAILED: {
			LOGMeshVerbose("NRF_MESH_EVT_SAR_FAILED");
			break;
		}
		case NRF_MESH_EVT_FLASH_FAILED: {
			LOGMeshVerbose("NRF_MESH_EVT_FLASH_FAILED");
			break;
		}
		case NRF_MESH_EVT_CONFIG_STABLE: {
			LOGMeshVerbose("NRF_MESH_EVT_CONFIG_STABLE");
			break;
		}
		case NRF_MESH_EVT_CONFIG_STORAGE_FAILURE: {
			LOGMeshVerbose("NRF_MESH_EVT_CONFIG_STORAGE_FAILURE");
			break;
		}
		case NRF_MESH_EVT_CONFIG_LOAD_FAILURE: {
			LOGMeshVerbose("NRF_MESH_EVT_CONFIG_LOAD_FAILURE");
			break;
		}
		case NRF_MESH_EVT_LPN_FRIEND_OFFER: {
			LOGMeshVerbose("NRF_MESH_EVT_LPN_FRIEND_OFFER");
			break;
		}
		case NRF_MESH_EVT_LPN_FRIEND_UPDATE: {
			LOGMeshVerbose("NRF_MESH_EVT_LPN_FRIEND_UPDATE");
			break;
		}
		case NRF_MESH_EVT_LPN_FRIEND_REQUEST_TIMEOUT: {
			LOGMeshVerbose("NRF_MESH_EVT_LPN_FRIEND_REQUEST_TIMEOUT");
			break;
		}
		case NRF_MESH_EVT_LPN_FRIEND_POLL_COMPLETE: {
			LOGMeshVerbose("NRF_MESH_EVT_LPN_FRIEND_POLL_COMPLETE");
			break;
		}
		case NRF_MESH_EVT_FRIENDSHIP_ESTABLISHED: {
			LOGMeshVerbose("NRF_MESH_EVT_FRIENDSHIP_ESTABLISHED");
			break;
		}
		case NRF_MESH_EVT_FRIENDSHIP_TERMINATED: {
			LOGMeshVerbose("NRF_MESH_EVT_FRIENDSHIP_TERMINATED");
			break;
		}
		case NRF_MESH_EVT_DISABLED: {
			LOGMeshVerbose("NRF_MESH_EVT_DISABLED");
			event_t event(CS_TYPE::EVT_BLE_CENTRAL_CONNECT_CLEARANCE_REPLY);
			event.dispatch();
			break;
		}
		case NRF_MESH_EVT_PROXY_STOPPED: {
			LOGMeshVerbose("NRF_MESH_EVT_PROXY_STOPPED");
			break;
		}
		case NRF_MESH_EVT_FRIEND_REQUEST: {
			LOGMeshVerbose("NRF_MESH_EVT_FRIEND_REQUEST");
			break;
		}
#if MESH_SDK_VERSION_MAJOR > 4
		case NRF_MESH_EVT_ENABLED: {
			LOGMeshVerbose("NRF_MESH_EVT_ENABLED");
			break;
		}
		case NRF_MESH_EVT_READY_TO_POWER_OFF: {
			LOGMeshVerbose("NRF_MESH_EVT_READY_TO_POWER_OFF");
			break;
		}
#endif
	}
}

static void configServerEventCallback(const config_server_evt_t* p_evt) {
	LOGMeshInfo("Config event %u", p_evt->type);
	if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET) {
		LOGMeshInfo("----- Node reset  -----");
		// This function may return if there are ongoing flash operations.
		mesh_stack_device_reset();
	}
}

static void scan_cb(const nrf_mesh_adv_packet_rx_data_t* p_rx_data) {
	MeshCore::getInstance().scanCallback(p_rx_data);
}

void MeshCore::scanCallback(const nrf_mesh_adv_packet_rx_data_t* scanData) {
	_scanCallback(scanData);
}

static void staticModelsInitCallback() {
	MeshCore::getInstance().modelsInitCallback();
}

void MeshCore::modelsInitCallback() {
	_modelInitCallback();
}

MeshCore::MeshCore() {}

MeshCore& MeshCore::getInstance() {
	static MeshCore instance;
	return instance;
}

void MeshCore::registerModelInitCallback(const callback_model_init_t& closure) {
	_modelInitCallback = closure;
}

void MeshCore::registerModelConfigureCallback(const callback_model_configure_t& closure) {
	_modelConfigureCallback = closure;
}

void MeshCore::registerScanCallback(const callback_scan_t& closure) {
	_scanCallback = closure;
}

cs_ret_code_t MeshCore::init(const boards_config_t& board) {
#if CS_SERIAL_NRF_LOG_ENABLED == 1
	__LOG_INIT(
			LOG_SRC_APP | LOG_SRC_PROV | LOG_SRC_ACCESS | LOG_SRC_BEARER | LOG_SRC_TRANSPORT | LOG_SRC_NETWORK,
			LOG_LEVEL_DBG3,
			LOG_CALLBACK_DEFAULT);
	__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Mesh init -----\n");
#endif
	assert(_modelInitCallback != nullptr && _modelConfigureCallback != nullptr && _scanCallback != nullptr,
		   "Callback not set");

#if MESH_PERSISTENT_STORAGE == 1
	if (!isFlashValid()) {
		return ERR_WRONG_STATE;
	}
#endif

	// Listen for bluenet events.
	listen();

	nrf_clock_lf_cfg_t lfclksrc;
	lfclksrc.source       = NRF_SDH_CLOCK_LF_SRC;
	lfclksrc.rc_ctiv      = NRF_SDH_CLOCK_LF_RC_CTIV;
	lfclksrc.rc_temp_ctiv = NRF_SDH_CLOCK_LF_RC_TEMP_CTIV;
	lfclksrc.accuracy     = NRF_CLOCK_LF_ACCURACY_20_PPM;

	mesh_stack_init_params_t init_params;
	init_params.core.irq_priority       = NRF_MESH_IRQ_PRIORITY_THREAD;  // See mesh_interrupt_priorities.md
	init_params.core.lfclksrc           = lfclksrc;
	init_params.core.p_uuid             = NULL;
	init_params.core.relay_cb           = NULL;
	init_params.models.models_init_cb   = staticModelsInitCallback;
	init_params.models.config_server_cb = configServerEventCallback;

#if MESH_PERSISTENT_STORAGE == 2
	net_state_register_ext_storage_write_cb(cs_mesh_write_cb);
	net_state_register_ext_storage_read_cb(cs_mesh_read_cb);
	net_state_register_ext_storage_erase_cb(cs_mesh_erase_cb);
#endif

	TYPIFY(CONFIG_CROWNSTONE_ID) id;
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &id, sizeof(id));
	_ownAddress = id;

	uint32_t retCode = mesh_stack_init(&init_params, &_isProvisioned);
	APP_ERROR_CHECK(retCode);
	LOGMeshInfo("Mesh isProvisioned=%u", _isProvisioned);

	nrf_mesh_evt_handler_add(&meshEventHandlerStruct);

	LOGi("Mesh scanner: interval=%ums window=%ums", board.scanIntervalUs / 1000, board.scanWindowUs / 1000);
	scanner_config_scan_time_set(board.scanIntervalUs, board.scanWindowUs);

#if MESH_SCANNER == 1
	// Init scanned device variable before registering the callback.
	nrf_mesh_rx_cb_set(scan_cb);

#if MESH_SCANNER_FILTERED == 1
	// Add all AD types we are interested in.
	//	bearer_adtype_add(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE);
	//	bearer_adtype_add(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE);
	//	bearer_adtype_add(BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE);
	//	bearer_adtype_add(BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE);

	// Add all AD types, as we don't know what types will be used for asset filters or microapps.
	// This does not use more RAM, they are simply bits in a bitmask.
	for (int i = 0; i < 256; ++i) {
		bearer_adtype_add(i);
	}
#else
	bearer_adtype_filtering_set(false);
#endif

#else
	LOGw("Scanner in mesh not enabled");
#endif

	ble_gap_addr_t macAddress;
	retCode = sd_ble_gap_addr_get(&macAddress);
	APP_ERROR_CHECK(retCode);
	// have non-connectable address one value higher than connectable one
	macAddress.addr[0] -= 1;
	macAddress.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
	for (uint8_t i = 0; i < CORE_TX_ROLE_COUNT; ++i) {
		mesh_opt_core_adv_addr_set((core_tx_role_t)i, &macAddress);
	}

	TYPIFY(CONFIG_TX_POWER) txPower;
	State::getInstance().get(CS_TYPE::CONFIG_TX_POWER, &txPower, sizeof(txPower));
	setTxPower(txPower);

	//	EventDispatcher::getInstance().addListener(this);
	if (!_isProvisioned) {
		provisionSelf(_ownAddress);
	}
	else {
		provisionLoad();
	}

	_modelConfigureCallback(_appkeyHandle);

	// Settings for single target segmented messages.
	nrf_mesh_opt_t opt;
	opt.len     = sizeof(opt.opt.val);
	opt.opt.val = 4;
	transport_opt_set(NRF_MESH_OPT_TRS_SAR_TX_RETRIES, &opt);

	opt.opt.val = MESH_MODEL_ACKED_RETRY_INTERVAL_MS * 1000;  // μs
	transport_opt_set(NRF_MESH_OPT_TRS_SAR_TX_RETRY_TIMEOUT_BASE, &opt);

	return ERR_SUCCESS;
}

void MeshCore::provisionSelf(uint16_t id) {
	LOGi("provisionSelf");
	uint32_t retCode;

	State::getInstance().get(CS_TYPE::CONFIG_MESH_DEVICE_KEY, _devkey, sizeof(_devkey));
	State::getInstance().get(CS_TYPE::CONFIG_MESH_APP_KEY, _appkey, sizeof(_appkey));
	State::getInstance().get(CS_TYPE::CONFIG_MESH_NET_KEY, _netkey, sizeof(_netkey));

	// Used provisioner_helper.c::prov_helper_provision_self() as example.
	// Also see mesh_stack_provisioning_data_store()
	// And https://devzone.nordicsemi.com/f/nordic-q-a/44515/mesh-node-self-provisioning

	// Store provisioning data in DSM.
	dsm_local_unicast_address_t localAddress;
	localAddress.address_start = id;
	localAddress.count         = 1;
	retCode                    = dsm_local_unicast_addresses_set(&localAddress);
	APP_ERROR_CHECK(retCode);
	LOGi("unicast address=%u", localAddress.address_start);

	retCode = dsm_subnet_add(0, _netkey, &_netkeyHandle);
	APP_ERROR_CHECK(retCode);

	retCode = dsm_appkey_add(0, _netkeyHandle, _appkey, &_appkeyHandle);
	APP_ERROR_CHECK(retCode);

	retCode = dsm_devkey_add(id, _netkeyHandle, _devkey, &_devkeyHandle);
	APP_ERROR_CHECK(retCode);

	uint8_t key[NRF_MESH_KEY_SIZE];
	LOGMeshInfo("netKeyHandle=%u netKey=", _netkeyHandle);
	dsm_subnet_key_get(_netkeyHandle, key);
	// CsUtils::printArray(key, NRF_MESH_KEY_SIZE);
	LOGMeshInfo("appKeyHandle=%u appKey=", _appkeyHandle);
	LOGMeshInfo("devKeyHandle=%u devKey=", _devkeyHandle);

#if MESH_PERSISTENT_STORAGE == 0
	retCode = net_state_iv_index_set(0, 0);
	APP_ERROR_CHECK(retCode);
#endif

	// Bind config server to the device key
	retCode = config_server_bind(_devkeyHandle);
	APP_ERROR_CHECK(retCode);

#if MESH_SDK_VERSION_MAJOR < 5
	access_flash_config_store();
#endif
}

void MeshCore::provisionLoad() {
	LOGi("provisionLoad");
	// Used provisioner_helper.c::prov_helper_device_handles_load() as example.
	uint32_t retCode;
	dsm_local_unicast_address_t localAddress;
	uint32_t netKeyCount                     = 1;
	mesh_key_index_t keyIndices[netKeyCount] = {0};

	retCode = dsm_subnet_get_all(keyIndices, &netKeyCount);
	APP_ERROR_CHECK(retCode);

	if (netKeyCount != 1) {
		APP_ERROR_CHECK(ERR_UNSPECIFIED);
	}

	_netkeyHandle = dsm_net_key_index_to_subnet_handle(keyIndices[0]);

	retCode = dsm_appkey_get_all(_netkeyHandle, &_appkeyHandle, &netKeyCount);
	APP_ERROR_CHECK(retCode);

	dsm_local_unicast_addresses_get(&localAddress);
	retCode = dsm_devkey_handle_get(localAddress.address_start, &_devkeyHandle);
	APP_ERROR_CHECK(retCode);

	LOGi("unicast address=%u", localAddress.address_start);
	uint8_t key[NRF_MESH_KEY_SIZE];
	LOGMeshInfo("netKeyHandle=%u netKey=", _netkeyHandle);
	dsm_subnet_key_get(_netkeyHandle, key);
	LOGMeshInfo("appKeyHandle=%u appKey=", _appkeyHandle);
	LOGMeshInfo("devKeyHandle=%u devKey=", _devkeyHandle);
}

uint16_t MeshCore::getUnicastAddress() {
	return _ownAddress;
}

void MeshCore::provision() {
#if ENABLE_MESH_PROVISIONING == 1
	uint32_t retCode;
	static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
	mesh_provisionee_start_params_t prov_start_params;
	prov_start_params.p_static_data    = static_auth_data;
	prov_start_params.prov_complete_cb = provisioning_complete_cb;
	prov_start_params.prov_device_identification_start_cb = device_identification_start_cb;
	prov_start_params.prov_device_identification_stop_cb = NULL;
	prov_start_params.prov_abort_cb = provisioning_aborted_cb;
	prov_start_params.p_device_uri = URI_SCHEME_EXAMPLE "URI for LS Client example";
	prov_start_params.p_device_uri = URI_SCHEME_EXAMPLE "URI for LS Server example";
	retCode = mesh_provisionee_prov_start(&prov_start_params);
	APP_ERROR_CHECK(retCode);
#endif
}

void MeshCore::start() {
	uint32_t retCode;
#if ENABLE_MESH_PROVISIONING == 1
	if (!_isProvisioned) {
		provision();
	}
#endif
	LOGMeshInfo("ACCESS_FLASH_ENTRY_SIZE=%u", ACCESS_FLASH_ENTRY_SIZE);

	_log(SERIAL_DEBUG, false, "Device UUID: ");
	//	CsUtils::printArray(uuid, NRF_MESH_UUID_SIZE);
	//	_logArray(SERIAL_DEBUG, true, uuid, NRF_MESH_UUID_SIZE, "%02X");
	_logArray(SERIAL_DEBUG, true, nrf_mesh_configure_device_uuid_get(), NRF_MESH_UUID_SIZE);

	// Returns NRF_ERROR_INVALID_STATE if the mesh was already enabled (or mesh stack not initialized).
	retCode = mesh_stack_start();
	if (retCode != NRF_SUCCESS) {
		LOGw("mesh stack start failed: %u", retCode);
	}
}

cs_ret_code_t MeshCore::stop() {
	// Returns NRF_ERROR_INVALID_STATE if the mesh was already disabled.
	uint32_t nrfCode = nrf_mesh_disable();
	switch (nrfCode) {
		case NRF_SUCCESS: {
			return ERR_WAIT_FOR_SUCCESS;
		}
		case NRF_ERROR_INVALID_STATE: {
			return ERR_SUCCESS;
		}
		default: {
			LOGw("mesh disable failed: %u", nrfCode);
			return ERR_UNSPECIFIED;
		}
	}

	// Scanner doesn't stop immediately, but will quickly timeout.
	// See
	// https://devzone.nordicsemi.com/f/nordic-q-a/43301/nrf_mesh_disable-function-changed-since-sdk-for-mesh-v3-1-0
	//	[2020-12-21 14:08:39.941] <info> app: stop
	//	[2020-12-21 14:08:39.944] <debug> nrf_sdh_soc: SoC event: 0x7.
	//	[2020-12-21 14:08:39.947] <debug> nrf_sdh_soc: SoC event: 0x8.
}

cs_ret_code_t MeshCore::setTxPower(int8_t txPower) {
	LOGi("setTxPower %i", txPower);
	radio_tx_power_t radioTxPower = RADIO_POWER_NRF_POS4DBM;
	switch (txPower) {
		case -40:
		case -20:
		case -16:
		case -12:
		case -8:
		case -4:
		case 0:
		case 4: radioTxPower = (radio_tx_power_t)txPower; break;
	}
	for (uint8_t i = 0; i < CORE_TX_ROLE_COUNT; ++i) {
		// @retval NRF_SUCCESS The TX power was successfully set.
		// @retval NRF_ERROR_INVALID_PARAMS Invalid options parameters
		// @retval NRF_ERROR_NOT_FOUND Unknown role.
		uint32_t nrfCode = mesh_opt_core_tx_power_set((core_tx_role_t)i, radioTxPower);
		if (nrfCode != NRF_SUCCESS) {
			LOGe("mesh_opt_core_tx_power_set nrfCode=%u", nrfCode);
			return ERR_UNSPECIFIED;
		}
	}
	return ERR_SUCCESS;
}

void MeshCore::factoryReset() {
	LOGw("factoryReset");

	mesh_stack_config_clear();  // Check if flash_is_stable, or wait for NRF_MESH_EVT_FLASH_STABLE
	_performingFactoryReset = true;
	if (flash_manager_is_stable()) {
		factoryResetDone();
	}
}

void MeshCore::factoryResetDone() {
	if (!_performingFactoryReset) {
		return;
	}
	LOGi("factoryResetDone");
	_performingFactoryReset = false;
	event_t event(CS_TYPE::EVT_MESH_FACTORY_RESET_DONE);
	event.dispatch();
}

#if MESH_PERSISTENT_STORAGE == 1

bool MeshCore::isFlashValid() {
	void* startAddress = NULL;
	void* endAddress   = NULL;
	getFlashPages(startAddress, endAddress);
	unsigned int const pageSize = NRF_FICR->CODEPAGESIZE;
	for (unsigned int* ptr = (unsigned int*)startAddress; ptr < (unsigned int*)endAddress;
		 ptr += pageSize / sizeof(uint32_t)) {
		LOGd("0x%p: 0x%x 0x%x", ptr, ptr[0], ptr[1]);
		// Check only if header is correct.
		// See
		// https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.meshsdk.v4.0.0/md_doc_libraries_flash_manager.html#flash_manager_areas.
		// Since each "area" only has 1 page, we can check of page count == 01, and index == 00.
		// Also accept empty page, all FF.
		if ((ptr[0] == 0x10100408 && ptr[1] == 0xFFFF0001) || (ptr[0] == 0xFFFFFFFF && ptr[1] == 0xFFFFFFFF)) {
			// Valid
		}
		else {
			LOGw("Flash page with invalid data");
			return false;
		}
	}
	return true;
}

void MeshCore::getFlashPages(void*& startAddress, void*& endAddress) {
	// By default, all pages are below the recovery page.
	// Unless one of these is defined:  ACCESS_FLASH_AREA_LOCATION, DSM_FLASH_AREA_LOCATION,
	// NET_FLASH_AREA_LOCATION.
	void* recoveryPage          = flash_manager_defrag_calc_recovery_page();
	const unsigned int numPages = (2 + ACCESS_FLASH_PAGE_COUNT + DSM_FLASH_PAGE_COUNT);
	LOGd("flash manager recovery page = 0x%p numPages=%u", recoveryPage, numPages);

	unsigned int const pageSize = NRF_FICR->CODEPAGESIZE;
	unsigned int endAddr        = (unsigned int)recoveryPage;
	unsigned int startAddr      = endAddr - (numPages * pageSize);
	startAddress                = (void*)startAddr;
	endAddress                  = (void*)endAddr;
}

cs_ret_code_t Mesh::eraseAllPages() {
	void* startAddress = NULL;
	void* endAddress   = NULL;
	getFlashPages(startAddress, endAddress);
	LOGw("eraseAllPages start=0x%p end=0x%p", startAddress, endAddress);
	return Storage::getInstance().erasePages(CS_TYPE::EVT_MESH_PAGES_ERASED, startAddress, endAddress);
}
#endif  // MESH_PERSISTENT_STORAGE == 1

void MeshCore::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::CMD_FACTORY_RESET: {
			factoryReset();
			break;
		}
		case CS_TYPE::EVT_TICK: {
			TYPIFY(EVT_TICK) tickCount = *((TYPIFY(EVT_TICK)*)event.data);
			if (tickCount % (10 * 1000 / TICK_INTERVAL_MS) == 0) {
				[[maybe_unused]] const scanner_stats_t* stats = scanner_stats_get();
				LOGMeshDebug(
						"Scanner stats: success=%u crcFail=%u lenFail=%u memFail=%u",
						stats->successful_receives,
						stats->crc_failures,
						stats->length_out_of_bounds,
						stats->out_of_memory);
			}
			break;
		}
		case CS_TYPE::CONFIG_TX_POWER: {
			auto packet = CS_TYPE_CAST(CONFIG_TX_POWER, event.data);
			setTxPower(*packet);
			break;
		}
#if MESH_PERSISTENT_STORAGE == 2
		case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
			TYPIFY(EVT_STORAGE_WRITE_DONE)* evtData = (TYPIFY(EVT_STORAGE_WRITE_DONE)*)event.data;
			switch (evtData->type) {
				case CS_TYPE::STATE_MESH_SEQ_NUMBER: {
					TYPIFY(STATE_MESH_SEQ_NUMBER) seqNumber;
					State::getInstance().get(CS_TYPE::STATE_MESH_SEQ_NUMBER, &seqNumber, sizeof(seqNumber));
					LOGi("net_state_ext_write_done seqNum=%u", seqNumber);
					net_state_ext_write_done(MESH_OPT_NET_STATE_SEQ_NUM_BLOCK_LEGACY_RECORD, &seqNumber, sizeof(seqNumber));
					break;
				}
				case CS_TYPE::STATE_MESH_SEQ_NUMBER_V5: {
					TYPIFY(STATE_MESH_SEQ_NUMBER_V5) seqNumber;
					State::getInstance().get(CS_TYPE::STATE_MESH_SEQ_NUMBER_V5, &seqNumber, sizeof(seqNumber));
					LOGi("net_state_ext_write_done seqNum=%u syncIndex=%u",
						 seqNumber.next_block,
						 seqNumber.synchro_index);
					net_state_ext_write_done(MESH_OPT_NET_STATE_SEQ_NUM_BLOCK_RECORD, &seqNumber, sizeof(seqNumber));
					break;
				}
				default: break;
			}
			break;
		}
#endif
		default: break;
	}
}
