#include <stddef.h>
#include <stdint.h>
#include <util/cs_DoubleStackCoroutine.h>
#include <logging/cs_CLogger.h>

enum { WORKING = 1, YIELDING_COROUTINE = 2, DONE };

// Get stack pointer
#define getStackPointer(p) asm volatile("mov %0, sp" : "=r"(p) : :)

// Set stack pointer
#define setStackPointer(p) asm volatile("mov sp, %0" : : "r"(p))

stack_params_t _stackParams;

/*
 * Starts a new coroutine. We store the parameters that we need when we switch back again to the current stack.
 */
void startCoroutine(coroutine_function_t coroutineFunction, void* argument, const uintptr_t ramEnd) {
	_stackParams.coroutineFunction  = coroutineFunction;
	_stackParams.coroutineArguments = argument;

	// Store current stack pointer in oldStackPointer.
	getStackPointer(_stackParams.oldStackPointer);

	// Set the stack pointer for the microapp to where we want it.
	setStackPointer(ramEnd);
	CLOGi("setStackPointer %p", ramEnd);


	// Save current context (registers) for the microapp.
	int ret = setjmp(_stackParams.coroutine.coroutineContext);
	if (ret == 0) {
		// We have saved the context for the microapp, but set stack pointer back.
		// We first continue with bluenet as usual.
		setStackPointer(_stackParams.oldStackPointer);
		CLOGi("setStackPointer %p", _stackParams.oldStackPointer);
		return;
	}

	// We now come here from the first call to nextCoroutine.
	// The stack pointer is now again at stackParams - move_down.

	// Call the very first function in the microapp. This will in the end yield.
	(*_stackParams.coroutineFunction)(_stackParams.coroutineArguments);

	// We should not end up here. This means that the microapp function actually returns while it should always yield.
	longjmp(_stackParams.coroutine.bluenetContext, DONE);
}

/*
 * The nextCoroutine is called from the bluenet code base. The first time that setjmp is called it will save all
 * registers in the given buffer. It then resumes execution (returning 0). Now, when longjmp is given this buffer as
 * argument it restores all registers and execution resumes again from the same if statement but now setjmp returns 1
 * (or the value given by longjmp).
 */
int nextCoroutine() {
	// Save current context (registers) for bluenet.
	int ret = setjmp(_stackParams.coroutine.bluenetContext);
	if (ret == 0) {
		// The bluenet registers are saved, fine now to jump to the microapp.
		longjmp(_stackParams.coroutine.coroutineContext, WORKING);
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
	int ret                = setjmp(_stackParams.coroutine.coroutineContext);
	if (ret == 0) {
		// Context (registers) have been saved. Jump to bluenet.
		longjmp(_stackParams.coroutine.bluenetContext, YIELDING_COROUTINE);
	}
	// continue with microapp code
}

void* getCoroutineArguments() {
	return _stackParams.coroutineArguments;
}
