/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 3, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Eddystone.h>
#include <util/cs_Utils.h>
#include <util/cs_BleError.h>

//using namespace BLEpp;

#define EDDYSTONE_UID                   0
#define EDDYSTONE_URL                   1
#define EDDYSTONE_TLM                   2

#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(100, UNIT_0_625_MS) /**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */

#define APP_CFG_NON_CONN_ADV_TIMEOUT    0                                 /**< Time for which the device must be advertising in non-connectable mode (in seconds). 0 disables timeout. */

#define APP_MEASURED_RSSI               0xC3                              /**< The Beacon's measured RSSI at 1 meter distance in dBm. */

static edstn_frame_t edstn_frames[3];

static ble_gap_adv_params_t   m_adv_params; 
static ble_gap_adv_data_t     m_adv_data;

static uint8_t m_adv_handle;

static uint8_t m_conn_handle = 1;

static uint8_t pdu_count = 0;

/**
 * If you choose a http or http_www url scheme prefix, make sure that the website resolves to an https site.
 * You can use a shortener like  
 */
enum UrlSchemePrefix {
	http_www  = 0x00,   // make sure http redirects to an https site, or it won't visualize on 
	https_www = 0x01, 
	http      = 0x02,
	https     = 0x03    // only https works on Android
};

/**
 * Initialization advertising as an Eddystone beacon. 
 * There are three frames, the UID, URL, and TML frames.
 */
void Eddystone::advertising_init(void) {
	LOGi("Initialize advertising Eddystone...");

	uint32_t err_code;
	
	LOGd("nrf_sdh_enable_request");
	err_code = nrf_sdh_enable_request(); 
	APP_ERROR_CHECK(err_code);

	uint32_t ram_start = 0;
	LOGd("nrf_sdh_ble_default_cfg_set at %p", ram_start);
	err_code = nrf_sdh_ble_default_cfg_set(m_conn_handle, &ram_start); 
	APP_ERROR_CHECK(err_code);

	LOGd("nrf_sdh_ble_enable");
	err_code = nrf_sdh_ble_enable(&ram_start);
	APP_ERROR_CHECK(err_code);

	init_uid_frame_buffer();
	init_url_frame_buffer();
	init_tlm_frame_buffer();

	// Write the advertisement data to the softdevice
	// It seems we have to choose
	eddystone_set_adv_data(EDDYSTONE_URL);

	// Initialize advertising parameters (used when starting advertising).
	memset(&m_adv_params, 0, sizeof(m_adv_params));

	m_adv_params.primary_phy                 = BLE_GAP_PHY_1MBPS;
	m_adv_params.properties.type             = BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED;
	m_adv_params.p_peer_addr                 = NULL;
	m_adv_params.filter_policy               = BLE_GAP_ADV_FP_ANY;
	m_adv_params.interval                    = NON_CONNECTABLE_ADV_INTERVAL;
	m_adv_params.duration                    = 0;
}

/**
 * Actually start advertising. 
 * TODO: integrate iBeacon and Eddystone code. The Nordic SDK does not seem to have a way
 * in which easily can be sampled from multiple buffers for the advertisement packets.
 * 
 */
void Eddystone::advertising_start(void) {
	LOGi("Start advertising Eddystone...");
	uint32_t err_code;

	err_code = sd_ble_gap_adv_start(m_adv_handle, m_conn_handle);
	LOGd("adv_start %d", err_code)
	APP_ERROR_CHECK(err_code);
}

void Eddystone::init_url_frame_buffer() {
	uint8_t *encoded_advdata = edstn_frames[EDDYSTONE_URL].adv_frame;
	uint8_t *len_advdata = &edstn_frames[EDDYSTONE_URL].adv_len;

	eddystone_head_encode(encoded_advdata, 0x10, len_advdata);

	encoded_advdata[(*len_advdata)++] = APP_MEASURED_RSSI;

#ifdef USE_BITDO
	encoded_advdata[(*len_advdata)++] = http; 
	const std::string website = "bit.do/crownstone";
#else
	encoded_advdata[(*len_advdata)++] = https; 
	const std::string website = "goo.gl/EzjCO3";	
#endif
	LOGi("Broadcast URL: %s", website.c_str());
	memcpy(&encoded_advdata[*len_advdata], website.c_str(), website.length());
	(*len_advdata) += website.length();
	
	encoded_advdata[0x07] = (*len_advdata) - 8; // Length	Service Data. Ibid. § 1.11
}

void Eddystone::init_tlm_frame_buffer() {
	uint8_t *encoded_advdata = edstn_frames[EDDYSTONE_TLM].adv_frame;
	uint8_t *len_advdata = &edstn_frames[EDDYSTONE_TLM].adv_len;

	eddystone_head_encode(encoded_advdata, 0x20, len_advdata);
	encoded_advdata[(*len_advdata)++] = 0x00; // Version

	//    uint8_t battery_data = battery_level_get();
	encoded_advdata[(*len_advdata)++] = 0x00; // Battery voltage, 1 mV/bit
	encoded_advdata[(*len_advdata)++] = 0x00;

	// Beacon temperature
	uint8_t temperature = 30; // temporary
	eddystone_uint16(encoded_advdata, len_advdata, temperature);

	// Advertising PDU count
	eddystone_uint32(encoded_advdata, len_advdata, pdu_count);

	// Time since power-on or reboot
	//   *len_advdata += big32cpy(encoded_advdata + *len_advdata, pdu_count);

	encoded_advdata[0x07] = (*len_advdata) - 8; // Length	Service Data. Ibid. § 1.11
}

