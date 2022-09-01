/**
 * Only to be directly included in cs_Logger.h.
 *
 * Defines macros for:
 *   _log, forwarding to cs_log_args,
 *   _logArray, forwarding to cs_log_array.
 */


#define _log(level, addNewLine, fmt, ...)                                                                       \
	if (level <= SERIAL_VERBOSITY) {                                                                            \
		cs_log_args(fileNameHash(__FILE__, sizeof(__FILE__)), __LINE__, level, addNewLine, fmt, ##__VA_ARGS__); \
	}


// No manual formatting: uses default format based on element type.
#define _logArray0(level, addNewLine, pointer, size)      \
	if (level <= SERIAL_VERBOSITY) {                      \
		cs_log_array(                                     \
				fileNameHash(__FILE__, sizeof(__FILE__)), \
				__LINE__,                                 \
				level,                                    \
				addNewLine,                               \
				false,                                    \
				pointer,                                  \
				size,                                     \
				"[",                                      \
				"]",                                      \
				", ");                                    \
	}

// Manual element format, but default start and end format.
#define _logArray1(level, addNewLine, pointer, size, elementFmt) \
	if (level <= SERIAL_VERBOSITY) {                             \
		cs_log_array(                                            \
				fileNameHash(__FILE__, sizeof(__FILE__)),        \
				__LINE__,                                        \
				level,                                           \
				addNewLine,                                      \
				false,                                           \
				pointer,                                         \
				size,                                            \
				"[",                                             \
				"]",                                             \
				", ",                                            \
				elementFmt);                                     \
	}

// Manual start and end format. Default element format, based on element type.
#define _logArray2(level, addNewLine, pointer, size, startFmt, endFmt) \
	if (level <= SERIAL_VERBOSITY) {                                   \
		cs_log_array(                                                  \
				fileNameHash(__FILE__, sizeof(__FILE__)),              \
				__LINE__,                                              \
				level,                                                 \
				addNewLine,                                            \
				false,                                                 \
				pointer,                                               \
				size,                                                  \
				startFmt,                                              \
				endFmt,                                                \
				", ");                                                 \
	}

// Manual start and end format. Default element separation format.
#define _logArray3(level, addNewLine, pointer, size, startFmt, endFmt, elementFmt) \
	if (level <= SERIAL_VERBOSITY) {                                               \
		cs_log_array(                                                              \
				fileNameHash(__FILE__, sizeof(__FILE__)),                          \
				__LINE__,                                                          \
				level,                                                             \
				addNewLine,                                                        \
				false,                                                             \
				pointer,                                                           \
				size,                                                              \
				startFmt,                                                          \
				endFmt,                                                            \
				", ",                                                              \
				elementFmt);                                                       \
	}

// Manual format.
#define _logArray4(level, addNewLine, pointer, size, startFmt, endFmt, elementFmt, seperationFmt) \
	if (level <= SERIAL_VERBOSITY) {                                                              \
		cs_log_array(                                                                             \
				fileNameHash(__FILE__, sizeof(__FILE__)),                                         \
				__LINE__,                                                                         \
				level,                                                                            \
				addNewLine,                                                                       \
				false,                                                                            \
				pointer,                                                                          \
				size,                                                                             \
				startFmt,                                                                         \
				endFmt,                                                                           \
				seperationFmt,                                                                    \
				elementFmt);                                                                      \
	}

// Manual format.
#define _logArray5(level, addNewLine, pointer, size, startFmt, endFmt, elementFmt, seperationFmt, reverse) \
	if (level <= SERIAL_VERBOSITY) {                                                                       \
		cs_log_array(                                                                                      \
				fileNameHash(__FILE__, sizeof(__FILE__)),                                                  \
				__LINE__,                                                                                  \
				level,                                                                                     \
				addNewLine,                                                                                \
				reverse,                                                                                   \
				pointer,                                                                                   \
				size,                                                                                      \
				startFmt,                                                                                  \
				endFmt,                                                                                    \
				seperationFmt,                                                                             \
				elementFmt);                                                                               \
	}

// Helper function that returns argument 5 (which is one of the _logArrayX definitions).
#define _logArrayGetArg5(arg0, arg1, arg2, arg3, arg4, arg5, arg6, ...) arg6

#define _logArray(level, addNewLine, pointer, ...)                                                         \
	_logArrayGetArg5(__VA_ARGS__, _logArray5, _logArray4, _logArray3, _logArray2, _logArray1, _logArray0)( \
			level, addNewLine, pointer, __VA_ARGS__)
