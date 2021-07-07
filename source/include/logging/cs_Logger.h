/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Logger class.
 *
 * Generally, you want to use: LOGnone, LOGv, LOGd, LOGi, LOGw, LOGe, or LOGf.
 * These can be used like printf(), except that they will always add a newline.
 *
 * With binary logging, the arguments are not first converted into plain text before sending them over the UART.
 * This is why templated and specialized functions are required for the implementation.
 * This saves a lot of bytes to be sent, and saves binary size, as the strings do not end up in the firmware.
 *
 * Since the log functions have an unknown amount of arguments, these macros require a variadic macro as well.
 * See http://www.wikiwand.com/en/Variadic_macro.
 * The two ## are e.g. a gcc specific extension that removes the , so that the ... arguments can also be left out.
 */

#ifdef HOST_TARGET
#include <stdio.h>
#endif

#include <cstdint>
#include <protocol/cs_UartMsgTypes.h>
#include <drivers/cs_Serial.h> // For SERIAL_VERBOSITY.
#include <cfg/cs_Strings.h> // Should actually be included by the files that use these.


// Deprecated, use LOGvv instead. To disable particular logs, but without commenting it.
#define LOGnone(fmt, ...)



#if !defined HOST_TARGET && (CS_SERIAL_NRF_LOG_ENABLED > 0)
	#define LOG_FLUSH NRF_LOG_FLUSH
#else
	#define LOG_FLUSH()
#endif

#if !defined HOST_TARGET && (CS_SERIAL_NRF_LOG_ENABLED > 0)
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
		#define _log(level, addNewLine, fmt, ...) \
				if (level <= SERIAL_VERBOSITY) { \
					NRF_LOG_DEBUG(fmt, ##__VA_ARGS__); \
				}
		#define _logArray(level, addNewLine, pointer, size, ...) \
				if (level <= SERIAL_VERBOSITY) { \
					NRF_LOG_HEXDUMP_DEBUG(pointer, size); \
				}
	#ifdef __cplusplus
	}
	#endif
