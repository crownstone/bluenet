#pragma once

#include <cfg/cs_Boards.h>

#include <stdint.h>

/*
 * Define a series of strings. Will be filled in by a cmake configure step.
 */
extern const char g_BOOTLOADER_VERSION[];

extern const char g_COMPILATION_DAY[];

extern const char g_BUILD_TYPE[];

extern const uint8_t g_BOOTLOADER_IPC_RAM_PROTOCOL;

extern const uint16_t g_BOOTLOADER_DFU_VERSION;

extern const uint8_t g_BOOTLOADER_VERSION_MAJOR;

extern const uint8_t g_BOOTLOADER_VERSION_MINOR;

extern const uint8_t g_BOOTLOADER_VERSION_PATCH;

extern const uint8_t g_BOOTLOADER_VERSION_PRERELEASE;

extern const uint8_t g_BOOTLOADER_BUILD_TYPE;
