/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 4, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_ServiceData.h>
#include <cfg/cs_DeviceTypes.h>
#include <drivers/cs_RNG.h>
#include <drivers/cs_RTC.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Temperature.h>
#include <encryption/cs_AES.h>
#include <encryption/cs_KeysAndAccess.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <uart/cs_UartConnection.h>
#include <uart/cs_UartHandler.h>
#include <util/cs_Utils.h>

//#define PRINT_DEBUG_EXTERNAL_DATA
//#define PRINT_VERBOSE_EXTERNAL_DATA

ServiceData::ServiceData() {
//	_stateErrors.asInt = 0;
	// Initialize the service data
	memset(_serviceData.array, 0, sizeof(_serviceData.array));
	assert(sizeof(service_data_encrypted_t) == AES_BLOCK_SIZE, "Size of service_data_encrypted_t must be 1 block.");
	assert(sizeof(service_data_micro_app_encrypted_t) == AES_BLOCK_SIZE, "Size of service_data_micro_app_encrypted_t must be 1 block.");
};

void ServiceData::init(uint8_t deviceType) {
	// Cache the operation mode.
	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	_operationMode = getOperationMode(mode);

	_externalStates.init();

	// Init flags
	updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, State::getInstance().isTrue(CS_TYPE::CONFIG_PWM_ALLOWED));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCHCRAFT_ENABLED, State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_TAP_TO_TOGGLE_ENABLED, State::getInstance().isTrue(CS_TYPE::CONFIG_TAP_TO_TOGGLE_ENABLED));

	// Set the device type.
	TYPIFY(STATE_HUB_MODE) hubMode;
	State::getInstance().get(CS_TYPE::STATE_HUB_MODE, &hubMode, sizeof(hubMode));
	if (hubMode) {
		LOGd("Set device type hub");
		setDeviceType(DEVICE_CROWNSTONE_HUB);
	}
	else {
		setDeviceType(deviceType);
	}

	// Set switch state.
	TYPIFY(STATE_SWITCH_STATE) switchState;
	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	updateSwitchState(switchState.asInt);

	// Set temperature.
	updateTemperature(getTemperature());

	// Some state info is only set in normal mode.
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {

		// Set crownstone id.
		TYPIFY(CONFIG_CROWNSTONE_ID) crownstoneId;
		State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &crownstoneId, sizeof(crownstoneId));
		updateCrownstoneId(crownstoneId);

	}

	// Init the timer: we want to update the advertisement packet on a fixed interval.
	_updateTimerData = { {0} };
	_updateTimerId = &_updateTimerData;
	Timer::getInstance().createSingleShot(_updateTimerId, (app_timer_timeout_handler_t)ServiceData::staticTimeout);

	// Set the initial advertisement (timer has to be initialized before).
	updateAdvertisement(true);

	// Start the timer.
	Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);

	// Start listening for events.
	EventDispatcher::getInstance().addListener(this);
}

void ServiceData::setDeviceType(uint8_t deviceType) {
//	_deviceType = deviceType;
	_serviceData.params.deviceType = deviceType;
}

void ServiceData::updatePowerUsage(int32_t powerUsage) {
	_powerUsageReal = powerUsage;
	_powerFactor = 127;
}

void ServiceData::updateAccumulatedEnergy(int32_t energy) {
	_energyUsed = energy;
}

void ServiceData::updateCrownstoneId(uint8_t crownstoneId) {
	LOGi("Set crownstone id to %u", crownstoneId);
	_crownstoneId = crownstoneId;
}

void ServiceData::updateSwitchState(uint8_t switchState) {
	_switchState = switchState;
}

void ServiceData::updateFlagsBitmask(uint8_t bitmask) {
	_flags = bitmask;
}

void ServiceData::updateFlagsBitmask(uint8_t bit, bool set) {
	if (set) {
		BLEutil::setBit(_flags, bit);
	}
	else {
		BLEutil::clearBit(_flags, bit);
	}
}

void ServiceData::updateExtraFlagsBitmask(uint8_t bit, bool set) {
	if (set) {
		BLEutil::setBit(_extraFlags, bit);
	}
	else {
		BLEutil::clearBit(_extraFlags, bit);
	}
}

void ServiceData::updateTemperature(int8_t temperature) {
	_temperature = temperature;
}

uint8_t* ServiceData::getArray() {
	return _serviceData.array;
}

uint16_t ServiceData::getArraySize() {
	return sizeof(service_data_t);
}


