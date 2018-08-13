/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 9 Aug., 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Config.h>
#include <storage/cs_State.h>


/**
 * The configuration parameters here will have the following format:
 *   LABEL
 * and
 *   CONFIG_LABEL_DEFAULT
 * It would be cumbersome for the user to type CONFIG and DEFAULT all the time, so the incoming macro is assumed
 * to be just LABEL. Hence, you will have a macro that can be used within the firmware:
 *
 *   #ifdef LABEL
 *     #define CONFIG_LABEL_DEFAULT LABEL
 *   #else
 *     #define CONFIG_LABEL_DEFAULT ...
 *   #endif
 *
 * In this way we will be able to quickly rewrite the code if there is a conflict. For example, prepending each
 * config macro with a Crownstone specific prefix (parallel to the Nordic prefix, NRF_, we might go for CBN_ as
 * Crownstone bluenet abbreviation). If we use a short abbreviation we can have exactly the same macro for the
 * user.
 *
 * There are values that indicate if parts of the code base will be compiled such as:
 *
 *   CBN_MESH_COMPILED
 *
 *   CBN_MESH_ENABLED
 *   CBN_ENCRYPTION_ENABLED
 *   CBN_IBEACON_ENABLED
 *   CBN_SCANNER_ENABLED
 *   CBN_POWER_SAMPLER_ENABLED
 *   CBN_RELAY_START_STATE
 *   CBN_PWM_ENABLED
 */

#if defined MESHING 
#define CONFIG_MESH_DEFAULT MESHING
#else
#define CONFIG_MESH_DEFAULT 0
#endif

#if defined ENCRYPTION
#define CONFIG_ENCRYPTION_DEFAULT ENCRYPTION
#else
#define CONFIG_ENCRYPTION_DEFAULT 0
#endif

#if defined IBEACON 
#define CONFIG_IBEACON_DEFAULT IBEACON
#else
#define CONFIG_IBEACON_DEFAULT 0
#endif

#if defined INTERVAL_SCANNER_ENABLED 
#define CONFIG_SCANNER_DEFAULT INTERVAL_SCANNER_ENABLED
#else
#define CONFIG_SCANNER_DEFAULT 0
#endif

#if defined CONTINUOUS_POWER_SAMPLER 
#define CONFIG_POWER_SAMPLER_DEFAULT CONTINUOUS_POWER_SAMPLER
#else
#define CONFIG_POWER_SAMPLER_DEFAULT 0
#endif

#if defined DEFAULT_ON 
#define CONFIG_RELAY_START_DEFAULT DEFAULT_ON
#else
#define CONFIG_RELAY_START_DEFAULT 0
#endif

#if defined PWM 
#define CONFIG_PWM_DEFAULT PWM
#else
#define CONFIG_PWM_DEFAULT 0
#endif

#if defined SWITCH_LOCK 
#define CONFIG_SWITCH_LOCK_DEFAULT SWITCH_LOCK
#else
#define CONFIG_SWITCH_LOCK_DEFAULT 0
#endif

#if defined CONFIG_FLOOR 
#define CONFIG_FLOOR_DEFAULT CONFIG_FLOOR 
#else
#define CONFIG_FLOOR_DEFAULT 0
#endif

#if defined SWITCHCRAFT 
#define CONFIG_SWITCHCRAFT_DEFAULT SWITCHCRAFT
#else
#define CONFIG_SWITCHCRAFT_DEFAULT 0
#endif
	    
#if defined CONFIG_SCANNER_NEARBY_TIMEOUT_DEFAULT
#else
#define CONFIG_SCANNER_NEARBY_TIMEOUT_DEFAULT 1
#endif

#if defined CROWNSTONE_ID
#define CONFIG_CROWNSTONE_ID_DEFAULT CROWNSTONE_ID
#else
#define CONFIG_CROWNSTONE_ID_DEFAULT 0
#endif

#if defined CONFIG_BOOT_DELAY_DEFAULT
#else
#define CONFIG_BOOT_DELAY_DEFAULT 0
#endif

#if defined CONFIG_LOW_TX_POWER_DEFAULT
#else
#define CONFIG_LOW_TX_POWER_DEFAULT 0
#endif

#if defined CONFIG_VOLTAGE_MULTIPLIER_DEFAULT
#else
#define CONFIG_VOLTAGE_MULTIPLIER_DEFAULT 1
#endif

#if defined CONFIG_CURRENT_MULTIPLIER_DEFAULT
#else
#define CONFIG_CURRENT_MULTIPLIER_DEFAULT 1
#endif

#if defined CONFIG_VOLTAGE_ZERO_DEFAULT
#else
#define CONFIG_VOLTAGE_ZERO_DEFAULT 0
#endif

#if defined CONFIG_CURRENT_ZERO_DEFAULT
#else
#define CONFIG_CURRENT_ZERO_DEFAULT 0
#endif

#if defined CONFIG_POWER_ZERO_DEFAULT
#else
#define CONFIG_POWER_ZERO_DEFAULT 0
#endif

#if defined CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP_DEFAULT
#else
#define CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP_DEFAULT 0
#endif

#ifndef CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN_DEFAULT
#define CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN_DEFAULT 0
#endif


void getDefaults(uint8_t configurationType, void* default_type, size_t & default_size);
