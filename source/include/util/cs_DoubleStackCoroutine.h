/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 28, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <setjmp.h>

/**
 * This class implements a coroutine with its own stack.
 *
 * The coroutine implementation:
 * - Makes it possible to stop the coroutine half way a function, and resume from there later.
 * - Is used by the microapp, so that blocking calls can be used in the microapp code, without blocking bluenet.
 * - Uses setjmp and longjmp to jump back and forth.
 */

/**
 * Struct with the context to jump between bluenet and coroutine.
 */
typedef struct {
	jmp_buf coroutineContext;
	jmp_buf bluenetContext;
} coroutine_t;

typedef void (*coroutine_function_t)(void*);

/**
 * Struct with all the state we need for a coroutine.
 */
typedef struct {
	coroutine_t coroutine;
	coroutine_function_t coroutineFunction;
	void* coroutineArguments;
	void* oldStackPointer;
} stack_params_t;

/**
 * Start the coroutine.
 *
 * @param[in] coroutineFunction   The first function of the coroutine to be executed.
 * @param[in] argument            The argument for the coroutine function.
 * @param[in] ramEnd              Pointer to the end of the ram reserved for this coroutine.
 */
void startCoroutine(coroutine_function_t coroutineFunction, void* argument, const uintptr_t ramEnd);

/**
 * Yield the coroutine: resume in bluenet.
 */
void yieldCoroutine();

/**
 * Resume the coroutine.
 *
 * @return 0       When the coroutine finished.
 */
int nextCoroutine();

/**
 * Get the coroutine arguments.
 */
void* getCoroutineArguments();