void ServiceData::updateAdvertisement(bool initial) {
	Timer::getInstance().stop(_updateTimerId);

	_updateCount++;

	bool serviceDataSet = false;

	uint32_t timestamp = SystemTime::posix();
	updateFlagsBitmask(SERVICE_DATA_FLAGS_TIME_SET, timestamp != 0);

//		// Update flag
//		updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, State::getInstance().isTrue(CS_TYPE::CONFIG_PWM_ALLOWED));
//		updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED));

	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_ERROR, stateErrors.asInt);

	// Set error timestamp
	if (stateErrors.asInt == 0) {
		_firstErrorTimestamp = 0;
	}
	else if (_firstErrorTimestamp != 0) {
		_firstErrorTimestamp = timestamp;
	}

	if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
		// In setup mode, only advertise this state.
		_serviceData.params.type = SERVICE_DATA_TYPE_SETUP;
		_serviceData.params.setup.type = 0;
		_serviceData.params.setup.state.switchState = _switchState;
		_serviceData.params.setup.state.flags = _flags;
		_serviceData.params.setup.state.temperature = _temperature;
		_serviceData.params.setup.state.powerFactor = _powerFactor;
		_serviceData.params.setup.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
		_serviceData.params.setup.state.errors = stateErrors.asInt;
		_serviceData.params.setup.state.counter = _updateCount;
		memset(_serviceData.params.setup.state.reserved, 0, sizeof(_serviceData.params.setup.state.reserved));
		serviceDataSet = true;
	}

	// Every 2 updates, we advertise the errors (if any), or the state of another crownstone.
	if (!serviceDataSet && _updateCount % 2 == 0) {
		if (stateErrors.asInt != 0) {
			_serviceData.params.type = SERVICE_DATA_TYPE_ENCRYPTED;
			_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_ERROR;
			_serviceData.params.encrypted.error.id = _crownstoneId;
			_serviceData.params.encrypted.error.errors = stateErrors.asInt;
			_serviceData.params.encrypted.error.timestamp = _firstErrorTimestamp;
			_serviceData.params.encrypted.error.flags = _flags;
			_serviceData.params.encrypted.error.temperature = _temperature;
			_serviceData.params.encrypted.error.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
			_serviceData.params.encrypted.error.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
			serviceDataSet = true;
		}
		else if (getExternalAdvertisement(_crownstoneId, _serviceData)) {
			serviceDataSet = true;
		}
	}

	TYPIFY(STATE_HUB_MODE) hubMode;
	State::getInstance().get(CS_TYPE::STATE_HUB_MODE, &hubMode, sizeof(hubMode));
	if (hubMode) {
		service_data_hub_state_t* serviceDataHubState = nullptr;
		if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
			_serviceData.params.type = SERVICE_DATA_TYPE_SETUP;
			serviceDataHubState = &(_serviceData.params.setup.hubState);
		}
		else {
			_serviceData.params.type = SERVICE_DATA_TYPE_ENCRYPTED;
			serviceDataHubState = &(_serviceData.params.encrypted.hubState);
		}

		_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_HUB_STATE;
		serviceDataHubState->id = _crownstoneId;
		auto selfFlags = UartConnection::getInstance().getSelfStatus().flags.flags;
		auto hubFlags = UartConnection::getInstance().getUserStatus().flags.flags;

		serviceDataHubState->flags.asInt = 0;
		serviceDataHubState->flags.flags.uartAlive = UartConnection::getInstance().isAlive();
		serviceDataHubState->flags.flags.uartAliveEncrypted = UartConnection::getInstance().isEncryptedAlive();
		serviceDataHubState->flags.flags.uartEncryptionRequiredByStone = selfFlags.encryptionRequired;
		serviceDataHubState->flags.flags.uartEncryptionRequiredByHub = hubFlags.encryptionRequired;
		serviceDataHubState->flags.flags.hasBeenSetUp = hubFlags.hasBeenSetUp;
		serviceDataHubState->flags.flags.hasInternet = hubFlags.hasInternet;
		serviceDataHubState->flags.flags.hasError = hubFlags.hasError;

		auto hubData = UartConnection::getInstance().getUserStatus();
		if (hubData.type == UART_HUB_DATA_TYPE_CROWNSTONE_HUB) {
			memcpy(serviceDataHubState->hubData, hubData.data, SERVICE_DATA_HUB_DATA_SIZE);
		}
		else {
			memset(serviceDataHubState->hubData, 0, SERVICE_DATA_HUB_DATA_SIZE);
		}
		serviceDataHubState->partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
		serviceDataHubState->reserved = 0;
		serviceDataHubState->validation = SERVICE_DATA_VALIDATION;
		serviceDataSet = true;
	}

	if (!serviceDataSet) {
		if (_updateCount % 16 == 0) {
			TYPIFY(STATE_BEHAVIOUR_MASTER_HASH) behaviourHash;
			State::getInstance().get(CS_TYPE::STATE_BEHAVIOUR_MASTER_HASH, &behaviourHash, sizeof(behaviourHash));
			_serviceData.params.type = SERVICE_DATA_TYPE_ENCRYPTED;
			_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_ALTERNATIVE_STATE;
			_serviceData.params.encrypted.altState.id = _crownstoneId;
			_serviceData.params.encrypted.altState.switchState = _switchState;
			_serviceData.params.encrypted.altState.flags = _flags;
			_serviceData.params.encrypted.altState.behaviourMasterHash = getPartialBehaviourHash(behaviourHash);
			memset(_serviceData.params.encrypted.altState.reserved, 0, sizeof(_serviceData.params.encrypted.altState.reserved));
//			_serviceData.params.encrypted.altState.reserved = {0};
			_serviceData.params.encrypted.altState.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
			_serviceData.params.encrypted.altState.reserved2 = 0;
			_serviceData.params.encrypted.altState.validation = SERVICE_DATA_VALIDATION;
		}
		else {
			_serviceData.params.type = SERVICE_DATA_TYPE_ENCRYPTED;
			_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_STATE;
			_serviceData.params.encrypted.state.id = _crownstoneId;
			_serviceData.params.encrypted.state.switchState = _switchState;
			_serviceData.params.encrypted.state.flags = _flags;
			_serviceData.params.encrypted.state.temperature = _temperature;
			_serviceData.params.encrypted.state.powerFactor = _powerFactor;
			_serviceData.params.encrypted.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
			_serviceData.params.encrypted.state.energyUsed = _energyUsed;
			_serviceData.params.encrypted.state.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
			_serviceData.params.encrypted.state.extraFlags = _extraFlags;
			_serviceData.params.encrypted.state.validation = SERVICE_DATA_VALIDATION;
		}
	}

