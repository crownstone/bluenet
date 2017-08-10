/*
 * Author: Crownstone Team
 * Copyright: Crownstone
 * Date: Dec 8, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT
 */
#pragma once

#include "ble/cs_Nordic.h"
#include "events/cs_EventListener.h"
#include "util/cs_Utils.h"

#define ENOCEAN_COMPANY_ID                       0x03DA

#define PIN_ACTION_TYPE   0
#define PIN_A0            1
#define PIN_A1            2
#define PIN_B0            3
#define PIN_B1            4

#define MAX_SWITCHES 5

/** Data telegram for EnOcean switch.
 */
struct __attribute__((packed)) data_telegram_t {
	uint16_t companyId;
	uint32_t seqCounter;
	uint8_t switchState;
	uint32_t securitySignature;
};

/** Commissioning telegram for EnOcean switch.
 */
struct __attribute__((packed)) commissioning_telegram_t {
	uint16_t companyId;
	uint32_t seqCounter;
	uint8_t securityKey[16];
};

/** Learned EnOcean switch, includes BLE address and security key.
 */
struct __attribute__((packed)) learned_enocean_t {
	uint8_t addr[BLE_GAP_ADDR_LEN];
	uint8_t securityKey[16];
};

class EnOceanHandler : public EventListener {

private:
	EnOceanHandler();

	learned_enocean_t* findEnOcean(uint8_t * adrs_ptr);
	learned_enocean_t* newEnOcean();

	bool authenticate(uint8_t * adrs_ptr, data_telegram_t* p_data);

	bool parseAdvertisement(ble_gap_evt_adv_report_t* p_adv_report);

	bool triggerEnOcean(uint8_t * adrs_ptr, data_t* p_data);
	bool learnEnOcean(uint8_t * adrs_ptr, data_t* p_data);

	void save();

	learned_enocean_t _learnedSwitches[MAX_SWITCHES];

	uint32_t _learningStartTime;

	uint32_t _lastSequenceCounter;

public:
	static EnOceanHandler& getInstance() {
		static EnOceanHandler instance;
		return instance;
	}

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	void init();

};

