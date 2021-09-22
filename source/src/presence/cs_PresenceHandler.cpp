/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <common/cs_Types.h>
#include <drivers/cs_RNG.h>
#include <logging/cs_Logger.h>
#include <presence/cs_PresenceHandler.h>
#include <storage/cs_State.h>
#include <time/cs_TimeOfDay.h>

#define LOGPresenceHandlerDebug LOGvv

//#define PRESENCE_HANDLER_TESTING_CODE

PresenceHandler::PresenceRecord PresenceHandler::_presenceRecords[PresenceHandler::MAX_RECORDS];

PresenceHandler::PresenceHandler() {
	resetRecords();
}

void PresenceHandler::init() {
	LOGi("init");

	listen();
}

void PresenceHandler::handleEvent(event_t& event) {

	switch (event.type) {
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
			auto parsedAdvEventData = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(event.data);

			if (BLEutil::isBitSet(parsedAdvEventData->flags, BG_ADV_FLAG_IGNORE_FOR_PRESENCE)) {
				return;
			}

			uint8_t profile  = parsedAdvEventData->profileId;
			uint8_t location = parsedAdvEventData->locationId;
			bool forwardToMesh = true;

			handlePresenceEvent(profile, location, forwardToMesh);
			return;
		}
		case CS_TYPE::EVT_RECEIVED_PROFILE_LOCATION: {
			auto profileLocationEventData = reinterpret_cast<TYPIFY(EVT_RECEIVED_PROFILE_LOCATION)*>(event.data);

			uint8_t profile  = profileLocationEventData->profileId;
			uint8_t location = profileLocationEventData->locationId;
			bool forwardToMesh = !profileLocationEventData->fromMesh;

			LOGPresenceHandlerDebug("Received: profile=%u location=%u forwardToMesh=%u", profile, location, forwardToMesh);
			handlePresenceEvent(profile, location, forwardToMesh);
			return;
		}
		case CS_TYPE::EVT_ASSET_ACCEPTED: {
			auto acceptedAssetEventData = reinterpret_cast<TYPIFY(EVT_ASSET_ACCEPTED)*>(event.data);

			uint8_t profileId = *acceptedAssetEventData->_primaryFilter.filterdata().metadata().profileId();
			uint8_t location  = 0; // Location 0 signifies 'in sphere, no specific room'
			bool forwardToMesh = true;

			LOGPresenceHandlerDebug("PresenceHandler received EVT_ASSET_ACCEPTED (profileId %u, location 0)", profileId);
			handlePresenceEvent(profileId, location, forwardToMesh);
			break;
		}
		case CS_TYPE::CMD_GET_PRESENCE: {
			LOGPresenceHandlerDebug("Get presence");
			if (event.result.buf.data == nullptr) {
				LOGPresenceHandlerDebug("ERR_BUFFER_UNASSIGNED");
				event.result.returnCode = ERR_BUFFER_UNASSIGNED;
				return;
			}
			presence_t* resultData = reinterpret_cast<presence_t*>(event.result.buf.data);
			if (event.result.buf.len < sizeof(*resultData)) {
				LOGPresenceHandlerDebug("ERR_BUFFER_TOO_SMALL");
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				return;
			}

			std::optional<PresenceStateDescription> presence = getCurrentPresenceDescription();
			uint8_t numProfiles = sizeof(resultData->presence) / sizeof(resultData->presence[0]);
			for (uint8_t i = 0; i < numProfiles; ++i) {
				resultData->presence[i] = 0;
			}
			resultData->presence[0] = presence ? presence.value().getBitmask() : 0;
			event.result.dataSize     = sizeof(*resultData);
			event.result.returnCode   = ERR_SUCCESS;
			return;
		}
		case CS_TYPE::EVT_TICK: {
			TYPIFY(EVT_TICK)* tickCount = reinterpret_cast<TYPIFY(EVT_TICK)*>(event.data);
			if (*tickCount % (1000 / TICK_INTERVAL_MS) == 0) {
				tickSecond();
			}
			return;
		}
		default: return;
	}
}

