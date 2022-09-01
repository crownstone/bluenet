/**
 *
 */

#define LOGvv(fmt, ...) _log(SERIAL_VERY_VERBOSE, true, fmt, ##__VA_ARGS__)
#define LOGv(fmt, ...) _log(SERIAL_VERBOSE, true, fmt, ##__VA_ARGS__)
#define LOGd(fmt, ...) _log(SERIAL_DEBUG, true, fmt, ##__VA_ARGS__)
#define LOGi(fmt, ...) _log(SERIAL_INFO, true, fmt, ##__VA_ARGS__)
#define LOGw(fmt, ...) _log(SERIAL_WARN, true, fmt, ##__VA_ARGS__)
#define LOGe(fmt, ...) _log(SERIAL_ERROR, true, fmt, ##__VA_ARGS__)
#define LOGf(fmt, ...) _log(SERIAL_FATAL, true, fmt, ##__VA_ARGS__)

/**
 * Returns the 32 bits DJB2 hash of the reversed file name, up to the first '/'.
 */
constexpr uint32_t fileNameHash(const char* str, size_t strLen) {
	uint32_t hash = 5381;
	for (int32_t i = strLen - 1; i >= 0; --i) {
		if (str[i] == '/') {
			return hash;
		}
		hash = ((hash << 5) + hash) + str[i];  // hash * 33 + c
	}
	return hash;
}

void cs_log_start(size_t msgSize, uart_msg_log_header_t& header);
void cs_log_arg(const uint8_t* const valPtr, size_t valSize);
void cs_log_end();

void cs_log_array_no_fmt(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const uint8_t* const ptr,
		size_t size,
		ElementType elementType,
		size_t elementSize);

// This function takes unused format arguments.
// These formats ends up in the .ii file (preprocessed .cpp file), and is then used to gather all log formats.
// To make it easier for the compiler to optimize out the format strings, we only call cs_log_array_no_fmt without the
// format args here.
inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const uint8_t* const ptr,
		size_t size,
		ElementType elementType,
		size_t elementSize,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat) {
	cs_log_array_no_fmt(fileNameHash, lineNumber, logLevel, addNewLine, reverse, ptr, size, elementType, elementSize);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const uint8_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%3u") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			ptr,
			size,
			ELEMENT_TYPE_UNSIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const uint16_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%5u") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_UNSIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const uint32_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%10u") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_UNSIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const uint64_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%20u") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_UNSIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const int8_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%3i") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_SIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const int16_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%5i") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_SIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const int32_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%10i") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_SIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const int64_t* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%20i") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_SIGNED_INTEGER,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const float* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%f.") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_FLOAT,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

inline void cs_log_array(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		bool reverse,
		const double* const ptr,
		size_t size,
		const char* startFormat,
		const char* endFormat,
		const char* seperationFormat,
		const char* elementFormat = "%f.") {
	cs_log_array(
			fileNameHash,
			lineNumber,
			logLevel,
			addNewLine,
			reverse,
			reinterpret_cast<const uint8_t*>(ptr),
			size,
			ELEMENT_TYPE_FLOAT,
			sizeof(ptr[0]),
			startFormat,
			endFormat,
			seperationFormat,
			elementFormat);
}

template <typename T>
void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, T val) {
	size += sizeof(uart_msg_log_arg_header_t) + sizeof(T);
	++numArgs;
}

template <>
void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, char* str);

template <>
void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, const char* str);

template <typename T>
void cs_log_arg(T val) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(&val);
	cs_log_arg(valPtr, sizeof(T));
}

template <>
void cs_log_arg(char* str);

template <>
void cs_log_arg(const char* str);

// Uses the fold expression, a handy way to replace a recursive call.
template <class... Args>
void cs_log_args_no_fmt(
		uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const Args&... args) {
	uart_msg_log_header_t header;
	header.header.fileNameHash  = fileNameHash;
	header.header.lineNumber    = lineNumber;
	header.header.logLevel      = logLevel;
	header.header.flags.newLine = addNewLine;
	header.numArgs              = 0;

	// Get number of arguments and size of all arguments.
	size_t totalSize            = sizeof(header);
	(cs_log_add_arg_size(totalSize, header.numArgs, args), ...);

	// Write the header.
	cs_log_start(totalSize, header);

	// Write each argument.
	(cs_log_arg(args), ...);

	// Finalize the uart msg.
	cs_log_end();
}

// This function takes an unused argument "fmt".
// This fmt ends up in the .ii file (preprocessed .cpp file), and is then used to gather all log strings.
// To make it easier for the compiler to optimize out the fmt string, we only call cs_log_args without the fmt arg here.
template <class... Args>
void cs_log_args(
		uint32_t fileNameHash,
		uint32_t lineNumber,
		uint8_t logLevel,
		bool addNewLine,
		const char* fmt,
		const Args&... args) {
	cs_log_args_no_fmt(fileNameHash, lineNumber, logLevel, addNewLine, args...);
}


// Write logs as plain text.
#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
	void cs_log_printf(const char* str, ...);
	__attribute__((unused)) static bool _logPrefixPlainText = true;

	// Adding the file name and line number, adds a lot to the binary size.
	#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__) - 30 - 1) : __FILE__)

	#undef _log
	#define _log(level, addNewLine, fmt, ...)                         \
		if (level <= SERIAL_VERBOSITY) {                              \
			if (_logPrefixPlainText) {                                \
				cs_log_printf("[%-30.30s : %-4d] ", _FILE, __LINE__); \
			}                                                         \
			cs_log_printf(fmt, ##__VA_ARGS__);                        \
			if (addNewLine) {                                         \
				cs_log_printf("\r\n");                                \
			}                                                         \
			_logPrefixPlainText = addNewLine;                         \
		}

	#undef _logArray
	#define _logArray(level, addNewLine, pointer, size, ...)
#endif

// Write a string with printf functionality.
#ifdef HOST_TARGET
#pragma message("cs logger impl host target")
	__attribute__((unused)) static bool _logPrefixHost      = true;

	#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__) - 30 - 1) : __FILE__)

	#undef _log
	#define _log(level, addNewLine, fmt, ...)                  \
		if (level <= SERIAL_VERBOSITY) {                       \
			if (_logPrefixHost) {                              \
				printf("[%-30.30s : %-4d] ", _FILE, __LINE__); \
			}                                                  \
			printf(fmt, ##__VA_ARGS__);                        \
			if (addNewLine) {                                  \
				printf("\r\n");                                \
			}                                                  \
			_logPrefixHost = addNewLine;                       \
		}

	#undef _logArray
	#define _logArray(level, addNewLine, pointer, size, ...)
#endif
