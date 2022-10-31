/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 20, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourStore.h>
#include <cs_Crownstone.h>
#include <events/cs_Event.h>
#include <test/cs_MemUsageTest.h>
#include <tracking/cs_TrackedDevices.h>

MemUsageTest::MemUsageTest(const boards_config_t& boardsConfig) : _boardConfig(boardsConfig) {}

void MemUsageTest::start() {
	_started = true;
}

void MemUsageTest::onTick() {
	if (!_started) {
		return;
	}

	if (setNextStateType() == false) {
		printRamStats();
		return;
	}

	if (sendNextPresence() == false) {
		printRamStats();
		return;
	}

	if (sendNextRssiData() == false) {
		printRamStats();
		return;
	}

	if (setNextTrackedDevice() == false) {
		printRamStats();
		return;
	}

	if (setNextBehaviour() == false) {
		printRamStats();
		return;
	}
}

void MemUsageTest::printRamStats() {
	Crownstone::updateHeapStats();
	Crownstone::updateMinStackEnd();
	Crownstone::printLoadStats();
}

bool MemUsageTest::setNextBehaviour() {
	// Can't add all behaviours at once, as that prints too much and crashes the firmware.
//	if (_behaviourIndex >= (int)BehaviourStore::MaxBehaviours) {
	if (_behaviourIndex >= 10) {
		return true;
	}
	cs_result_t result;

	// ExtenedSwitchBehaviour is the largest behaviour type.
	ExtendedSwitchBehaviour behaviour(
			SwitchBehaviour(
					100,
					0,
					0xFF,
					TimeOfDay(2 * _behaviourIndex),
					TimeOfDay(2 * _behaviourIndex + 1),
					PresenceCondition(
							PresencePredicate(PresencePredicate::Condition::VacuouslyTrue, PresenceStateDescription()),
							0)),
			PresenceCondition(
					PresencePredicate(PresencePredicate::Condition::VacuouslyTrue, PresenceStateDescription()), 0));
	uint8_t buf[behaviour.serializedSize()];
	behaviour.serialize(buf, sizeof(buf));

	LOGd("Add behaviour: serializedSize=%u classSize=%u", sizeof(buf), sizeof(behaviour));
	event_t event(CS_TYPE::CMD_ADD_BEHAVIOUR, buf, sizeof(buf), result);
	event.dispatch();
	LOGd("result=%u", event.result.returnCode);

	++_behaviourIndex;
	return false;
}

