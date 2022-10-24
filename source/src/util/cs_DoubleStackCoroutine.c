#include <logging/cs_CLogger.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <util/cs_DoubleStackCoroutine.h>

enum { WORKING = 1, YIELDING_COROUTINE = 2, DONE };

stack_params_t* _stackParams = NULL;

// Get stack pointer
#define getStackPointer(p) asm volatile("mov %0, sp" : "=r"(p) : :)

// Set stack pointer
#define setStackPointer(p) asm volatile("mov sp, %0" : : "r"(p))

/*
 * Starts a new coroutine. We store the parameters that we need when we switch back again to the current stack.
 */
int initCoroutine(
		coroutine_function_t coroutineFunction, void* argument, uint8_t argumentSize, const uintptr_t ramEnd) {
	// Place the stack params at the end of the coroutine ram.
	_stackParams                    = ((stack_params_t*)ramEnd) - 1;
	_stackParams->coroutineFunction = coroutineFunction;

	if (argumentSize > sizeof(_stackParams->coroutineArgumentBuffer)) {
		return -1;
	}
	memcpy(_stackParams->coroutineArgumentBuffer, argument, argumentSize);

	// As soon as setStackPointer is called, only fields in stackParams can be used.
	// Between getStackPointer and setjmp, local variables should not be used (for example for logging).
	CLOGi("setStackPointer %p sizeof(stack_params_t)=%u sizeof(coroutine_t)=%u",
		  _stackParams,
		  sizeof(stack_params_t),
		  sizeof(coroutine_t));

	// Store current bluenet stack pointer in oldStackPointer.
	getStackPointer(_stackParams->oldStackPointer);

	// Initialize the coroutine stack pointer for the coroutine before the stack params.
	setStackPointer(_stackParams);

	// Save current context (registers) for the coroutine.
	if (setjmp(_stackParams->coroutine.coroutineContext) == 0) {
		// The call to setjmp always returns 0.
		// We have saved the context for the coroutine.
		// We first continue with bluenet as usual, so set the stack pointer back.
		setStackPointer(_stackParams->oldStackPointer);
		CLOGi("setStackPointer %p", _stackParams->oldStackPointer);
		return 0;
	}

	// We now come here from the first call to nextCoroutine, by a longjmp.
	// The stack pointer is now again at stackParams (at the end of the coroutine stack).

	// Call the very first instruction of this coroutine.
	// This method will - in the end - yield, but it should never return, as the coroutine has an infinite loop.
	(*_stackParams->coroutineFunction)(_stackParams->coroutineArgumentBuffer);

	// We should not end up here. This means that the coroutine function actually returns while it should always yield.
	longjmp(_stackParams->coroutine.bluenetContext, DONE);

	return -1;
}

/*
 * This function is called from the bluenet code base. The first time that setjmp is called it will save all
 * registers in the given buffer. It then resumes execution (returning 0). Now, when longjmp is given this buffer as
 * argument it restores all registers and execution resumes again from the same if statement but now setjmp returns 1
 * (or the value given by longjmp).
 */
int resumeCoroutine() {
	// Save current context (registers) for bluenet.
	int ret = setjmp(_stackParams->coroutine.bluenetContext);
	if (ret == 0) {
		// The first call to setjmp always returns 0.
		// The bluenet registers are saved.
		// Jump to the coroutine, we end up at the last setjmp with coroutine context, returning non 0.
		longjmp(_stackParams->coroutine.coroutineContext, WORKING);
	}
	else {
		// We come back from a longjmp from the coroutine.
		// For now, ret should be YIELDING_COROUTINE.
		return ret;
	}
	return 0;
}

/*
 * This function is always preceded by nextCoroutine. That's where it will be yielding to.
 */
void yieldCoroutine() {
	// Save current context (registers) for the coroutine.
	int ret = setjmp(_stackParams->coroutine.coroutineContext);
	if (ret == 0) {
		// The call to setjmp always return 0.
		// Context (registers) have been saved.
		// Jump to bluenet, we end up at the setjmp with bluenet context, returning non 0.
		longjmp(_stackParams->coroutine.bluenetContext, YIELDING_COROUTINE);
	}
	// Continue with coroutine code.
}

uint8_t* getCoroutineArgumentBuffer() {
	return _stackParams->coroutineArgumentBuffer;
}
