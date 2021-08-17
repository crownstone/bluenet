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

#define LOGPresenceHandlerDebug LOGnone

//#define PRESENCE_HANDLER_TESTING_CODE

std::list<PresenceHandler::PresenceRecord> PresenceHandler::WhenWhoWhere;

void PresenceHandler::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_ownId, sizeof(_ownId));

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

			LOGPresenceHandlerDebug("Received: profile=%u location=%u mesh=%u", profile, location, forwardToMesh);
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

void PresenceHandler::handlePresenceEvent(uint8_t profile, uint8_t location, bool fromMesh) {
	// Validate data.
	if (profile > max_profile_id || location > max_location_id) {
		LOGw("Invalid profile(%u) or location(%u)", profile, location);
		return;
	}

	MutationType mutation = handleProfileLocationAdministration(profile, location, fromMesh);

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
	auto prevdescription = getCurrentPresenceDescription();

#ifdef PRESENCE_HANDLER_TESTING_CODE
	if (profile == 0xFF && location == 0xFF) {
		LOGw("DEBUG: clear presence record data");

		if (prevdescription.value_or(0) != 0) {
			// sphere exit
			WhenWhoWhere.clear();
			return MutationType::LastUserExitSphere;
		}

		return MutationType::NothingChanged;
	}
#endif

	// purge whowhenwhere of old entries
	if (WhenWhoWhere.size() >= max_records) {
		LOGw("Reached max number of records");
		PresenceRecord record = WhenWhoWhere.back();
		sendPresenceChange(PresenceChange::PROFILE_LOCATION_EXIT, record.who, record.where);
		WhenWhoWhere.pop_back();
	}

	uint8_t meshCountdown = 0;
	bool newLocation = true;

	for (auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); ++iter) {
		if (iter->timeoutCountdownSeconds == 0) {
			// Should not happen, record should've been removed.
			LOGw("timed out record");
		}
		if (iter->who == profile && iter->where == location) {
			LOGPresenceHandlerDebug("erasing old record profile(%u) location(%u)", profile, location);
			meshCountdown = iter->meshSendCountdownSeconds;
			WhenWhoWhere.erase(iter);
			newLocation = false;
			break;
		}
	}
	// When record is new, or the old record mesh send countdown was 0: send profile location over the mesh.
	if (meshCountdown == 0) {
		if (forwardToMesh) {
			propagateMeshMessage(profile, location);
		}
		meshCountdown = presence_mesh_send_throttle_seconds + (RNG::getInstance().getRandom8() % presence_mesh_send_throttle_seconds_variation);
	}

	// Add the new entry
	LOGPresenceHandlerDebug("add record profile(%u) location(%u)", profile, location);
	WhenWhoWhere.push_front(PresenceRecord(profile, location, presence_time_out_s, meshCountdown));

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

void PresenceHandler::removeOldRecords() {
	WhenWhoWhere.remove_if(
			[&] (PresenceRecord www) {
		if (www.timeoutCountdownSeconds == 0) {
			LOGPresenceHandlerDebug("erasing timed out record profile=%u location=%u", www.who, www.where);
			return true;
		}
		return false;
	}
	);
}

void PresenceHandler::triggerPresenceMutation(MutationType mutationtype) {
	event_t presence_event(CS_TYPE::EVT_PRESENCE_MUTATION, &mutationtype, sizeof(mutationtype));
	presence_event.dispatch();
}

void PresenceHandler::sendPresenceChange(PresenceChange type, uint8_t profileId, uint8_t locationId) {
	TYPIFY(EVT_PRESENCE_CHANGE) eventData;
	eventData.type = static_cast<uint8_t>(type);
	eventData.profileId = profileId;
	eventData.locationId = locationId;
	event_t event(CS_TYPE::EVT_PRESENCE_CHANGE, &eventData, sizeof(eventData));
	event.dispatch();
}

void PresenceHandler::propagateMeshMessage(uint8_t profile, uint8_t location) {
	TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION) eventData;
	eventData.profile = profile;
	eventData.location = location;
	event_t event(CS_TYPE::CMD_SEND_MESH_MSG_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
}


std::optional<PresenceStateDescription> PresenceHandler::getCurrentPresenceDescription() {
	if (SystemTime::up() < presence_uncertain_due_reboot_time_out_s) {
		LOGPresenceHandlerDebug("presence_uncertain_due_reboot_time_out_s hasn't expired");
		return {};
	}
	PresenceStateDescription presence;
	for (auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); ++iter) {
		if (iter->timeoutCountdownSeconds == 0) {
			// Should not happen, record should've been removed.
			LOGw("timed out record");
		}
		else {
			// appearently iter is valid, so the .where field describes an occupied room.
			presence.setRoom(iter->where);
			// LOGPresenceHandlerDebug("adding room %d to currentPresenceDescription", iter->where);
		}
	}
	return presence;
}

void PresenceHandler::tickSecond() {
	auto prevdescription = getCurrentPresenceDescription();
	for (auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end();) {
		if (iter->timeoutCountdownSeconds) {
			iter->timeoutCountdownSeconds--;
		}
		if (iter->timeoutCountdownSeconds == 0) {
			sendPresenceChange(PresenceChange::PROFILE_LOCATION_EXIT, iter->who, iter->where);
			iter = WhenWhoWhere.erase(iter);
		}
		else {
			if (iter->meshSendCountdownSeconds) {
				iter->meshSendCountdownSeconds--;
			}
			++iter;
		}
	}
	auto nextdescription = getCurrentPresenceDescription();
	auto mutation = getMutationType(prevdescription, nextdescription);

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

void PresenceHandler::print() {
	// for (auto iter = WhenWhoWhere.begin(); iter != WhenWhoWhere.end(); iter++) {
	//     LOGPresenceHandlerDebug("at %d seconds after startup user #%d was found in room %d", iter->when, iter->who, iter->where);
	// }

	std::optional<PresenceStateDescription> desc = getCurrentPresenceDescription();
	if (desc) {
		// desc->print();
	}
	else {
		LOGPresenceHandlerDebug("presenchandler status: unavailable");
	}
}
