/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

// resistance of the shunt in milli ohm
#define SHUNT_VALUE                              120

// size of the buffer used for characteristics
#define GENERAL_BUFFER_SIZE                      140

/* maximum length of strings used for characteristic values */
#define MAX_STRING_LENGTH                        25

/* Command to enter the bootloader and stay there.
 *
 * This should be the same value as defined in the bootloader. 
 */
#define COMMAND_ENTER_RADIO_BOOTLOADER           66

/*
 */
#define APP_TIMER_PRESCALER                      240
#define APP_TIMER_MAX_TIMERS                     4
#define APP_TIMER_OP_QUEUE_SIZE                  4
