/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 28, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <setjmp.h>

/**
 * We introduce here the DoubleStackCoroutine because the term Coroutine already has been taken in cs_Coroutine.h.
 * The coroutine implementation:
 *   - is used to switch back and forth from microapp code and bluenet code
 *   - is especially used because a delay(..) function from microapp code should not be blocking
 *   - uses a separate stack for the coroutine
 *   - is in plain C
 *   - uses setjmp and longjmp to jump back and forth
 */

/**
 * We have a buffer to store our stack pointer and registers to if we go back and forth. There are only two coroutines
 * envisioned.
 */
typedef struct {
	jmp_buf microapp0Context;
	jmp_buf bluenetContext;
} coroutine_t;

typedef void (*coroutineFunc)(void*);

/**
 * Start the coroutine.
 */
void startCoroutine(coroutine_t* coroutine, coroutineFunc coroutineFunction, void* arg);

/**
 * Yield the coroutine.
 */
void yieldCoroutine(coroutine_t* coroutine);

/**
 * Resume the coroutine.
 */
int nextCoroutine(coroutine_t* coroutine);