void Eddystone::init_uid_frame_buffer() {
	uint8_t *encoded_advdata = edstn_frames[EDDYSTONE_UID].adv_frame;
	uint8_t *len_advdata = &edstn_frames[EDDYSTONE_UID].adv_len;

	eddystone_head_encode(encoded_advdata, 0x00, len_advdata);

	encoded_advdata[(*len_advdata)++] = APP_MEASURED_RSSI;
	encoded_advdata[(*len_advdata)++] = 0x00;
	encoded_advdata[(*len_advdata)++] = 0x01;
	encoded_advdata[(*len_advdata)++] = 0x02;
	encoded_advdata[(*len_advdata)++] = 0x03;
	encoded_advdata[(*len_advdata)++] = 0x04;
	encoded_advdata[(*len_advdata)++] = 0x05;
	encoded_advdata[(*len_advdata)++] = 0x06;
	encoded_advdata[(*len_advdata)++] = 0x07;
	encoded_advdata[(*len_advdata)++] = 0x08;
	encoded_advdata[(*len_advdata)++] = 0x09;

	encoded_advdata[(*len_advdata)++] = 0x00;
	encoded_advdata[(*len_advdata)++] = 0x01;
	encoded_advdata[(*len_advdata)++] = 0x02;
	encoded_advdata[(*len_advdata)++] = 0x03;
	encoded_advdata[(*len_advdata)++] = 0x04;
	encoded_advdata[(*len_advdata)++] = 0x05;
	encoded_advdata[(*len_advdata)++] = 0x06;

	encoded_advdata[0x07] = (*len_advdata) - 8; // Length	Service Data. Ibid. § 1.11
}

//static ble_advdata_t          adv_data;

uint32_t Eddystone::eddystone_set_adv_data(uint32_t frame_index) {
	/*
	std::string str = "Test";
	uint32_t ret;
	ble_gap_conn_sec_mode_t sec_mode;
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
	ret = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t*)str.c_str(), str.size());
	APP_ERROR_CHECK(ret);

	memset(&adv_data, 0, sizeof(adv_data));
	static int8_t tx_power_level = 4;
	adv_data.name_type          = BLE_ADVDATA_NO_NAME;
	adv_data.short_name_len     = 6;
	adv_data.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	adv_data.p_tx_power_level   = &tx_power_level;
	adv_data.include_appearance = false;
	m_adv_data.adv_data.len = 31;
	ret = ble_advdata_encode(&adv_data, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
	APP_ERROR_CHECK(ret);
*/
	m_adv_data.adv_data.p_data      = edstn_frames[frame_index].adv_frame;
	m_adv_data.adv_data.len         = edstn_frames[frame_index].adv_len;
	m_adv_data.scan_rsp_data.p_data = NULL;
	m_adv_data.scan_rsp_data.len    = 0;
	LOGd("Encoded data size: %i", m_adv_data.adv_data.len);
	return sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
}

uint32_t Eddystone::eddystone_head_encode(uint8_t *p_encoded_data,
		uint8_t frame_type,
		uint8_t *p_len) {
	// Check for buffer overflow.
	if ((*p_len) + 12 > BLE_GAP_ADV_MAX_SIZE) {
		return NRF_ERROR_DATA_SIZE;
	}

	(*p_len) = 0;
	p_encoded_data[(*p_len)++] = 0x02; // Length	Flags. CSS v5, Part A, § 1.3
	p_encoded_data[(*p_len)++] = 0x01; // Flags data type value
	p_encoded_data[(*p_len)++] = 0x06; // Flags data
	p_encoded_data[(*p_len)++] = 0x03; // Length	Complete list of 16-bit Service UUIDs. Ibid. § 1.1
	p_encoded_data[(*p_len)++] = 0x03; // Complete list of 16-bit Service UUIDs data type value
	p_encoded_data[(*p_len)++] = 0xAA; // 16-bit Eddystone UUID
	p_encoded_data[(*p_len)++] = 0xFE;
	p_encoded_data[(*p_len)++] = 0x03; // Length	Service Data. Ibid. § 1.11
	p_encoded_data[(*p_len)++] = 0x16; // Service Data data type value
	p_encoded_data[(*p_len)++] = 0xAA; // 16-bit Eddystone UUID
	p_encoded_data[(*p_len)++] = 0xFE;
	p_encoded_data[(*p_len)++] = frame_type;

	return NRF_SUCCESS;
}

uint32_t Eddystone::eddystone_uint32(uint8_t *p_encoded_data,
		uint8_t *p_len,
		uint32_t val) {
	if ((*p_len) + 4 > BLE_GAP_ADV_MAX_SIZE) {
		return NRF_ERROR_DATA_SIZE;
	}

	p_encoded_data[(*p_len)++] = (uint8_t) (val >> 24u);
	p_encoded_data[(*p_len)++] = (uint8_t) (val >> 16u);
	p_encoded_data[(*p_len)++] = (uint8_t) (val >> 8u);
	p_encoded_data[(*p_len)++] = (uint8_t) (val >> 0u);

	return NRF_SUCCESS;
}

uint32_t Eddystone::eddystone_uint16(uint8_t *p_encoded_data,
		uint8_t *p_len,
		uint16_t val) {
	if ((*p_len) + 2 > BLE_GAP_ADV_MAX_SIZE) {
		return NRF_ERROR_DATA_SIZE;
	}

	p_encoded_data[(*p_len)++] = (uint8_t) (val >> 8u);
	p_encoded_data[(*p_len)++] = (uint8_t) (val >> 0u);

	return NRF_SUCCESS;
}


