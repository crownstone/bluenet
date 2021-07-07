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
#include <logging/cs_Logger.h>
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
};

void ServiceData::init(uint8_t deviceType) {
	// Cache the operation mode.
	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	_operationMode = getOperationMode(mode);

	_externalStates.init();

	// Init flags
	_flags.asInt = 0;
	_flags.flags.markedDimmable = State::getInstance().isTrue(CS_TYPE::CONFIG_DIMMING_ALLOWED);
	_flags.flags.switchLocked = State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED);
	_flags.flags.switchcraft = State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED);
	_flags.flags.tapToToggle = State::getInstance().isTrue(CS_TYPE::CONFIG_TAP_TO_TOGGLE_ENABLED);

	_extraFlags.asInt = 0;

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

		// Init microapp service data.
		_microappServiceData.id = crownstoneId;
		_microappServiceData.validation = SERVICE_DATA_VALIDATION;
		_microappServiceData.flags.asInt = 0;
	}

	// Init the timer: we want to update the advertisement packet on a fixed interval.
	_updateTimerData = { {0} };
	_updateTimerId = &_updateTimerData;
	Timer::getInstance().createSingleShot(_updateTimerId, (app_timer_timeout_handler_t)ServiceData::staticTimeout);

	// Set the initial advertisement (timer has to be initialized before).
	updateServiceData(true);

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

void ServiceData::updateTemperature(int8_t temperature) {
	_temperature = temperature;
}

uint8_t* ServiceData::getArray() {
	return _serviceData.array;
}

uint16_t ServiceData::getArraySize() {
	return sizeof(service_data_t);
}



void ServiceData::updateServiceData(bool initial) {
	Timer::getInstance().stop(_updateTimerId);

	_updateCount++;

	uint32_t timestamp = SystemTime::posix();

	// Update the state errors.
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &_stateErrors, sizeof(_stateErrors));

	// Update flags.
	_flags.flags.timeSet = (timestamp != 0);
	_flags.flags.error = _stateErrors.asInt != 0;

	// Set timestamp of first error.
	if (_stateErrors.asInt == 0) {
		_firstErrorTimestamp = 0;
	}
	else if (_firstErrorTimestamp != 0) {
		_firstErrorTimestamp = timestamp;
	}

	bool encrypt = fillServiceData(timestamp);

#ifdef PRINT_DEBUG_EXTERNAL_DATA
	_log(SERIAL_DEBUG, false, "servideData: ");
	_logArray(SERIAL_DEBUG, true, _serviceData.array, sizeof(service_data_t));
//		LOGd("serviceData: type=%u id=%u switch=%u bitmask=%u temp=%i P=%i E=%i time=%u", serviceData->params.type, serviceData->params.crownstoneId, serviceData->params.switchState, serviceData->params.flagBitmask, serviceData->params.temperature, serviceData->params.powerUsageReal, serviceData->params.accumulatedEnergy, serviceData->params.partialTimestamp);
#endif

	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_SERVICE_DATA, _serviceData.array, sizeof(_serviceData.array));

	if (encrypt && State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED)) {
		encryptServiceData();
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



bool ServiceData::fillServiceData(uint32_t timestamp) {
	bool serviceDataSet = false;

	TYPIFY(STATE_HUB_MODE) hubMode;
	State::getInstance().get(CS_TYPE::STATE_HUB_MODE, &hubMode, sizeof(hubMode));
	if (hubMode) {
		// In hub mode, only use hub state as service data.
		fillWithHubState(timestamp);
		return _operationMode != OperationMode::OPERATION_MODE_SETUP;
	}

	if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
		// In setup mode, only use setup state as service data.
		fillWithSetupState(timestamp);
		return false;
	}

	// Every 2 updates, we advertise the errors (if any), or the state of another crownstone.
	if (_updateCount % 2 == 0) {
		if (_stateErrors.asInt != 0) {
			fillWithError(timestamp);
			return true;
		}

		// Interleave microapp data with state of other crownstones.
		if (_updateCount % 4 == 0) {
			serviceDataSet = fillWithMicroapp(timestamp);
		}
		if (!serviceDataSet) {
			serviceDataSet = fillWithExternalState();
		}
	}

	// If service data wasn't filled with external state
	if (!serviceDataSet) {
		if (_updateCount % 16 == 1) {
			fillWithAlternativeState(timestamp);
		}
		else {
			fillWithState(timestamp);
		}
	}
	return true;
}



