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

//! Max buffer size for coroutine arguments.
#define DOUBLE_STACK_COROUTINE_ARGUMENTS_BUFFER_SIZE 8

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
	uint8_t coroutineArgumentBuffer[DOUBLE_STACK_COROUTINE_ARGUMENTS_BUFFER_SIZE];
	void* oldStackPointer;
} stack_params_t;

/**
 * Initialize the coroutine.
 *
 * Make sure to first set the coroutine arguments.
 *
 * @param[in] coroutineFunction   The first function of the coroutine to be executed.
 * @param[in] argument            The argument for the coroutine function, this will be copied,
 *                                and can be retrieved again with getCoroutineArgumentBuffer().
 * @param[in] argumentSize        Size of the argument.
 * @param[in] ramEnd              Pointer to the end of the ram reserved for this coroutine.
 * @return                        0 on success.
 */
int initCoroutine(coroutine_function_t coroutineFunction, void* argument, uint8_t argumentSize, const uintptr_t ramEnd);

/**
 * Yield the coroutine: resume in bluenet.
 */
void yieldCoroutine();

/**
 * Resume the coroutine.
 *
 * @return 0       When the coroutine finished.
 */
int resumeCoroutine();

/**
 * Get the coroutine argument buffer.
 */
uint8_t* getCoroutineArgumentBuffer();
