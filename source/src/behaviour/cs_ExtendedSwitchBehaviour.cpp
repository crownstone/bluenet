/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_ExtendedSwitchBehaviour.h>
#include <logging/cs_Logger.h>
#include <time/cs_SystemTime.h>
#include <util/cs_WireFormat.h>

ExtendedSwitchBehaviour::ExtendedSwitchBehaviour(SwitchBehaviour coreBehaviour, PresenceCondition extCondition) :
	SwitchBehaviour(coreBehaviour), extensionCondition(extCondition) {
	typ = Behaviour::Type::Extended;
}

ExtendedSwitchBehaviour::ExtendedSwitchBehaviour(SerializedDataType arr) : 
	ExtendedSwitchBehaviour(
			WireFormat::deserialize<SwitchBehaviour>(
					arr.data() +  0,
					WireFormat::size<SwitchBehaviour>()
			),
			WireFormat::deserialize<PresenceCondition>(
					arr.data() + WireFormat::size<SwitchBehaviour>(),
					WireFormat::size<PresenceCondition>()
			)
	) {
}

ExtendedSwitchBehaviour::SerializedDataType ExtendedSwitchBehaviour::serialize() {
	SerializedDataType result;

	serialize(result.data(), WireFormat::size<ExtendedSwitchBehaviour>());

	return result;
}

uint8_t* ExtendedSwitchBehaviour::serialize(uint8_t* outBuff, size_t maxSize) {
	if (maxSize != 0) {
		if (outBuff == nullptr || maxSize < serializedSize()) {
			// all or nothing..
			return outBuff;
		}
	}

	// outbuff is big enough :)

	outBuff = SwitchBehaviour::serialize(outBuff, maxSize);
	outBuff = extensionCondition.serialize(outBuff);

	return outBuff;
}

size_t ExtendedSwitchBehaviour::serializedSize() const {
	return WireFormat::size<ExtendedSwitchBehaviour>();
}

bool ExtendedSwitchBehaviour::requiresPresence() {
	return extensionIsActive
			? presenceCondition.predicate.requiresPresence()
			: extensionCondition.predicate.requiresPresence();
}

bool ExtendedSwitchBehaviour::requiresAbsence() {
	return extensionIsActive
			? presenceCondition.predicate.requiresAbsence()
			: extensionCondition.predicate.requiresAbsence();
}

bool ExtendedSwitchBehaviour::isValid(Time currentTime, PresenceStateDescription currentPresence) {
	// implementation detail:
	// SwitchBehaviour::isValid(PresenceStateDescription) caches the last valid presence timestamp.
	// However, this must be recomputed in the extension anyway because the conditions may differ.

	if (SwitchBehaviour::isValid(currentTime)) {
		// currenttime between from() and until()
		extensionIsActive = SwitchBehaviour::isValid(currentPresence);
		return extensionIsActive;
	}

	if (!extensionIsActive) {
		// not extended, nor in active time slot
		return false;
	}

	if (extensionCondition.isTrue(currentPresence)) {
		// in extension and presence match
		prevExtensionIsValidTimeStamp = SystemTime::now();
		return true;
	}

	if (prevExtensionIsValidTimeStamp) {
		if (CsMath::Interval<uint32_t>(
				SystemTime::posix(), extensionCondition.timeOut, true)
				.contains(prevExtensionIsValidTimeStamp->timestamp())) {
			// in extension and presence is invalid,
			// but we're in the extension's grace period.
			return true;
		}
	}

	// deactivate
	extensionIsActive = false;
	prevExtensionIsValidTimeStamp.reset();

	return false;
}

void ExtendedSwitchBehaviour::print() {
	LOGd("## ExtendedSwitchBehaviour:");
	SwitchBehaviour::print();
	extensionCondition.predicate.print();

	LOGd("extension is active: %d", extensionIsActive);

	if (prevExtensionIsValidTimeStamp) {
		[[maybe_unused]] TimeOfDay t = prevExtensionIsValidTimeStamp->timeOfDay();
		LOGd("extension timestamp : %02u:%02u:%02u", t.h(), t.m(), t.s());
	}
	LOGd("##");
}
