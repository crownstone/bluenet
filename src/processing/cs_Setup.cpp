/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 9, 2018
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#include <drivers/cs_Serial.h>
#include <processing/cs_Setup.h>

Setup::Setup() {
	EventDispatcher::getInstance().addListener(this);
	_last_record_key = 0;
}

cs_ret_code_t Setup::handleCommand(uint8_t* data, uint16_t size) {
	uint8_t mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, PersistenceMode::STRATEGY1);
	_persistenceMode = static_cast<OperationMode>(mode);
	if (_persistenceMode != OperationMode::OPERATION_MODE_SETUP) {
		LOGw("only available in setup mode");
		return ERR_NOT_AVAILABLE;
	}

	if (size != sizeof(setup_data_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	setup_data_t* setupData = (setup_data_t*) data;

	// Validate settings
	if (setupData->type != 0) {
		LOGw("type=%u", setupData->type);
		return ERR_WRONG_PARAMETER;
	}
	if (setupData->id == 0) {
		LOGw("id=0");
		return ERR_WRONG_PARAMETER;
	}
	// TODO: check setupData->meshAccessAddress
	// TODO: check setupData->ibeaconUuid
	if (setupData->ibeaconMajor == 0) {
		LOGw("major=0");
		return ERR_WRONG_PARAMETER;
	}
	if (setupData->ibeaconMinor == 0) {
		LOGw("minor=0");
		return ERR_WRONG_PARAMETER;
	}

	// Save all settings, use a padded struct.
	padded_setup_data_t setup_data;
	setup_data.data = *setupData;
	setup_data_t & sd = setup_data.data;

	LOGd("Store keys, mesh address, and other config data");
	st_value_t value;
	State &state = State::getInstance();
	value.u32 = 0;
	value.u8 = sd.id;
	state.set(CS_TYPE::CONFIG_CROWNSTONE_ID, &value, sizeof(value), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_KEY_ADMIN, sd.adminKey, sizeof(sd.adminKey), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_KEY_MEMBER, sd.memberKey, sizeof(sd.memberKey), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_KEY_GUEST, sd.guestKey, sizeof(sd.guestKey), PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_MESH_ACCESS_ADDRESS, &(sd.meshAccessAddress), sizeof(sd.meshAccessAddress), 
			PersistenceMode::STRATEGY1);
	state.set(CS_TYPE::CONFIG_IBEACON_UUID, &(sd.ibeaconUuid.uuid128), sizeof(sd.ibeaconUuid), 
			PersistenceMode::STRATEGY1);
	value.u32 = 0;
	value.u16 = sd.ibeaconMajor;
	state.set(CS_TYPE::CONFIG_IBEACON_MAJOR, &value, sizeof(value), PersistenceMode::STRATEGY1);
	value.u32 = 0;
	value.u16 = sd.ibeaconMinor;
	state.set(CS_TYPE::CONFIG_IBEACON_MINOR, &value, sizeof(value), PersistenceMode::STRATEGY1);

	// Set operation mode to normal mode
	_persistenceMode = OperationMode::OPERATION_MODE_NORMAL;
	mode = to_underlying_type(_persistenceMode);
	value.u32 = 0;
	value.u8 = mode;
	_last_record_key = to_underlying_type(CS_TYPE::STATE_OPERATION_MODE);
	LOGi("Set mode NORMAL");
	LOGi("Set mode %x", value.u32);
	state.set(CS_TYPE::STATE_OPERATION_MODE, &value, sizeof(value), PersistenceMode::FLASH);

	// Switch relay on
	event_t event0(CS_TYPE::EVT_POWER_ON);
	EventDispatcher::getInstance().dispatch(event0);

	// Reboot will be done when persistent storage is done.
	
//	_setupDone = true;

//	event_t event1(CS_TYPE::EVT_CROWNSTONE_SWITCH_MODE, (void*)&_persistenceMode, sizeof(_persistenceMode));
//	EventDispatcher::getInstance().dispatch(event1);

	LOGi("Setup completed");
	return ERR_WAIT_FOR_SUCCESS;
}

/**
 * This event handler only listens to one event, namely EVT_STORAGE_WRITE_DONE. In that case it will continue with
 * a reset procedure. However, how does it know that the right item has been written? What should be done instead is
 * just calling a reset procedure that - in a loop - queries if there are peripherals still busy. As soon as for
 * example the entire storage queue has been handled, it should perform the reset.
 */
void Setup::handleEvent(event_t & event) {
	// we want to react to the last write request (OPERATION_MODE_NORMAL)
	if (event.type != CS_TYPE::EVT_ADVERTISEMENT_UPDATED) {
		LOGnone("Setup received %s [%i]", TypeName(event.type), to_underlying_type(event.type));
	}
	switch (event.type) {
	case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
		uint8_t record_key = *(uint8_t*)event.data;

		if (record_key == _last_record_key) {
			LOGi("Setup done... Reset crownstone");
			// set char value
			event_t event1(CS_TYPE::EVT_SETUP_DONE);
			EventDispatcher::getInstance().dispatch(event1);
	
			State &state = State::getInstance();
			uint8_t mode = 0;
			state.get(CS_TYPE::STATE_OPERATION_MODE, &mode, PersistenceMode::FLASH);
			LOGd("New mode is %x", mode);
			OperationMode _tmpOperationMode = static_cast<OperationMode>(mode);
			LOGd("Operation mode: %s", TypeName(_tmpOperationMode));
			if (!ValidMode(_tmpOperationMode)) {
				LOGe("Invalid operation mode!");
				// for now continue with reset (will be considered setup mode)
			}

			// reset after 1000 ms
			evt_do_reset_delayed_t payload;
			payload.resetCode = GPREGRET_SOFT_RESET;
			payload.delayMs = 1000;
			event_t event2(CS_TYPE::EVT_DO_RESET_DELAYED, &payload, sizeof(payload));
			EventDispatcher::getInstance().dispatch(event2);
		} else {
			LOGnone("Compare key %i with %i", record_key, _last_record_key);
		}
		break;
	}
	default: {
		//LOGd("Setup receives event %s", TypeName(event.type));
	}
	}
}

