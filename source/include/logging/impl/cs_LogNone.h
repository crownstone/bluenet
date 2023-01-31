/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 1, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Only to be directly included in cs_Logger.h.
 *
 * This implementation of _log/_logArray just swallows its arguments.
 */

#define _log(level, addNewLine, fmt, ...)
#define _logArray(level, addNewLine, pointer, size, ...)
