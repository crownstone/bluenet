/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 13, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <presence/cs_PresenceHandler.h>

#include <time/cs_TimeOfDay.h>
#include <common/cs_Types.h>

#include <storage/cs_State.h>

#include <drivers/cs_RNG.h>
#include <logging/cs_Logger.h>

#define LOGPresenceHandlerDebug LOGvv

//#define PRESENCE_HANDLER_TESTING_CODE

PresenceHandler::PresenceRecord PresenceHandler::_presenceRecords[PresenceHandler::MAX_RECORDS];

void PresenceHandler::init() {
	LOGi("init");

	resetRecords();

	// TODO Anne @Arend. The listener is now set in cs_Crownstone.cpp, outside of the class. This seems to be an
	// implementation detail however that should be part of the class. If you want the user to start and stop
	// listening to events, I'd add member functions for those.

	// EventDispatcher::getInstance().addListener(this);
}

void PresenceHandler::handleEvent(event_t& evt) {

	switch (evt.type) {
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
			auto parsedAdvEventData = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);

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
			auto profileLocationEventData = reinterpret_cast<TYPIFY(EVT_RECEIVED_PROFILE_LOCATION)*>(evt.data);

			uint8_t profile  = profileLocationEventData->profileId;
			uint8_t location = profileLocationEventData->locationId;
			bool forwardToMesh = !profileLocationEventData->fromMesh;

			LOGPresenceHandlerDebug("Received: profile=%u location=%u forwardToMesh=%u", profile, location, forwardToMesh);
			handlePresenceEvent(profile, location, forwardToMesh);
			return;
		}
		case CS_TYPE::EVT_ASSET_ACCEPTED: {
			auto acceptedAssetEventData = reinterpret_cast<TYPIFY(EVT_ASSET_ACCEPTED)*>(evt.data);

			uint8_t profileId = *acceptedAssetEventData->_primaryFilter.filterdata().metadata().profileId();
			uint8_t location  = 0; // Location 0 signifies 'in sphere, no specific room'
			bool forwardToMesh = true;

			LOGPresenceHandlerDebug("PresenceHandler received EVT_ASSET_ACCEPTED (profileId %u, location 0)", profileId);
			handlePresenceEvent(profileId, location, forwardToMesh);
			break;
		}
		case CS_TYPE::CMD_GET_PRESENCE: {
			LOGPresenceHandlerDebug("Get presence");
			if (evt.result.buf.data == nullptr) {
				LOGPresenceHandlerDebug("ERR_BUFFER_UNASSIGNED");
				evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
				return;
			}
			presence_t* resultData = reinterpret_cast<presence_t*>(evt.result.buf.data);
			if (evt.result.buf.len < sizeof(*resultData)) {
				LOGPresenceHandlerDebug("ERR_BUFFER_TOO_SMALL");
				evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
				return;
			}

			std::optional<PresenceStateDescription> presence = getCurrentPresenceDescription();
			uint8_t numProfiles = sizeof(resultData->presence) / sizeof(resultData->presence[0]);
			for (uint8_t i = 0; i < numProfiles; ++i) {
				resultData->presence[i] = 0;
			}
			resultData->presence[0] = presence ? presence.value().getBitmask() : 0;
			evt.result.dataSize     = sizeof(*resultData);
			evt.result.returnCode   = ERR_SUCCESS;
			return;
		}
		case CS_TYPE::EVT_TICK: {
			TYPIFY(EVT_TICK)* tickCount = reinterpret_cast<TYPIFY(EVT_TICK)*>(evt.data);
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

	MutationType mutation = handleProfileLocationAdministration(profile, location, forwardToMesh);

	if (mutation != MutationType::NothingChanged) {
		triggerPresenceMutation(mutation);
	}

	switch (mutation) {
		case MutationType::FirstUserEnterSphere: {
			sendPresenceChange(PresenceChange::FIRST_SPHERE_ENTER);
			sendPresenceChange(PresenceChange::PROFILE_SPHERE_ENTER, profile);
			break;
		}
		case MutationType::LastUserExitSphere: {
			sendPresenceChange(PresenceChange::LAST_SPHERE_EXIT);
			sendPresenceChange(PresenceChange::PROFILE_SPHERE_EXIT, profile);
			break;
		}
		default: break;
	}
}

PresenceHandler::MutationType PresenceHandler::handleProfileLocationAdministration(uint8_t profile, uint8_t location, bool forwardToMesh) {
	LOGPresenceHandlerDebug("handleProfileLocationAdministration profile=%u location=%u forwardToMesh=%u", profile, location, forwardToMesh);
	auto prevdescription = getCurrentPresenceDescription();

#ifdef PRESENCE_HANDLER_TESTING_CODE
	if (profile == 0xFF && location == 0xFF) {
		LOGw("DEBUG: clear presence record data");

		if (prevdescription.value_or(0) != 0) {
			// sphere exit
			resetRecords();
			return MutationType::LastUserExitSphere;
		}

		return MutationType::NothingChanged;
	}
#endif

	uint8_t meshCountdown = 0;
	bool newLocation = true;

	PresenceHandler::PresenceRecord* record = findOrAddRecord(profile, location);
	if (record != nullptr) {
		record->timeoutCountdownSeconds = PRESENCE_TIMEOUT_SECONDS;
		meshCountdown = record->meshSendCountdownSeconds;
		newLocation = false;
	}

	// When record is new, or the old record mesh send countdown was 0: send profile location over the mesh.
	if (meshCountdown == 0) {
		if (forwardToMesh) {
			propagateMeshMessage(profile, location);
		}
		meshCountdown = PRESENCE_MESH_SEND_THROTTLE_SECONDS + (RNG::getInstance().getRandom8() % PRESENCE_MESH_SEND_THROTTLE_SECONDS_VARIATION);
	}

	if (record != nullptr) {
		record->meshSendCountdownSeconds = meshCountdown;
	}

	if (newLocation) {
		sendPresenceChange(PresenceChange::PROFILE_LOCATION_ENTER, profile, location);
	}

	auto nextdescription = getCurrentPresenceDescription();
	return getMutationType(prevdescription, nextdescription);
}