void ServiceData::encryptServiceData() {
	// encrypt the array using the guest key ECB if encryption is enabled.
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



void ServiceData::fillWithSetupState(uint32_t timestamp) {
	_serviceData.params.type = SERVICE_DATA_TYPE_SETUP;
	_serviceData.params.setup.type = SERVICE_DATA_DATA_TYPE_STATE;
	_serviceData.params.setup.state.switchState = _switchState;
	_serviceData.params.setup.state.flags = _flags;
	_serviceData.params.setup.state.temperature = _temperature;
	_serviceData.params.setup.state.powerFactor = _powerFactor;
	_serviceData.params.setup.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
	_serviceData.params.setup.state.errors = _stateErrors.asInt;
	_serviceData.params.setup.state.counter = _updateCount;
	memset(_serviceData.params.setup.state.reserved, 0, sizeof(_serviceData.params.setup.state.reserved));
}

void ServiceData::fillWithState(uint32_t timestamp) {
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

void ServiceData::fillWithError(uint32_t timestamp) {
	_serviceData.params.type = SERVICE_DATA_TYPE_ENCRYPTED;
	_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_ERROR;
	_serviceData.params.encrypted.error.id = _crownstoneId;
	_serviceData.params.encrypted.error.errors = _stateErrors.asInt;
	_serviceData.params.encrypted.error.timestamp = _firstErrorTimestamp;
	_serviceData.params.encrypted.error.flags = _flags;
	_serviceData.params.encrypted.error.temperature = _temperature;
	_serviceData.params.encrypted.error.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
	_serviceData.params.encrypted.error.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
}

bool ServiceData::fillWithExternalState() {
	service_data_encrypted_t* extState = _externalStates.getNextState();
	if (extState == NULL) {
		return false;
	}
	memcpy(&_serviceData.params.encrypted, extState, sizeof(*extState));
	return true;
}

void ServiceData::fillWithAlternativeState(uint32_t timestamp) {
	TYPIFY(STATE_BEHAVIOUR_MASTER_HASH) behaviourHash;
	State::getInstance().get(CS_TYPE::STATE_BEHAVIOUR_MASTER_HASH, &behaviourHash, sizeof(behaviourHash));
	_serviceData.params.type = SERVICE_DATA_TYPE_ENCRYPTED;
	_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_ALTERNATIVE_STATE;
	_serviceData.params.encrypted.altState.id = _crownstoneId;
	_serviceData.params.encrypted.altState.switchState = _switchState;
	_serviceData.params.encrypted.altState.flags = _flags;
	_serviceData.params.encrypted.altState.behaviourMasterHash = getPartialBehaviourHash(behaviourHash);

	TYPIFY(STATE_ASSET_FILTERS_VERSION) filtersVersion;
	State::getInstance().get(CS_TYPE::STATE_ASSET_FILTERS_VERSION, &filtersVersion, sizeof(filtersVersion));
	_serviceData.params.encrypted.altState.assetFiltersVersion = filtersVersion.masterVersion;
	_serviceData.params.encrypted.altState.assetFiltersCrc = filtersVersion.masterCrc;

//	_serviceData.params.encrypted.altState.reserved = {0};
	_serviceData.params.encrypted.altState.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
	_serviceData.params.encrypted.altState.reserved2 = 0;
	_serviceData.params.encrypted.altState.validation = SERVICE_DATA_VALIDATION;
}

void ServiceData::fillWithHubState(uint32_t timestamp) {
	service_data_hub_state_t* serviceDataHubState = nullptr;
	if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
		_serviceData.params.type = SERVICE_DATA_TYPE_SETUP;
		_serviceData.params.setup.type = SERVICE_DATA_DATA_TYPE_HUB_STATE;
		serviceDataHubState = &(_serviceData.params.setup.hubState);
	}
	else {
		_serviceData.params.type = SERVICE_DATA_TYPE_ENCRYPTED;
		_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_HUB_STATE;
		serviceDataHubState = &(_serviceData.params.encrypted.hubState);
	}

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
}

bool ServiceData::fillWithMicroapp(uint32_t timestamp) {
	if (!_microappServiceDataSet) {
		return false;
	}
	_serviceData.params.encrypted.type = SERVICE_DATA_DATA_TYPE_MICROAPP;
	_serviceData.params.encrypted.microapp = _microappServiceData;
	// Stone ID and validation are set at init.
	// Timestamp is set when microapp service data was received.
	return true;
}

void ServiceData::handleEvent(event_t & event) {
	// Keep track of the BLE connection status. If we are connected we do not need to update the packet.
	switch (event.type) {
		case CS_TYPE::EVT_BLE_CONNECT: {
			LOGd("Connected");
			_connected = true;
			break;
		}
		case CS_TYPE::EVT_BLE_DISCONNECT: {
			LOGd("Disconnected");
			_connected = false;
			updateServiceData(false);
			break;
		}
		case CS_TYPE::STATE_ERRORS: {
			LOGd("Event: $typeName(%u)", event.type);
			state_errors_t* stateErrors = (TYPIFY(STATE_ERRORS)*) event.data;
			_flags.flags.error = stateErrors->asInt;
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
			_extraFlags.flags.behaviourEnabled = behaviourSettings->flags.enabled;
			break;
		}
		case CS_TYPE::EVT_TIME_SET: {
			_flags.flags.timeSet = true;
			break;
		}
		case CS_TYPE::EVT_DIMMER_POWERED: {
			_flags.flags.dimmingReady = *reinterpret_cast<TYPIFY(EVT_DIMMER_POWERED)*>(event.data);
			break;
		}
		case CS_TYPE::CONFIG_DIMMING_ALLOWED: {
			_flags.flags.markedDimmable = *reinterpret_cast<TYPIFY(CONFIG_DIMMING_ALLOWED)*>(event.data);
			break;
		}
		case CS_TYPE::CONFIG_SWITCH_LOCKED: {
			_flags.flags.switchLocked = *reinterpret_cast<TYPIFY(CONFIG_SWITCH_LOCKED)*>(event.data);
			break;
		}
		case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED: {
			_flags.flags.switchcraft = *reinterpret_cast<TYPIFY(CONFIG_SWITCHCRAFT_ENABLED)*>(event.data);
			break;
		}
		case CS_TYPE::CONFIG_TAP_TO_TOGGLE_ENABLED: {
			_flags.flags.tapToToggle = *reinterpret_cast<TYPIFY(CONFIG_TAP_TO_TOGGLE_ENABLED)*>(event.data);
			break;
		}
		case CS_TYPE::EVT_BEHAVIOUR_OVERRIDDEN: {
			_flags.flags.behaviourOverridden = *reinterpret_cast<TYPIFY(EVT_BEHAVIOUR_OVERRIDDEN)*>(event.data);
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
		case CS_TYPE::CMD_MICROAPP_ADVERTISE: {
			TYPIFY(CMD_MICROAPP_ADVERTISE)* advertise = reinterpret_cast<TYPIFY(CMD_MICROAPP_ADVERTISE)*>(event.data);
			if (advertise->type != 0) {
				break;
			}
			if (advertise->data.len == 0) {
				break;
			}
			_microappServiceData.appUuid = advertise->appUuid;

			memset(_microappServiceData.data, 0, sizeof(_microappServiceData.data));
			cs_buffer_size_t dataSize = std::min(advertise->data.len, static_cast<uint16_t>(sizeof(_microappServiceData.data)));
			memcpy(_microappServiceData.data, advertise->data.data, dataSize);


			uint32_t timestamp = SystemTime::posix();
			_microappServiceData.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
			_microappServiceData.flags.flags.timeSet = _flags.flags.timeSet;

			LOGd("Updated microapp data: uuid=%u timestamp=%u", _microappServiceData.appUuid, _microappServiceData.partialTimestamp);
			_microappServiceDataSet = true;
			break;
		}
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
		packet.flags = _flags.asInt;
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
