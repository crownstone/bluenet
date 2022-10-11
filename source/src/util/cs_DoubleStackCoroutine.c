#include <cfg/cs_StaticConfig.h>
#include <stddef.h>
#include <stdint.h>
#include <util/cs_DoubleStackCoroutine.h>

enum { WORKING = 1, YIELDING_MICROAPP0 = 2, DONE };

// Get stack pointer
#define getStackPointer(p) asm volatile("mov %0, sp" : "=r"(p) : :)

// Set stack pointer
#define setStackPointer(p) asm volatile("mov sp, %0" : : "r"(p))

// We hardcode the stack at the end of the RAM dedicated to the microapp
stack_params_t* const stackParams = (stack_params_t*)(uintptr_t)(g_RAM_MICROAPP_END) - 1;

stack_params_t* getStackParams() {
	return stackParams;
}

/*
 * Starts a new coroutine. We store the parameters that we need when we switch back again to the current stack.
 * As soon as setStackPointer is called, only fields through stackParams are valid. Between setStackPointer and
 * setjmp local variables should not be used (for example for logging).
 */
void startCoroutine(coroutine_t* coroutine, coroutineFunc coroutineFunction, void* arg) {
	// Save parameters before the coroutine stack (in hardcoded area, see above).
	stackParams->coroutine         = coroutine;
	stackParams->coroutineFunction = coroutineFunction;
	stackParams->arg               = arg;
	// Store current stack pointer in oldStackPointer.
	getStackPointer(stackParams->oldStackPointer);

	// Set the stack pointer for the microapp to the end of the microapp RAM space just below the stackParams struct.
	setStackPointer(stackParams);

	// Save current context (registers) for the microapp.
	int ret = setjmp(stackParams->coroutine->microapp0Context);
	if (ret == 0) {
		// We have saved the context for the microapp, but set the stack pointer back.
		// We first continue with bluenet as usual.
		setStackPointer(stackParams->oldStackPointer);
		return;
	}

	// We now come here from the first call to nextCoroutine.
	// The stack pointer is now again at stackParams (at the end of the stack)

	// Call the very first instruction of this coroutine.
	// This is the function `goIntoMicroapp()` and is given the address `coargs->entry` to jump to.
	// This method will - in the end - yield, but it should never return. The microapp has an infinite loop.
	(*stackParams->coroutineFunction)(stackParams->arg);

	// We should not end up here. This means that the microapp function actually returns while it should always yield.
	longjmp(stackParams->coroutine->bluenetContext, DONE);
}

/*
 * The nextCoroutine is called from the bluenet code base. The first time that setjmp is called it will save all
 * registers in the given buffer. It then resumes execution (returning 0). Now, when longjmp is given this buffer as
 * argument it restores all registers and execution resumes again from the same if statement but now setjmp returns 1
 * (or the value given by longjmp).
 */
int nextCoroutine() {
	// Save current context (registers) for bluenet.
	coroutine_t* coroutine = stackParams->coroutine;
	int ret                = setjmp(coroutine->bluenetContext);
	if (ret == 0) {
		// The bluenet registers are saved, fine now to jump to the microapp.
		longjmp(coroutine->microapp0Context, WORKING);
	}
	else {
		// We come back from a longjmp from the microapp.
		// For now, there's only one yieldCoroutine which will return:
		//   ret = YIELDING_MICROAPP0
		return ret;
	}
}

/*
 * This function is always preceded by nextCoroutine. That's where it will be yielding to.
 */
void yieldCoroutine() {
	// Save current context (registers) for microapp 0.
	coroutine_t* coroutine = stackParams->coroutine;
	int ret                = setjmp(coroutine->microapp0Context);
	if (ret == 0) {
		// Context (registers) have been saved. Jump to bluenet.
		longjmp(coroutine->bluenetContext, YIELDING_MICROAPP0);
	}
	// continue with microapp code
}
