/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <testaccess/cs_SwitchBehaviour.h>
#include <behaviour/cs_BehaviourStore.h>
#include <testaccess/cs_BehaviourHandler.h>
#include <presence/cs_PresenceHandler.h>

#include <iostream>

/**
 * Checks if the _behaviourHandler resolves to the switch behaviour in the _behaviourStore with the given
 * expectedBehaviour(Index) and presence. If not, print a message with the line number for reference.

 * @return true on correct resolution, false otherwise.
 */
bool checkCase(BehaviourHandler& _behaviourHandler, BehaviourStore& _behaviourStore, SwitchBehaviour* expected, PresenceStateDescription presence, int line) {
    Time testTime(DayOfWeek::Tuesday, 12, 0);
    SwitchBehaviour* resolved = TestAccess<BehaviourHandler>::resolveSwitchBehaviour(_behaviourHandler, testTime, presence);

    if (resolved == expected && expected != nullptr) {
        return true;
    } else {
        std::cout << "FAILED test at line: " << line << std::endl;

    	if(expected != nullptr) {
			std::cout << "expected:" << *expected << std::endl;
		} else {
			std::cout << "expected:" << "nullptr" << std::endl;
		}
        if(resolved != nullptr) {
            std::cout << "resolved:" << *resolved << std::endl;
        } else {
            std::cout << "resolved:" << "nullptr" << std::endl;
        }
        std::cout << "time: " << testTime << std::endl;
        std::cout << "presence:" << presence << std::endl;

        return false;
    }
}

bool checkCase(BehaviourHandler& _behaviourHandler, BehaviourStore& _behaviourStore, int expectedBehaviourIndex, PresenceStateDescription presence, int line) {
    SwitchBehaviour* expected = dynamic_cast<SwitchBehaviour*>(_behaviourStore.getBehaviour(expectedBehaviourIndex));
    if(! checkCase (_behaviourHandler, _behaviourStore, expected, presence, line)) {
		std::cout << "index: " << expectedBehaviourIndex << std::endl;
    	return false;
    }
    return true;
}