void PresenceHandler::handlePresenceEvent(uint8_t profile, uint8_t location, bool forwardToMesh) {
	// Validate data.
	if (profile > MAX_PROFILE_ID || location > MAX_LOCATION_ID) {
		if (profile != 0xFF) {
			LOGw("Invalid profile(%u) or location(%u)", profile, location);
		}
		return;
	}

	PresenceMutation mutation = handleProfileLocation(profile, location, forwardToMesh);

	if (mutation != PresenceMutation::NothingChanged) {
		dispatchPresenceMutationEvent(mutation);
	}

	switch (mutation) {
		case PresenceMutation::FirstUserEnterSphere: {
			dispatchPresenceChangeEvent(PresenceChange::FIRST_SPHERE_ENTER);
			dispatchPresenceChangeEvent(PresenceChange::PROFILE_SPHERE_ENTER, profile);
			break;
		}
		case PresenceMutation::LastUserExitSphere: {
			dispatchPresenceChangeEvent(PresenceChange::LAST_SPHERE_EXIT);
			dispatchPresenceChangeEvent(PresenceChange::PROFILE_SPHERE_EXIT, profile);
			break;
		}
		default: break;
	}
}

PresenceMutation PresenceHandler::handleProfileLocation(uint8_t profile, uint8_t location, bool forwardToMesh) {
	LOGPresenceHandlerDebug("handleProfileLocationAdministration profile=%u location=%u forwardToMesh=%u", profile, location, forwardToMesh);
	auto prevdescription = getCurrentPresenceDescription();

#ifdef PRESENCE_HANDLER_TESTING_CODE
	if (profile == 0xFF && location == 0xFF) {
		LOGw("DEBUG: clear presence record data");

		if (prevdescription.value_or(0) != 0) {
			// sphere exit
			resetRecords();
			return PresenceMutation::LastUserExitSphere;
		}

		return PresenceMutation::NothingChanged;
	}
#endif

	uint8_t meshCountdown = 0;
	bool newLocation = true;

	PresenceHandler::PresenceRecord* record = findRecord(profile, location);
	if (record == nullptr) {
		// Create a new record
		record = addRecord(profile, location);

		if (record == nullptr) {
			// We cannot handle this presence event.
			return PresenceMutation::NothingChanged;
		}
	}
	else {
		// This record already exists.
		newLocation = false;

		// Reset the timeout countdown.
		record->timeoutCountdownSeconds = PRESENCE_TIMEOUT_SECONDS;
		meshCountdown = record->meshSendCountdownSeconds;
	}

	// When record is new, or the old record mesh send countdown was 0: send profile location over the mesh.
	if (meshCountdown == 0) {
		if (forwardToMesh) {
			sendMeshMessage(profile, location);
		}
		meshCountdown = PRESENCE_MESH_SEND_THROTTLE_SECONDS + (RNG::getInstance().getRandom8() % PRESENCE_MESH_SEND_THROTTLE_SECONDS_VARIATION);
	}
	record->meshSendCountdownSeconds = meshCountdown;

	if (newLocation) {
		dispatchPresenceChangeEvent(PresenceChange::PROFILE_LOCATION_ENTER, profile, location);
	}

	auto nextdescription = getCurrentPresenceDescription();
	return getMutationType(prevdescription, nextdescription);
}

PresenceMutation PresenceHandler::getMutationType(
		std::optional<PresenceStateDescription> prevDescription,
		std::optional<PresenceStateDescription> nextDescription) {

	if (prevDescription == nextDescription) {
		return PresenceMutation::NothingChanged; // neither has value or values are equal.
	}

	if (prevDescription.has_value() && nextDescription.has_value()) {
		// values unequal so can distinguish on == 0 of a single value here
		if (prevDescription.value().getBitmask() == 0) {
			return PresenceMutation::FirstUserEnterSphere;
		}
		if (nextDescription.value().getBitmask() == 0) {
			return PresenceMutation::LastUserExitSphere;
		}
		// both non-zero, non-trivial change happened.
		return PresenceMutation::OccupiedRoomsMaskChanged;
	}

	if (nextDescription.has_value()) {
		// Previous has no value.
		return PresenceMutation::Online;
	}

	if (prevDescription.has_value()) {
		// New has no value.
		return PresenceMutation::Offline;
	}

	// can't reach here but that's too difficult for the compiler to conclude.
	LOGw("unhandled presence handler description");
	return PresenceMutation::NothingChanged;
}

void PresenceHandler::dispatchPresenceMutationEvent(PresenceMutation mutation) {
	LOGi("dispatchPresenceMutationEvent mutation=%u", mutation);
	event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION, &mutation, sizeof(mutation));
	presence_event.dispatch();
}

void PresenceHandler::dispatchPresenceChangeEvent(PresenceChange type, uint8_t profileId, uint8_t locationId) {
	LOGi("dispatchPresenceChangeEvent type=%u profile=%u location=%u", type, profileId, locationId);
	TYPIFY(EVT_PRESENCE_CHANGE) eventData;
	eventData.type = static_cast<uint8_t>(type);
	eventData.profileId = profileId;
	eventData.locationId = locationId;
	event_t event(CS_TYPE::EVT_PRESENCE_CHANGE, &eventData, sizeof(eventData));
	event.dispatch();
}

