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
#define LOGvv NRF_LOG_DEBUG
#define LOGv NRF_LOG_DEBUG
#define LOGd NRF_LOG_DEBUG
#define LOGi NRF_LOG_INFO
#define LOGw NRF_LOG_WARNING
#define LOGe NRF_LOG_ERROR
#define LOGf NRF_LOG_ERROR

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
