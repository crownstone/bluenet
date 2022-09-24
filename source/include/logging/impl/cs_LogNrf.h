/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 1, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Implementation details for the case
 *
 * 			!defined HOST_TARGET && (CS_SERIAL_NRF_LOG_ENABLED > 0)
 *
 * Only to be directly included in cs_Logger.h.
 */

// Use NRF Logger instead.
#ifdef __cplusplus
extern "C" {
#endif

#define _log(level, addNewLine, fmt, ...)  \
	if (level <= SERIAL_VERBOSITY) {       \
		NRF_LOG_DEBUG(fmt, ##__VA_ARGS__); \
	}

#define _logArray(level, addNewLine, pointer, size, ...) \
	if (level <= SERIAL_VERBOSITY) {                     \
		NRF_LOG_HEXDUMP_DEBUG(pointer, size);            \
	}

#ifdef __cplusplus
}
#endif