void PresenceHandler::sendMeshMessage(uint8_t profile, uint8_t location) {
	LOGPresenceHandlerDebug("sendMeshMessage profile=%u location=%u", profile, location);
	TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION) eventData;
	eventData.profile = profile;
	eventData.location = location;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
}


std::optional<PresenceStateDescription> PresenceHandler::getCurrentPresenceDescription() {
	if (SystemTime::up() < PRESENCE_UNCERTAIN_SECONDS_AFTER_BOOT) {
		LOGPresenceHandlerDebug("Presence is uncertain after boot");
		return {};
	}
	PresenceStateDescription presence;
	for (uint8_t i = 0; i < MAX_RECORDS; ++i) {
		if (!_presenceRecords[i].isValid()) {
			continue;
		}
		presence.setLocation(_presenceRecords[i].location);
	}
	return presence;
}

void PresenceHandler::tickSecond() {
	auto prevDescription = getCurrentPresenceDescription();
	for (uint8_t i = 0; i < MAX_RECORDS; ++i) {
		if (_presenceRecords[i].isValid() && _presenceRecords[i].timeoutCountdownSeconds != 0) {
			_presenceRecords[i].timeoutCountdownSeconds--;
			if (_presenceRecords[i].timeoutCountdownSeconds == 0) {
				LOGi("Timeout: profile=%u location=%u", _presenceRecords[i].profile, _presenceRecords[i].location);
				dispatchPresenceChangeEvent(PresenceChange::PROFILE_LOCATION_EXIT, _presenceRecords[i].profile, _presenceRecords[i].location);
			}
			else {
				if (_presenceRecords[i].meshSendCountdownSeconds != 0) {
					_presenceRecords[i].meshSendCountdownSeconds--;
				}
			}
		}
	}

	auto nextDescription = getCurrentPresenceDescription();
	auto mutation = getMutationType(prevDescription, nextDescription);
	LOGPresenceHandlerDebug("mutation=%u", mutation);

	switch (mutation) {
		case PresenceMutation::LastUserExitSphere: {
			dispatchPresenceChangeEvent(PresenceChange::LAST_SPHERE_EXIT);
			dispatchPresenceChangeEvent(PresenceChange::PROFILE_SPHERE_EXIT, 0);
			break;
		}
		default:
			break;
	}
}

void PresenceHandler::resetRecords() {
	LOGi("resetRecords");
	for (uint8_t i = 0; i < MAX_RECORDS; ++i) {
		_presenceRecords[i].invalidate();
	}
}

PresenceHandler::PresenceRecord* PresenceHandler::findRecord(uint8_t profile, uint8_t location) {
	LOGPresenceHandlerDebug("findRecord profile=%u location=%u", profile, location);
	for (uint8_t i = 0; i < MAX_RECORDS; ++i) {
		if (_presenceRecords[i].isValid()) {
			if (_presenceRecords[i].profile == profile && _presenceRecords[i].location == location) {
				LOGPresenceHandlerDebug("Found at index=%u", i);
				return &(_presenceRecords[i]);
			}
		}
	}
	return nullptr;
}

PresenceHandler::PresenceRecord* PresenceHandler::addRecord(uint8_t profile, uint8_t location) {
	uint8_t emptySpotIndex = 0xFF;
	uint8_t oldestEntryCounter = 0xFF;
	uint8_t oldestEntryIndex = 0xFF;
	for (uint8_t i = 0; i < MAX_RECORDS; ++i) {
		if (!_presenceRecords[i].isValid()) {
			emptySpotIndex = i;
			break;
		}
		if (_presenceRecords[i].timeoutCountdownSeconds < oldestEntryCounter) {
			oldestEntryCounter = _presenceRecords[i].timeoutCountdownSeconds;
			oldestEntryIndex = i;
		}
	}

	if (emptySpotIndex != 0xFF) {
		LOGPresenceHandlerDebug("Add at empty index=%u", emptySpotIndex);
		_presenceRecords[emptySpotIndex] = PresenceRecord(profile, location);
		return &(_presenceRecords[emptySpotIndex]);
	}

	if (oldestEntryIndex != 0xFF) {
		LOGPresenceHandlerDebug("Add at oldest index=%u", oldestEntryIndex);
		_presenceRecords[oldestEntryIndex] = PresenceRecord(profile, location);
		return &(_presenceRecords[oldestEntryIndex]);
	}

	LOGw("No space to add");
	return nullptr;
}
