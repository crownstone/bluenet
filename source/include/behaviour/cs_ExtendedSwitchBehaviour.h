/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <behaviour/cs_SwitchBehaviour.h>

/**
 * A smart timer behaviour is a switch behaviour that is allowed to extend itself
 * passed the until time defined. The extension is based on a PresenceCondition.
 *
 * Envisioned use case:
 * - A dumb timer is annoying because it will turn off a light when a user is in the
 *   room. It would be much nicer if it delayed this until the user left.
 */
class ExtendedSwitchBehaviour : public SwitchBehaviour {
public:
	typedef std::array<uint8_t, WireFormat::size<SwitchBehaviour>() + WireFormat::size<PresenceCondition>()>
			SerializedDataType;

	ExtendedSwitchBehaviour(SwitchBehaviour corebehaviour, PresenceCondition extensioncondition);
	virtual ~ExtendedSwitchBehaviour() = default;

	ExtendedSwitchBehaviour(SerializedDataType arr);

	SerializedDataType serialize();

	virtual uint8_t* serialize(uint8_t* outbuff, size_t max_size) override;
	virtual size_t serializedSize() const override;

	virtual Type getType() const override { return Type::Extended; }

	// requiresPresence depends on corebehaviour, extensionIsActive and extensionCondition.
	// it assumes that extensionIsActive is up to date.
	virtual bool requiresPresence() override;

	// requiresAbsence depends on corebehaviour, extensionIsActive and extensionCondition.
	// it assumes that extensionIsActive is up to date.
	virtual bool requiresAbsence() override;

	// currentPresenceCondition depends on corebehaviour, extensionIsActive and extensionCondition.
	// it assumes that extensionIsActive is up to date.
	virtual PresencePredicate currentPresencePredicate() override;

	// See SwitchBehaviour for more elaborate explanation why this is necessary.
	using SwitchBehaviour::isValid;

	/**
	 * Does the behaviour apply to the current situation?
	 * Depends on corebehaviour, extensionIsActive and extensionCondition.
	 **/
	virtual bool isValid(Time currenttime, PresenceStateDescription currentpresence);

	virtual void print();

	bool extensionPeriodIsActive() { return extensionIsActive; }

private:
	PresenceCondition extensionCondition;

	/**
	 * extensionIsActive will be set to true when at the end of the core behaviour
	 * valid period a call to isValid(Time,PresenceCondition) was made that returned true.
	 *
	 * When extensionIsActive is true, the isValid(Time,PresenceCondition) function will
	 * return true when either the core behaviour isValid is satisfied, or the extensionCondition
	 * isValid is satisfied.
	 *
	 * extensionIsActive will be reset to false [extensionCondition.timeOut] seconds after the
	 * until() value of the core behaviour, unless this timeOut is zero: in that case it will
	 * be reset to false as soon as the PresenceCondition evaluates to false.
	 */
	bool extensionIsActive                            = false;
	std::optional<Time> prevExtensionIsValidTimeStamp = {};
};
