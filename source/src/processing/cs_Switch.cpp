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

