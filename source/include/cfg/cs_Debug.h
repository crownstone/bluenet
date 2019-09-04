/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 30, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

/////////////////////////////////////////////////////
///// group enables
/////////////////////////////////////////////////////

#define STACK_VERBOSE
//#define MESH_VERBOSE
//#define DRIVERS_VERBOSE

/////////////////////////////////////////////////////
///// single enables
/////////////////////////////////////////////////////

#define PRINT_STACK_VERBOSE
//#define PRINT_CHARACTERISTIC_VERBOSE

//#define PRINT_IBEACON_VERBOSE

//#define PRINT_ADC_VERBOSE
//#define PRINT_LPCOMP_VERBOSE
//#define PRINT_PWM_VERBOSE
//#define PRINT_STORAGE_VERBOSE

//#define PRINT_EVENTDISPATCHER_VERBOSE

//#define PRINT_MESH_VERBOSE
//#define PRINT_MESHCONTROL_VERBOSE

//#define PRINT_FRIDGE_VERBOSE
//#define PRINT_POWERSAMPLING_VERBOSE
#define PRINT_SCANNER_VERBOSE
#define PRINT_SWITCH_VERBOSE
//#define PRINT_TRACKER_VERBOSE

//#define PRINT_POWERSAMPLES_VERBOSE
//#define PRINT_SCANRESULT_VERBOSE

//#define PRINT_CIRCULARBUFFER_VERBOSE
//#define PRINT_SCHEDULEENTRIES_VERBOSE
//#define PRINT_STREAMBUFFER_VERBOSE
//#define PRINT_TRACKEDDEVICES_VERBOSE


/////////////////////////////////////////////////////

#ifdef STACK_VERBOSE
//#define PRINT_CHARACTERISTIC_VERBOSE
#define PRINT_STACK_VERBOSE
#endif

#ifdef MESH_VERBOSE
#define PRINT_MESH_VERBOSE
#define PRINT_MESHCONTROL_VERBOSE
#endif

#ifdef DRIVERS_VERBOSE
#define PRINT_ADC_VERBOSE
#define PRINT_LPCOMP_VERBOSE
#define PRINT_PWM_VERBOSE
#define PRINT_STORAGE_VERBOSE
#endif
