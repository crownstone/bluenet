/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2016
 * License: LGPLv3+
 */
#pragma once

#include <cfg/cs_Boards.h>

/*
	----------------------
	| GENERAL | PCB      |
	| PRODUCT | VERSION  |
	| INFO    |          |
	----------------------
	| 1 01 02 | 00 92 00 |
	----------------------
	  |  |  |    |  |  |---  Patch number of PCB version
	  |  |  |    |  |------  Minor number of PCB version
	  |  |  |    |---------  Major number of PCB version
	  |  |  |--------------  Product Type: 1 Dev, 2 Plug, 3 Builtin, 4 Guidestone
	  |  |-----------------  Market: 1 EU, 2 US
	  |--------------------  Family: 1 Crownstone
*/

// CROWNSTONE BUILTINS
#if (HARDWARE_BOARD==ACR01B1A)
#define HARDWARE_VERSION_STRING "10103000100"
#endif
#if (HARDWARE_BOARD==ACR01B1B)
#define HARDWARE_VERSION_STRING "10103000200"
#endif
#if (HARDWARE_BOARD==ACR01B1C)
#define HARDWARE_VERSION_STRING "10103000300"
#endif
#if (HARDWARE_BOARD==ACR01B1D)
#define HARDWARE_VERSION_STRING "10103000400"
#endif

// CROWNSTONE PLUGS
#if (HARDWARE_BOARD==ACR01B2A)
#define HARDWARE_VERSION_STRING "10102000100"
#endif
#if (HARDWARE_BOARD==ACR01B2B)
#define HARDWARE_VERSION_STRING "10102000200"
#endif
#if (HARDWARE_BOARD==ACR01B2C)
#define HARDWARE_VERSION_STRING "10102010000"
#endif

// GUIDESTONE
#if (HARDWARE_BOARD==GUIDESTONE)
#define HARDWARE_VERSION_STRING "10104010000"
#endif

/////////////////////////////////////////////////////////////////
//// Old and Third Party Boards
/////////////////////////////////////////////////////////////////

#if (HARDWARE_BOARD==VIRTUALMEMO)
#define HARDWARE_VERSION_STRING "VIRTUALMEMO"
#endif
#if (HARDWARE_BOARD==CROWNSTONE)
#define HARDWARE_VERSION_STRING "crownstone"
#endif
#if (HARDWARE_BOARD==CROWNSTONE2)
#define HARDWARE_VERSION_STRING "crownstone_v0.86"
#endif
#if (HARDWARE_BOARD==CROWNSTONE3)
#define HARDWARE_VERSION_STRING "crownstone_v0.90"
#endif
#if (HARDWARE_BOARD==CROWNSTONE4)
#define HARDWARE_VERSION_STRING "crownstone_v0.92"
#endif
#if (HARDWARE_BOARD==CROWNSTONE5)
#define HARDWARE_VERSION_STRING "plugin_quant"
#endif
#if (HARDWARE_BOARD==PLUGIN_FLEXPRINT_01)
#define HARDWARE_VERSION_STRING "plugin_flexprint_01"
#endif
#if (HARDWARE_BOARD==DOBEACON)
#define HARDWARE_VERSION_STRING "dobeacon_v0.7"
#endif
#if (HARDWARE_BOARD==DOBEACON2)
#define HARDWARE_VERSION_STRING "dobeacon_v?"
#endif

// Nordic Boards
#if (HARDWARE_BOARD==PCA10001)
#define HARDWARE_VERSION_STRING "PCA10001"
#endif
#if (HARDWARE_BOARD==NRF6310)
#define HARDWARE_VERSION_STRING "NRF6310"
#endif
#if (HARDWARE_BOARD==PCA10000)
#define HARDWARE_VERSION_STRING "PCA10000"
#endif
#if (HARDWARE_BOARD==PCA10031)
#define HARDWARE_VERSION_STRING "PCA10031"
#endif
#if (HARDWARE_BOARD==NORDIC_BEACON)
#define HARDWARE_VERSION_STRING "NORDIC_BEACON"
#endif
#if (HARDWARE_BOARD==PCA10036)
#define HARDWARE_VERSION_STRING "PCA10036"
#endif
#if (HARDWARE_BOARD==PCA10040)
#define HARDWARE_VERSION_STRING "PCA10040"
#endif

#ifndef HARDWARE_VERSION_STRING
#error "Add HARDWARE_VERSION_STRING to cfg/cs_HardwareVersions.h"
#endif

