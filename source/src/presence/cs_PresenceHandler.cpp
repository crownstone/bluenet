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
#include <util/cs_Math.h>

#define LOGPresenceHandlerDebug LOGvv

//#define PRESENCE_HANDLER_TESTING_CODE

PresenceHandler::PresenceHandler() {
	_store.clear();
}

cs_ret_code_t PresenceHandler::init() {
	LOGi("init");

	listen();
	return ERR_SUCCESS;
}

void PresenceHandler::registerPresence(ProfileLocation profileLocation) {
	handlePresenceEvent(profileLocation, false);
}

void PresenceHandler::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
			auto parsedAdvEventData = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(event.data);

			if (CsUtils::isBitSet(parsedAdvEventData->flags, BG_ADV_FLAG_IGNORE_FOR_PRESENCE)) {
				return;
			}

			ProfileLocation profileLocation = {
					.profile = parsedAdvEventData->profileId, .location = parsedAdvEventData->locationId};
			bool forwardToMesh = true;

			LOGPresenceHandlerDebug(
					"From background adv: profile=%u location=%u forwardToMesh=%u",
					profileLocation.profile,
					profileLocation.location,
					forwardToMesh);
			handlePresenceEvent(profileLocation, forwardToMesh);
			return;
		}
		case CS_TYPE::EVT_RECEIVED_PROFILE_LOCATION: {
			auto profileLocationEventData   = reinterpret_cast<TYPIFY(EVT_RECEIVED_PROFILE_LOCATION)*>(event.data);

			ProfileLocation profileLocation = {
					.profile = profileLocationEventData->profileId, .location = profileLocationEventData->locationId};
			bool forwardToMesh = !profileLocationEventData->fromMesh;

			LOGPresenceHandlerDebug(
					"Received: profile=%u location=%u forwardToMesh=%u",
					profileLocation.profile,
					profileLocation.location,
					forwardToMesh);
			handlePresenceEvent(profileLocation, forwardToMesh);
			return;
		}
		case CS_TYPE::EVT_ASSET_ACCEPTED: {
			auto acceptedAssetEventData     = reinterpret_cast<TYPIFY(EVT_ASSET_ACCEPTED)*>(event.data);

			ProfileLocation profileLocation = {
					.profile  = *acceptedAssetEventData->_primaryFilter.filterdata().metadata().profileId(),
					.location = 0  // Location 0 signifies 'in sphere, no specific room'
			};

			bool forwardToMesh = true;

			LOGPresenceHandlerDebug(
					"From asset: profile=%u location=%u forwardToMesh=%u",
					profileLocation.profile,
					profileLocation.location,
					forwardToMesh);
			handlePresenceEvent(profileLocation, forwardToMesh);
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
			event.result.dataSize   = sizeof(*resultData);
			event.result.returnCode = ERR_SUCCESS;
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

void PresenceHandler::handlePresenceEvent(ProfileLocation profileLocation, bool forwardToMesh) {
	// Validate data.
	if (!profileLocation.isValid()) {
		if (profileLocation.profile != 0xFF) {
			LOGw("Invalid profile(%u) or location(%u)", profileLocation.profile, profileLocation.location);
		}
		return;
	}

	PresenceMutation mutation = handleProfileLocation(profileLocation, forwardToMesh);

	if (mutation != PresenceMutation::NothingChanged) {
		dispatchPresenceMutationEvent(mutation);
	}

	switch (mutation) {
		case PresenceMutation::FirstUserEnterSphere: {
			dispatchPresenceChangeEvent(PresenceChange::FIRST_SPHERE_ENTER);
			dispatchPresenceChangeEvent(PresenceChange::PROFILE_SPHERE_ENTER, profileLocation);
			break;
		}
		case PresenceMutation::LastUserExitSphere: {
			dispatchPresenceChangeEvent(PresenceChange::LAST_SPHERE_EXIT);
			dispatchPresenceChangeEvent(PresenceChange::PROFILE_SPHERE_EXIT, profileLocation);
			break;
		}
		default: break;
	}
}

PresenceMutation PresenceHandler::handleProfileLocation(ProfileLocation profileLocation, bool forwardToMesh) {
	LOGPresenceHandlerDebug(
			"handleProfileLocation profile=%u location=%u forwardToMesh=%u",
			profileLocation.profile,
			profileLocation.location,
			forwardToMesh);

	auto prevdescription = getCurrentPresenceDescription();

#ifdef PRESENCE_HANDLER_TESTING_CODE
	if (profile == 0xFF && location == 0xFF) {
		LOGw("DEBUG: clear presence record data");

		if (prevdescription.value_or(0) != 0) {
			// sphere exit
			LOGi("resetRecords");
			_store.clear();
			return PresenceMutation::LastUserExitSphere;
		}

		return PresenceMutation::NothingChanged;
	}
#endif

	uint8_t meshCountdown  = 0;
	bool newLocation       = true;

	PresenceRecord* record = _store.getOrAdd(profileLocation);
	if (record == nullptr) {
		// All spots in the store are occupied, clear the oldest.
		record = clearOldestRecord(profileLocation);
	}
	else if (!record->isValid()) {
		// Invalid record was found in the store, clean up and use that one.
		*record = PresenceRecord(profileLocation);
	}
	else {
		// Record already exists for this profile location.
		newLocation                     = false;

		// Reset the timeout countdown.
		record->timeoutCountdownSeconds = PRESENCE_TIMEOUT_SECONDS;
		meshCountdown                   = record->meshSendCountdownSeconds;
	}

	// When record is new, or the old record mesh send countdown was 0: send profile location over the mesh.
	if (meshCountdown == 0) {
		if (forwardToMesh) {
			sendMeshMessage(profileLocation);
		}
		meshCountdown = PRESENCE_MESH_SEND_THROTTLE_SECONDS
						+ (RNG::getInstance().getRandom8() % PRESENCE_MESH_SEND_THROTTLE_SECONDS_VARIATION);
	}
	record->meshSendCountdownSeconds = meshCountdown;

	if (newLocation) {
		dispatchPresenceChangeEvent(PresenceChange::PROFILE_LOCATION_ENTER, profileLocation);
	}

	auto nextdescription = getCurrentPresenceDescription();
	return getMutationType(prevdescription, nextdescription);
}

PresenceMutation PresenceHandler::getMutationType(
		std::optional<PresenceStateDescription> prevDescription,
		std::optional<PresenceStateDescription> nextDescription) {

	if (prevDescription == nextDescription) {
		return PresenceMutation::NothingChanged;  // neither has value or values are equal.
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

void PresenceHandler::dispatchPresenceChangeEvent(PresenceChange type, ProfileLocation profileLocation) {
	LOGi("dispatchPresenceChangeEvent type=%u profile=%u location=%u",
		 type,
		 profileLocation.profile,
		 profileLocation.location);
	TYPIFY(EVT_PRESENCE_CHANGE) eventData;
	eventData.type       = static_cast<uint8_t>(type);
	eventData.profileId  = profileLocation.profile;
	eventData.locationId = profileLocation.location;
	event_t event(CS_TYPE::EVT_PRESENCE_CHANGE, &eventData, sizeof(eventData));
	event.dispatch();
}

void PresenceHandler::sendMeshMessage(ProfileLocation profileLocation) {
	LOGPresenceHandlerDebug(
			"sendMeshMessage profile=%u location=%u", profileLocation.profile, profileLocation.location);
	TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION) eventData;
	eventData.profile  = profileLocation.profile;
	eventData.location = profileLocation.location;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
}

std::optional<PresenceStateDescription> PresenceHandler::getCurrentPresenceDescription() {
	if (SystemTime::up() < PRESENCE_UNCERTAIN_SECONDS_AFTER_BOOT) {
		LOGPresenceHandlerDebug("Presence is uncertain after boot");
		return {};
	}
	PresenceStateDescription presence;
	for (auto presenceRecord : _store) {
		if (!presenceRecord.isValid()) {
			continue;
		}
		presence.setLocation(presenceRecord.profileLocation.location);
	}
	return presence;
}

void PresenceHandler::tickSecond() {
	auto prevDescription = getCurrentPresenceDescription();

	for (auto& presenceRecord : _store) {
		if (presenceRecord.isValid() && presenceRecord.timeoutCountdownSeconds != 0) {
			presenceRecord.timeoutCountdownSeconds--;
			if (presenceRecord.timeoutCountdownSeconds == 0) {
				LOGi("Timeout: profile=%u location=%u",
					 presenceRecord.profileLocation.profile,
					 presenceRecord.profileLocation.location);
				dispatchPresenceChangeEvent(PresenceChange::PROFILE_LOCATION_EXIT, presenceRecord.profileLocation);
			}
			else {
				CsMath::Decrease(presenceRecord.meshSendCountdownSeconds);
			}
		}
	}

	auto nextDescription = getCurrentPresenceDescription();
	auto mutation        = getMutationType(prevDescription, nextDescription);
	LOGPresenceHandlerDebug("mutation=%u", mutation);

	switch (mutation) {
		case PresenceMutation::LastUserExitSphere: {
			dispatchPresenceChangeEvent(PresenceChange::LAST_SPHERE_EXIT);
			dispatchPresenceChangeEvent(PresenceChange::PROFILE_SPHERE_EXIT);
			break;
		}
		default: break;
	}
}

PresenceHandler::PresenceRecord* PresenceHandler::clearOldestRecord(ProfileLocation profileLocation) {
	// Last option, overwrite oldest record.
	auto oldestRecord = _store.begin();
	for (auto record = _store.begin(); record != _store.end(); record++) {
		if (record->timeoutCountdownSeconds < oldestRecord->timeoutCountdownSeconds) {
			oldestRecord = record;
		}
	}

	LOGPresenceHandlerDebug("Overwriting oldest presence record");
	*oldestRecord = PresenceRecord(profileLocation);
	return oldestRecord;
}
