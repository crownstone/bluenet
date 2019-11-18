/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 4, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "ble/cs_ServiceData.h"
#include "drivers/cs_RNG.h"
#include "drivers/cs_RTC.h"
#include "drivers/cs_Serial.h"
#include "processing/cs_EncryptionHandler.h"
#include "protocol/cs_UartProtocol.h"
#include "storage/cs_State.h"
#include "util/cs_Utils.h"
#include "protocol/mesh/cs_MeshModelPacketHelper.h"

#define ADVERTISE_EXTERNAL_DATA
//#define PRINT_DEBUG_EXTERNAL_DATA
//#define PRINT_VERBOSE_EXTERNAL_DATA

ServiceData::ServiceData() :
	_updateTimerId(NULL)
	,_crownstoneId(0)
	,_switchState(0)
	,_flags(0)
	,_temperature(0)
	,_powerFactor(0)
	,_powerUsageReal(0)
	,_energyUsed(0)
	,_firstErrorTimestamp(0)
	,_operationMode(OperationMode::OPERATION_MODE_UNINITIALIZED)
	,_connected(false)
	,_updateCount(0)
{
//	_stateErrors.asInt = 0;
	// Initialize the service data
	memset(_serviceData.array, 0, sizeof(_serviceData.array));
};

void ServiceData::init() {
	// we want to update the advertisement packet on a fixed interval.
	_updateTimerData = { {0} };
	_updateTimerId = &_updateTimerData;
	Timer::getInstance().createSingleShot(_updateTimerId, (app_timer_timeout_handler_t)ServiceData::staticTimeout);

	// get the operation mode from state
	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	_operationMode = getOperationMode(mode);

	_externalStates.init();

	// start the update timer
	Timer::getInstance().start(_updateTimerId, MS_TO_TICKS(ADVERTISING_REFRESH_PERIOD), this);

	// Init flags
	updateFlagsBitmask(SERVICE_DATA_FLAGS_MARKED_DIMMABLE, State::getInstance().isTrue(CS_TYPE::CONFIG_PWM_ALLOWED));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCH_LOCKED, State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED));
	updateFlagsBitmask(SERVICE_DATA_FLAGS_SWITCHCRAFT_ENABLED, State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED));

	EventDispatcher::getInstance().addListener(this);

	// set the initial advertisement.
	updateAdvertisement(true);
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

	TYPIFY(STATE_TIME) timestamp;
	State::getInstance().get(CS_TYPE::STATE_TIME, &timestamp, sizeof(timestamp));

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
		_serviceData.params.protocolVersion = SERVICE_DATA_TYPE_SETUP;
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
			_serviceData.params.protocolVersion = SERVICE_DATA_TYPE_ENCRYPTED;
			_serviceData.params.encrypted.type = SERVICE_DATA_TYPE_ERROR;
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

	if (!serviceDataSet) {
		_serviceData.params.protocolVersion = SERVICE_DATA_TYPE_ENCRYPTED;
		_serviceData.params.encrypted.type = SERVICE_DATA_TYPE_STATE;
		_serviceData.params.encrypted.state.id = _crownstoneId;
		_serviceData.params.encrypted.state.switchState = _switchState;
		_serviceData.params.encrypted.state.flags = _flags;
		_serviceData.params.encrypted.state.temperature = _temperature;
		_serviceData.params.encrypted.state.powerFactor = _powerFactor;
		_serviceData.params.encrypted.state.powerUsageReal = compressPowerUsageMilliWatt(_powerUsageReal);
		_serviceData.params.encrypted.state.energyUsed = _energyUsed;
		_serviceData.params.encrypted.state.partialTimestamp = getPartialTimestampOrCounter(timestamp, _updateCount);
		_serviceData.params.encrypted.state.reserved = 0;
		_serviceData.params.encrypted.state.validation = SERVICE_DATA_VALIDATION;
	}

#ifdef PRINT_DEBUG_EXTERNAL_DATA
	LOGd("servideData:");
	BLEutil::printArray(_serviceData.array, sizeof(service_data_t));
//		LOGd("serviceData: type=%u id=%u switch=%u bitmask=%u temp=%i P=%i E=%i time=%u", serviceData->params.type, serviceData->params.crownstoneId, serviceData->params.switchState, serviceData->params.flagBitmask, serviceData->params.temperature, serviceData->params.powerUsageReal, serviceData->params.accumulatedEnergy, serviceData->params.partialTimestamp);
#endif
	UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_SERVICE_DATA, _serviceData.array, sizeof(service_data_t));

//		Mesh::getInstance().printRssiList();

	// encrypt the array using the guest key ECB if encryption is enabled.
	if (State::getInstance().isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED) && _operationMode != OperationMode::OPERATION_MODE_SETUP) {
		EncryptionHandler::getInstance().encrypt(
				_serviceData.params.encryptedArray, sizeof(_serviceData.params.encryptedArray),
				_serviceData.params.encryptedArray, sizeof(_serviceData.params.encryptedArray),
				SERVICE_DATA, ECB_GUEST);
//			EncryptionHandler::getInstance().encrypt((_serviceData.array) + 1, sizeof(service_data_t) - 1, _encryptedParams.payload,
//			                                         sizeof(_encryptedParams.payload), GUEST, ECB_GUEST);
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
			LOGd("Event: %s", TypeName(event.type));
			_connected = true;
			break;
		}
		case CS_TYPE::EVT_BLE_DISCONNECT: {
			LOGd("Event: %s", TypeName(event.type));
			_connected = false;
			updateAdvertisement(false);
			break;
		}
//		// TODO: do we need to keep up the error? Or we can simply retrieve it every service data update?
//		case CS_TYPE::EVT_DIMMER_FORCED_OFF:
//		case CS_TYPE::EVT_SWITCH_FORCED_OFF:
//		case CS_TYPE::EVT_RELAY_FORCED_ON:
//			LOGd("Event: %s", TypeName(event.type));
//			updateFlagsBitmask(SERVICE_DATA_FLAGS_ERROR, true);
//			break;
		case CS_TYPE::STATE_ERRORS: {
			LOGd("Event: %s", TypeName(event.type));
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
		case CS_TYPE::EVT_TICK: {
			TYPIFY(EVT_TICK) tickCount = *(TYPIFY(EVT_TICK)*)event.data;
			_externalStates.tick(tickCount);
			if (_sendStateCountdown-- == 0) {
				uint8_t rand8;
				RNG::fillBuffer(&rand8, 1);
				uint32_t randMs = MESH_SEND_STATE_INTERVAL_MS + rand8 * MESH_SEND_STATE_INTERVAL_MS_VARIATION / 255;
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

void ServiceData::sendMeshState(bool event) {
//#ifdef BUILD_MESHING
	LOGi("sendMeshState");
	TYPIFY(STATE_TIME) timestamp;
	State::getInstance().get(CS_TYPE::STATE_TIME, &timestamp, sizeof(timestamp));

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