#ifdef PRINT_DEBUG_EXTERNAL_DATA
	LOGd("servideData:");
	BLEutil::printArray(_serviceData.array, sizeof(service_data_t));
//		LOGd("serviceData: type=%u id=%u switch=%u bitmask=%u temp=%i P=%i E=%i time=%u", serviceData->params.type, serviceData->params.crownstoneId, serviceData->params.switchState, serviceData->params.flagBitmask, serviceData->params.temperature, serviceData->params.powerUsageReal, serviceData->params.accumulatedEnergy, serviceData->params.partialTimestamp);
#endif
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_SERVICE_DATA, _serviceData.array, sizeof(service_data_t));

//		Mesh::getInstance().printRssiList();

	// encrypt the array using the guest key ECB if encryption is enabled.
	if (State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED) && _operationMode != OperationMode::OPERATION_MODE_SETUP) {

		uint8_t key[ENCRYPTION_KEY_LENGTH];
		if (KeysAndAccess::getInstance().getKey(SERVICE_DATA, key, sizeof(key))) {
			cs_buffer_size_t writtenSize;
			AES::getInstance().encryptEcb(
					cs_data_t(key, sizeof(key)),
					cs_data_t(),
					cs_data_t(_serviceData.params.encryptedArray, sizeof(_serviceData.params.encryptedArray)),
					cs_data_t(_serviceData.params.encryptedArray, sizeof(_serviceData.params.encryptedArray)),
					writtenSize);
		}
	}

	if (!initial) {
		event_t event(CS_TYPE::EVT_ADVERTISEMENT_UPDATED);
		EventDispatcher::getInstance().dispatch(event);
	}

	// start the timer again.
	if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
		Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD_SETUP), this);
	}
	else {
		Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);
	}
}

bool ServiceData::getExternalAdvertisement(stone_id_t ownId, service_data_t& serviceData) {
	service_data_encrypted_t* extState = _externalStates.getNextState();
	if (extState == NULL) {
		return false;
	}
	memcpy(&serviceData.params.encrypted, extState, sizeof(*extState));
	return true;
}