PresenceHandler::MutationType PresenceHandler::getMutationType(
		std::optional<PresenceStateDescription> prevdescription,
		std::optional<PresenceStateDescription> nextdescription) {

	if (prevdescription == nextdescription) {
		return MutationType::NothingChanged; // neither has_value or value()'s eq.
	}

	if (prevdescription.has_value() && nextdescription.has_value()) {
		// values unequal so can distinguish on == 0 of a single value here
		if (*prevdescription == 0) {
			return MutationType::FirstUserEnterSphere;
		}
		if (*nextdescription == 0) {
			return MutationType::LastUserExitSphere;
		}
		// both non-zero, non-trivial change happened.
		return MutationType::OccupiedRoomsMaskChanged;
	}

	if (nextdescription.has_value()) {
		return MutationType::Online;
	}

	if (prevdescription.has_value()) {
		return MutationType::Offline;
	}

	// can't reach here but that's too difficult for the compiler to conclude.
	LOGw("unhandled presence handler description");
	return MutationType::NothingChanged;
}

void PresenceHandler::triggerPresenceMutation(MutationType mutationtype) {
	event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION, &mutationtype, sizeof(mutationtype));
	presence_event.dispatch();
}

void PresenceHandler::sendPresenceChange(PresenceChange type, uint8_t profileId, uint8_t locationId) {
	LOGi("sendPresenceChange type=%u profile=%u location=%u", type, profileId, locationId);
	TYPIFY(EVT_PRESENCE_CHANGE) eventData;
	eventData.type = static_cast<uint8_t>(type);
	eventData.profileId = profileId;
	eventData.locationId = locationId;
	event_t event(CS_TYPE::EVT_PRESENCE_CHANGE, &eventData, sizeof(eventData));
	event.dispatch();
}

void PresenceHandler::propagateMeshMessage(uint8_t profile, uint8_t location) {
	LOGPresenceHandlerDebug("propagateMeshMessage profile=%u location=%u", profile, location);
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
		presence.setRoom(_presenceRecords[i].location);
	}
	return presence;
}

void PresenceHandler::tickSecond() {
	auto prevdescription = getCurrentPresenceDescription();
	for (uint8_t i = 0; i < MAX_RECORDS; ++i) {
		if (_presenceRecords[i].isValid() && _presenceRecords[i].timeoutCountdownSeconds != 0) {
			_presenceRecords[i].timeoutCountdownSeconds--;
			if (_presenceRecords[i].timeoutCountdownSeconds == 0) {
				LOGi("Timeout: profile=%u location=%u", _presenceRecords[i].profile, _presenceRecords[i].location);
				sendPresenceChange(PresenceChange::PROFILE_LOCATION_EXIT, _presenceRecords[i].profile, _presenceRecords[i].location);
			}
			else {
				if (_presenceRecords[i].meshSendCountdownSeconds != 0) {
					_presenceRecords[i].meshSendCountdownSeconds--;
				}
			}
		}
	}

	auto nextdescription = getCurrentPresenceDescription();
	auto mutation = getMutationType(prevdescription, nextdescription);
	LOGPresenceHandlerDebug("mutation=%u", mutation);

	switch (mutation) {
		case MutationType::LastUserExitSphere: {
			sendPresenceChange(PresenceChange::LAST_SPHERE_EXIT);
			sendPresenceChange(PresenceChange::PROFILE_SPHERE_EXIT, 0);
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

PresenceHandler::PresenceRecord* PresenceHandler::findOrAddRecord(uint8_t profile, uint8_t location) {
	LOGPresenceHandlerDebug("findOrAddRecord profile=%u location=%u", profile, location);
	for (uint8_t i = 0; i < MAX_RECORDS; ++i) {
		if (_presenceRecords[i].isValid()) {
			if (_presenceRecords[i].profile == profile && _presenceRecords[i].location == location) {
				LOGPresenceHandlerDebug("Found at index=%u", i);
				return &(_presenceRecords[i]);
			}
		}
	}

	// Find a new spot.
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
		LOGPresenceHandlerDebug("Using empty index=%u", emptySpotIndex);
		_presenceRecords[emptySpotIndex] = PresenceRecord(profile, location);
		return &(_presenceRecords[emptySpotIndex]);
	}

	if (oldestEntryIndex != 0xFF) {
		LOGPresenceHandlerDebug("Using oldest index=%u", oldestEntryIndex);
		_presenceRecords[oldestEntryIndex] = PresenceRecord(profile, location);
		return &(_presenceRecords[oldestEntryIndex]);
	}

	LOGw("No space");
	return nullptr;
}