bool MemUsageTest::sendNextRssiData() {
	stone_id_t maxId = -1;
	if (_rssiDataStoneId > maxId) {
		return true;
	}
	LOGi("sendNextRssiData id=%i", _rssiDataStoneId);
	int untilId = _rssiDataStoneId + 1;
	if (untilId > (int)maxId + 1) {
		untilId = (int)maxId + 1;
	}

	TYPIFY(EVT_RECV_MESH_MSG) msg;
	msg.type = CS_MESH_MODEL_TYPE_UNKNOWN;
	msg.rssi = -50;
	msg.isMaybeRelayed = false;

	for (int id = _rssiDataStoneId; id < untilId; ++id) {
		for (uint8_t channel = 37; channel < 40; ++channel) {
			msg.channel    = channel;
			msg.srcStoneId = id;
			event_t event(CS_TYPE::EVT_RECV_MESH_MSG, reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
			event.dispatch();
		}
	}

	_rssiDataStoneId = untilId;
	//	++_rssiDataStoneId;
	return false;
}

bool MemUsageTest::sendNextPresence() {
	if (_presenceProfileId > PresenceHandler::ProfileLocation::MAX_PROFILE_ID) {
		return true;
	}
	LOGi("sendNextPresence profile=%i location=%i", _presenceProfileId, _presenceLocationId);

	int locationIdUntil    = _presenceLocationId + 8;
	int locationIdMaxUntil = (int)PresenceHandler::ProfileLocation::MAX_LOCATION_ID + 1;
	if (locationIdUntil > locationIdMaxUntil) {
		locationIdUntil = locationIdMaxUntil;
	}

	TYPIFY(EVT_RECEIVED_PROFILE_LOCATION) profileLocation;
	profileLocation.fromMesh  = true;
	profileLocation.simulated = false;
	profileLocation.profileId = _presenceProfileId;

	for (int locationId = _presenceLocationId; locationId < locationIdUntil; ++locationId) {
		profileLocation.locationId = locationId;
		event_t event(
				CS_TYPE::EVT_RECEIVED_PROFILE_LOCATION,
				reinterpret_cast<uint8_t*>(&profileLocation),
				sizeof(profileLocation));
		event.dispatch();
	}

	if (locationIdUntil >= locationIdMaxUntil) {
		// Next profile ID
		++_presenceProfileId;
		_presenceLocationId = 0;
	}
	else {
		_presenceLocationId = locationIdUntil;
	}
	return false;
}

bool MemUsageTest::setNextTrackedDevice() {
	if (_trackedDeviceId > TrackedDevices::MAX_TRACKED_DEVICES) {
		return true;
	}
	LOGi("setNextTrackedDevice trackedDeviceId=%i", _trackedDeviceId);

	TYPIFY(CMD_REGISTER_TRACKED_DEVICE) trackedDevice;
	trackedDevice.accessLevel                         = ADMIN;
	trackedDevice.data.deviceId                       = _trackedDeviceId;
	trackedDevice.data.deviceToken[0]                 = _trackedDeviceId;
	trackedDevice.data.flags.flags.ignoreForBehaviour = false;
	trackedDevice.data.flags.flags.tapToToggle        = false;
	trackedDevice.data.locationId                     = 0;
	trackedDevice.data.profileId                      = 0;
	trackedDevice.data.rssiOffset                     = 0;
	trackedDevice.data.timeToLiveMinutes              = 10;
	event_t event(
			CS_TYPE::CMD_REGISTER_TRACKED_DEVICE, reinterpret_cast<uint8_t*>(&trackedDevice), sizeof(trackedDevice));
	event.dispatch();

	++_trackedDeviceId;
	return false;
}

bool MemUsageTest::setNextStateType() {
	//	if (_stateType >= 0xFFFF) {
	if (_stateType >= InternalBaseBluetooth) {
		return true;
	}

	int stateTypeUntil = _stateType + 10;

	for (int stateType = _stateType; stateType < stateTypeUntil; ++stateType) {
		setNextStateType(stateType);
	}

	_stateType = stateTypeUntil;
	//	++_stateType;
	return false;
}

bool MemUsageTest::setNextStateType(int intType) {
	CS_TYPE stateType = toCsType(intType);

	if (stateType == CS_TYPE::CONFIG_DO_NOT_USE) {
		return false;
	}

	if (DefaultLocation(stateType) == PersistenceMode::NEITHER_RAM_NOR_FLASH) {
		return false;
	}

	if (TypeSize(stateType) == 0) {
		return false;
	}

	// Some types shouldn't be set
	switch (stateType) {
		// Set by setup:
		case CS_TYPE::CONFIG_SPHERE_ID:
		case CS_TYPE::CONFIG_CROWNSTONE_ID:
		case CS_TYPE::CONFIG_KEY_ADMIN:
		case CS_TYPE::CONFIG_KEY_MEMBER:
		case CS_TYPE::CONFIG_KEY_BASIC:
		case CS_TYPE::CONFIG_KEY_SERVICE_DATA:
		case CS_TYPE::CONFIG_KEY_LOCALIZATION:
		case CS_TYPE::CONFIG_MESH_DEVICE_KEY:
		case CS_TYPE::CONFIG_MESH_APP_KEY:
		case CS_TYPE::CONFIG_MESH_NET_KEY:
		case CS_TYPE::CONFIG_IBEACON_MAJOR:
		case CS_TYPE::CONFIG_IBEACON_MINOR:
		case CS_TYPE::CONFIG_IBEACON_UUID:
		// Set by system:
		case CS_TYPE::STATE_OPERATION_MODE:
		case CS_TYPE::STATE_MESH_IV_INDEX:
		case CS_TYPE::STATE_MESH_SEQ_NUMBER:
		// This messes up the uart:
		case CS_TYPE::STATE_UART_KEY:
		// These are set by the behaviour test:
		case CS_TYPE::STATE_BEHAVIOUR_RULE:
		case CS_TYPE::STATE_TWILIGHT_RULE:
		case CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE:
		case CS_TYPE::STATE_BEHAVIOUR_MASTER_HASH: return false;
		default: break;
	}

	LOGd("Set type %u", stateType);
	uint8_t* data = new uint8_t[TypeSize(stateType)];
	cs_state_data_t stateData(stateType, 0, data, TypeSize(stateType));
	getDefault(stateData, _boardConfig);
	cs_ret_code_t retCode = State::getInstance().set(stateData);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE: return true;
	}
	return false;
}