void ServiceData::handleEvent(event_t & event) {
	// Keep track of the BLE connection status. If we are connected we do not need to update the packet.
	switch(event.type) {
		case CS_TYPE::EVT_BLE_CONNECT: {
			LOGd("Connected");
			_connected = true;
			break;
		}
		case CS_TYPE::EVT_BLE_DISCONNECT: {
			LOGd("Disconnected");
			_connected = false;
			updateAdvertisement(false);
			break;
		}
//		// TODO: do we need to keep up the error? Or we can simply retrieve it every service data update?
//		case CS_TYPE::EVT_DIMMER_FORCED_OFF:
//		case CS_TYPE::EVT_SWITCH_FORCED_OFF:
//		case CS_TYPE::EVT_RELAY_FORCED_ON:
//			LOGd("Event: $typeName(%u)", event.type);
//			updateFlagsBitmask(SERVICE_DATA_FLAGS_ERROR, true);
//			break;
		case CS_TYPE::STATE_ERRORS: {
			LOGd("Event: $typeName(%u)", event.type);
			state_errors_t* stateErrors = (TYPIFY(STATE_ERRORS)*) event.data;
			updateFlagsBitmask(SERVICE_DATA_FLAGS_ERROR, stateErrors->asInt);
			break;
		}
		case CS_TYPE::CONFIG_CROWNSTONE_ID: {
			updateCrownstoneId(*(TYPIFY(CONFIG_CROWNSTONE_ID)*)event.data);
			break;
		}
		case CS_TYPE::STATE_SWITCH_STATE: {
			switch_state_t* state = (TYPIFY(STATE_SWITCH_STATE)*)event.data;
			updateSwitchState(state->asInt);
//			sendMeshState(true);
			// Abuse the state update timeout.
			_sendStateCountdown = 300 / TICK_INTERVAL_MS;
			break;
		}
		case CS_TYPE::STATE_ACCUMULATED_ENERGY: {
			int32_t energyUsed = (*(TYPIFY(STATE_ACCUMULATED_ENERGY)*)event.data) / 1000 / 1000 / 64;
			updateAccumulatedEnergy(energyUsed);
			// todo create mesh state event if changes significantly
			break;
		}
		case CS_TYPE::STATE_POWER_USAGE: {
			updatePowerUsage(*(TYPIFY(STATE_POWER_USAGE)*)event.data);
			// todo create mesh state event if changes significantly
			break;
		}
		case CS_TYPE::STATE_TEMPERATURE: {
			updateTemperature(*(TYPIFY(STATE_TEMPERATURE)*)event.data);
			break;
		}
		case CS_TYPE::STATE_BEHAVIOUR_SETTINGS: {
			TYPIFY(STATE_BEHAVIOUR_SETTINGS)* behaviourSettings = (TYPIFY(STATE_BEHAVIOUR_SETTINGS)*)event.data;
			updateExtraFlagsBitmask(SERVICE_DATA_EXTRA_FLAGS_BEHAVIOUR_ENABLED, behaviourSettings->flags.enabled);
			break;
		}
		case CS_TYPE::EVT_TIME_SET: {
			updateFlagsBitmask(SERVICE_DATA_FLAGS_TIME_SET, true);
			break;
		}
		case CS_TYPE::EVT_DIMMER_POWERED: {
			updateFlagsBitmask(SERVICE_DATA_FLAGS_DIMMING_AVAILABLE, *(TYPIFY(EVT_DIMMER_POWERED)*)event.data);
			break;
		}
		case CS_TYPE::CONFIG_PWM_ALLOWED: {
			updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, *(TYPIFY(CONFIG_PWM_ALLOWED)*)event.data);
			break;
		}
		case CS_TYPE::CONFIG_SWITCH_LOCKED: {
			updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, *(TYPIFY(CONFIG_SWITCH_LOCKED)*)event.data);
			break;
		}
		case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED: {
			updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCHCRAFT_ENABLED, *(TYPIFY(CONFIG_SWITCHCRAFT_ENABLED)*)event.data);
			break;
		}
		case CS_TYPE::CONFIG_TAP_TO_TOGGLE_ENABLED: {
			updateFlagsBitmask(SERVICE_DATA_FLAGS_TAP_TO_TOGGLE_ENABLED, *(TYPIFY(CONFIG_TAP_TO_TOGGLE_ENABLED)*)event.data);
			break;
		}
		case CS_TYPE::EVT_BEHAVIOUR_OVERRIDDEN: {
			updateFlagsBitmask(SERVICE_DATA_FLAGS_BEHAVIOUR_OVERRIDDEN, *(TYPIFY(EVT_BEHAVIOUR_OVERRIDDEN)*)event.data);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			TYPIFY(EVT_TICK) tickCount = *(TYPIFY(EVT_TICK)*)event.data;
			_externalStates.tick(tickCount);
			if (_sendStateCountdown-- == 0) {
				uint32_t randMs = MESH_SEND_STATE_INTERVAL_MS + RNG::getInstance().getRandom8() * MESH_SEND_STATE_INTERVAL_MS_VARIATION / 255;
				_sendStateCountdown = randMs / TICK_INTERVAL_MS;
				sendMeshState(false);
			}
			break;
		}
		case CS_TYPE::EVT_STATE_EXTERNAL_STONE: {
			TYPIFY(EVT_STATE_EXTERNAL_STONE)* extState = (TYPIFY(EVT_STATE_EXTERNAL_STONE)*)event.data;
			_externalStates.receivedState(extState);
			break;
		}
		case CS_TYPE::STATE_HUB_MODE: {
			TYPIFY(STATE_HUB_MODE)* hubMode = reinterpret_cast<TYPIFY(STATE_HUB_MODE)*>(event.data);
			if (*hubMode) {
				LOGd("Set device type hub");
				setDeviceType(DEVICE_CROWNSTONE_HUB);
			}
			else {
				LOGw("Reboot to set normal device type again..");
			}
			break;
		}
		// TODO: add bitmask events
		default:
			return;
	}
}

