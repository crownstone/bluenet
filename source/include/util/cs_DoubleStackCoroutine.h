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
 *   - uses a separate stack for each coroutine
 *   - is in plain C
 *   - uses setjmp and longjmp to jump back and forth
 *
 * We will use it in the following way:
 *
 * typedef struct {
 *   coroutine *c;
 *   ...
 * } coargs;
 *
 * // For example in delay code we call a function that ends up in bluenet and which calls yield 
 * void microapp_code(void *p) {
 *   coargs *args = (coargs*) p;
 *   ...
 *   yield(args->c);
 * }
 *
 * // Set up first call to bluenet (will call loop() down the line)
 * void bluenet_init() {
 *   coroutine c;
 *   // set stack pointer to end of stack for microapp
 *   coargs args = {&c};
 *   start(&c, &iterate, &args, *stack);
 * }
 * // we step until next returns negative, after that we can init again
 * void bluenet_step() {
 *   if (!next(&c)) {
 *     // We are done, at the end of the microapp code, next we can call loop() again
 *   }
 * }
 */

/**
 * We have a buffer to store our stack pointer and registers to if we go back and forth. There are only two coroutines
 * envisioned.
 */
typedef struct {
	jmp_buf calleeContext;
	jmp_buf callerContext;
} coroutine_t;

typedef void (*coroutineFunc)(void*);

/**
 * Start the coroutine.
 */
void start(coroutine_t* coroutine, coroutineFunc coroutineFunction, void* arg);

/**
 * Yield the coroutine.
 */
void yield(coroutine_t* coroutine);

/**
 * Resume
 */
int next(coroutine_t* coroutine);

