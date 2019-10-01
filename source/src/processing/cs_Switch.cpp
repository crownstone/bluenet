/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/cs_Switch.h>

#include <cfg/cs_Boards.h>
#include <cfg/cs_Config.h>
#include <drivers/cs_Serial.h>

#include <storage/cs_State.h>

#include <cfg/cs_Strings.h>
#include <ble/cs_Stack.h>
#include <processing/cs_Scanner.h>
#include <events/cs_EventDispatcher.h>
#include <drivers/cs_PWM.h>

#define PRINT_SWITCH_VERBOSE

/**
 * TODO: Remove function. This should be self-contained... If it is required to wait a bit after init, this type of
 * decision should be part of this class. If there is a message exchange required with the current sensing module,
 * then this should be done over the event bus.
 *
 * The PWM is required for syncing with zero-crossings.
 */
void Switch::start() {
	LOGd("Start switch");

	_relayPowered = true;


	// If switchcraft is enabled, assume a boot is due to a brownout caused by a too slow wall switch.
	// This means we will assume that the pwm is already powered and just set the _pwmPowered flag.
	// TODO: Really? Why can't we just organize this with events?
	bool switchcraftEnabled = State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED);

	if (switchcraftEnabled || (PWM_BOOT_DELAY_MS == 0) || _hardwareBoard == ACR01B10B || _hardwareBoard == ACR01B10C) {
		LOGd("dimmer powered on start");
		_pwmPowered = true;
		swSwitch->setDimmer(true);
		
		event_t event(CS_TYPE::EVT_DIMMER_POWERED, &_pwmPowered, sizeof(_pwmPowered));
		EventDispatcher::getInstance().dispatch(event);
	}
}

bool Switch::checkAndSetOwner(cmd_source_t source) {
	if (source.sourceId < CS_CMD_SOURCE_DEVICE_TOKEN) {
		// Non device token command can always set the switch.
		return true;
	}

	if (_ownerTimeoutCountdown == 0) {
		// Switch isn't claimed yet.
		_source = source;
		_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
		return true;
	}

	if (_source.sourceId != source.sourceId) {
		// Switch is claimed by other source.
		LOGd("Already claimed by %u", _source.sourceId);
		return false;
	}

	if (!BLEutil::isNewer(_source.count, source.count)) {
		// A command with newer counter has been received already.
		LOGd("Old command: %u, already got: %u", source.count, _source.count);
		return false;
	}

	_source = source;
	_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
	return true;
}