int16_t ServiceData::compressPowerUsageMilliWatt(int32_t powerUsageMW) {
	// units of 1/8 W
	int16_t retVal = powerUsageMW / 125; // similar to *8/1000, but then without chance to overflow
	return retVal;
}

int32_t ServiceData::decompressPowerUsage(int16_t compressedPowerUsage) {
	int32_t retVal = compressedPowerUsage * 125; // similar to /8*1000, but then without losing precision.
	return retVal;
}

int32_t ServiceData::convertEnergyV3ToV1(int32_t energyUsed) {
	int32_t retVal = ((int64_t)energyUsed) * 64 / 3600;
	return retVal;
}

int32_t ServiceData::convertEnergyV1ToV3(int32_t energyUsed) {
	int32_t retVal = ((int64_t)energyUsed) * 3600 / 64;
	return retVal;
}

uint16_t ServiceData::energyToPartialEnergy(int32_t energyUsage) {
//	uint16_t retVal = energyUsage & 0x0000FFFF; // TODO: also drops the sign
	uint16_t retVal = abs(energyUsage) % (UINT16_MAX+1); // Safer
	return retVal;
}

int32_t ServiceData::partialEnergyToEnergy (uint16_t partialEnergy) {
	int32_t retVal = partialEnergy; // TODO: what else can we do?
	return retVal;
}

uint16_t ServiceData::timestampToPartialTimestamp(uint32_t timestamp) {
//	uint16_t retVal = timestamp & 0x0000FFFF;
	uint16_t retVal = timestamp % (UINT16_MAX+1); // Safer
	return retVal;
}

uint16_t ServiceData::getPartialTimestampOrCounter(uint32_t timestamp, uint32_t counter) {
	if (timestamp == 0) {
		return counter;
	}
	return timestampToPartialTimestamp(timestamp);
}

uint16_t ServiceData::getPartialBehaviourHash(uint32_t behaviourHash) {
	// The most significant bytes are order dependent.
	return behaviourHash >> 16;
}

void ServiceData::sendMeshState(bool event) {
//#ifdef BUILD_MESHING
	LOGi("sendMeshState");
	uint32_t timestamp = SystemTime::posix();

	cs_mesh_msg_t meshMsg;
	if (event) {
		meshMsg.reliability = CS_MESH_RELIABILITY_MEDIUM;
		meshMsg.urgency = CS_MESH_URGENCY_HIGH;
	}
	else {
		meshMsg.reliability = CS_MESH_RELIABILITY_LOW;
		meshMsg.urgency = CS_MESH_URGENCY_LOW;
	}
	{
		cs_mesh_model_msg_state_1_t packet;
		packet.temperature = _temperature;
		packet.energyUsed = _energyUsed;
		packet.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);

		meshMsg.type = CS_MESH_MODEL_TYPE_STATE_1;
		meshMsg.payload = (uint8_t*)&packet;
		meshMsg.size = sizeof(packet);

		event_t cmd(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
		EventDispatcher::getInstance().dispatch(cmd);
	}
	{
		cs_mesh_model_msg_state_0_t packet;
		packet.switchState = _switchState;
		packet.flags = _flags;
		packet.powerFactor = _powerFactor;
		packet.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
		packet.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);

		meshMsg.type = CS_MESH_MODEL_TYPE_STATE_0;
		meshMsg.payload = (uint8_t*)&packet;
		meshMsg.size = sizeof(packet);

		event_t cmd(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
		EventDispatcher::getInstance().dispatch(cmd);
	}
//#endif
}