#else

	#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY

		#define _log(level, addNewLine, fmt, ...) \
				if (level <= SERIAL_VERBOSITY) { \
					cs_log_args(fileNameHash(__FILE__, sizeof(__FILE__)), __LINE__, level, addNewLine, ##__VA_ARGS__); \
				}

		#define _logArray0(level, addNewLine, pointer, size) \
				if (level <= SERIAL_VERBOSITY) { \
					cs_log_array(fileNameHash(__FILE__, sizeof(__FILE__)), __LINE__, level, addNewLine, pointer, size); \
				}

		#define _logArray1(level, addNewLine, pointer, size, elementFmt) \
				if (level <= SERIAL_VERBOSITY) { \
					cs_log_array(fileNameHash(__FILE__, sizeof(__FILE__)), __LINE__, level, addNewLine, reinterpret_cast<const uint8_t* const>(pointer), size, ELEMENT_TYPE_FROM_FORMAT, sizeof(pointer[0])); \
				}

		// Helper function that returns argument 1 (which is once of the definitions).
		#define _logArrayGetArg1(arg0, arg1, arg2, ...) arg2

		// Helper function that returns a defined function with the number of arguments in VA_ARGS.
		#define _logArrayChooser(...) _logArrayGetArg1(##__VA_ARGS__, _logArray1, _logArray0)

//		#define _logArray(level, addNewLine, pointer, size, ...) _logArrayChooser(##__VA_ARGS__)(level, addNewLine, pointer, size, ##__VA_ARGS__)
		#define _logArray(level, addNewLine, pointer, size, ...) _logArray0(level, addNewLine, pointer, size, ##__VA_ARGS__)

	#else
		#define _log(level, addNewLine, fmt, ...)
		#define _logArray(level, addNewLine, pointer, size, ...)
	#endif

	#define LOGvv(fmt, ...) _log(SERIAL_VERY_VERBOSE, true, fmt, ##__VA_ARGS__)
	#define LOGv(fmt, ...) _log(SERIAL_VERBOSE, true, fmt, ##__VA_ARGS__)
	#define LOGd(fmt, ...) _log(SERIAL_DEBUG,   true, fmt, ##__VA_ARGS__)
	#define LOGi(fmt, ...) _log(SERIAL_INFO,    true, fmt, ##__VA_ARGS__)
	#define LOGw(fmt, ...) _log(SERIAL_WARN,    true, fmt, ##__VA_ARGS__)
	#define LOGe(fmt, ...) _log(SERIAL_ERROR,   true, fmt, ##__VA_ARGS__)
	#define LOGf(fmt, ...) _log(SERIAL_FATAL,   true, fmt, ##__VA_ARGS__)



	/**
	 * Returns the 32 bits DJB2 hash of the reversed file name, up to the first '/'.
	 *
	 * TODO: For some reason, if this returns a uint16_t, it increased the size by 3k?
	 */
	constexpr uint32_t fileNameHash(const char* str, size_t strLen) {
		uint32_t hash = 5381;
		for (size_t i = strLen-1; i >= 0; --i) {
			if (str[i] == '/') {
				return hash;
			}
			hash = ((hash << 5) + hash) + str[i]; // hash * 33 + c
		}
		return hash;
	}



	void cs_log_start(size_t msgSize, uart_msg_log_header_t &header);
	void cs_log_arg(const uint8_t* const valPtr, size_t valSize);
	void cs_log_end();

	void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint8_t* const ptr, size_t size, ElementType elementType, size_t elementSize);

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint8_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, ptr, size, ELEMENT_TYPE_UNSIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint16_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_UNSIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint32_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_UNSIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint64_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_UNSIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const int8_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_SIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const int16_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_SIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const int32_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_SIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const int64_t* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_SIGNED_INTEGER, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const float* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_FLOAT, sizeof(ptr[0]));
	}

	inline void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const double* const ptr, size_t size) {
		cs_log_array(fileNameHash, lineNumber, logLevel, addNewLine, reinterpret_cast<const uint8_t* const>(ptr), size, ELEMENT_TYPE_FLOAT, sizeof(ptr[0]));
	}




	template<typename T>
	void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, T val) {
		size += sizeof(uart_msg_log_arg_header_t) + sizeof(T);
		++numArgs;
	}

	template<>
	void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, char* str);

	template<>
	void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, const char* str);


	template<typename T>
	void cs_log_arg(T val) {
		const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(&val);
		cs_log_arg(valPtr, sizeof(T));
	}

	template<>
	void cs_log_arg(char* str);

	template<>
	void cs_log_arg(const char* str);


	// Uses the fold expression, a handy way to replace a recursive call.
	template<class... Args>
	void cs_log_args(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const Args&... args) {
		uart_msg_log_header_t header;
		header.header.fileNameHash = fileNameHash;
		header.header.lineNumber = lineNumber;
		header.header.logLevel = logLevel;
		header.header.flags.newLine = addNewLine;
		header.numArgs = 0;

		// Get number of arguments and size of all arguments.
		size_t totalSize = sizeof(header);
		(cs_log_add_arg_size(totalSize, header.numArgs, args), ...);

		// Write the header.
		cs_log_start(totalSize, header);

		// Write each argument.
		(cs_log_arg(args), ...);

		// Finalize the uart msg.
		cs_log_end();
	}

	// Write logs as plain text.
	#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
		void cs_log_printf(const char *str, ...);
		__attribute__((unused)) static bool _logPrefix = true;

		// Adding the file name and line number, adds a lot to the binary size.
		#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__)-30-1) : __FILE__)

		#undef _log
		#define _log(level, addNewLine, fmt, ...) \
				if (level <= SERIAL_VERBOSITY) { \
					if (_logPrefix) { \
						cs_log_printf("[%-30.30s : %-4d] ", _FILE, __LINE__); \
					} \
					cs_log_printf(fmt, ##__VA_ARGS__); \
					if (addNewLine) { \
						cs_log_printf("\r\n"); \
					} \
					_logPrefix = addNewLine; \
				}

		#undef _logArray
		#define _logArray(level, addNewLine, pointer, size, ...)
	#endif

	// Write a string with printf functionality.
	#ifdef HOST_TARGET
		__attribute__((unused)) static bool _logPrefix = true;

		#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__)-30-1) : __FILE__)

		#undef _log
		#define _log(level, addNewLine, fmt, ...) \
				if (level <= SERIAL_VERBOSITY) { \
					if (_logPrefix) { \
						printf("[%-30.30s : %-4d] ", _FILE, __LINE__); \
					} \
					printf(fmt, ##__VA_ARGS__); \
					if (addNewLine) { \
						printf("\r\n"); \
					} \
					_logPrefix = addNewLine; \
				}

		#undef _logArray
		#define _logArray(level, addNewLine, pointer, size, ...)
	#endif


#endif


#if SERIAL_VERBOSITY < SERIAL_VERY_VERBOSE
	#undef LOGvv
	#define LOGvv(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_VERBOSE
	#undef LOGv
	#define LOGv(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_DEBUG
	#undef LOGd
	#define LOGd(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_INFO
	#undef LOGi
	#define LOGi(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_WARN
	#undef LOGw
	#define LOGw(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_ERROR
	#undef LOGe
	#define LOGe(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_FATAL
	#undef LOGf
	#define LOGf(fmt, ...)
#endif
