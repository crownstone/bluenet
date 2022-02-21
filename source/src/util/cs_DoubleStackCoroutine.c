#include <cfg/cs_StaticConfig.h>
#include <stddef.h>
#include <stdint.h>
#include <util/cs_DoubleStackCoroutine.h>

enum { WORKING = 1, YIELDING_MICROAPP0 = 2, DONE };

// Get stack pointer
#define get_sp(p) asm volatile("mov %0, sp" : "=r"(p) : :)

// Set stack pointer
#define set_sp(p) asm volatile("mov sp, %0" : : "r"(p))

// We hardcode the stack at the end of the RAM dedicated to the microapp
stack_params_t* const stack_params = (stack_params_t*)(uintptr_t)(g_RAM_MICROAPP_END);

stack_params_t* getStackParams() {
	return stack_params;
}

/*
 * Starts a new coroutine. We store the parameters that we need when we switch back again to the current stack.
 */
void startCoroutine(coroutine_t* coroutine, coroutineFunc coroutineFunction, void* arg) {
	// Save parameters before the coroutine stack (in hardcoded area, see above).
	stack_params->coroutine         = coroutine;
	stack_params->coroutineFunction = coroutineFunction;
	stack_params->arg               = arg;
	// Store current stack pointer in oldStackPointer.
	get_sp(stack_params->oldStackPointer);

	// Set the stack pointer for the microapp to where we want it.
	size_t move_down = sizeof(stack_params_t);
	set_sp(stack_params - move_down);

	// Save current context (registers) for the microapp.
	int ret = setjmp(stack_params->coroutine->microapp0Context);
	if (ret == 0) {
		// We have saved the context for the microapp, but set stack pointer back.
		// We first continue with bluenet as usual.
		set_sp(stack_params->oldStackPointer);
		return;
	}

	// We now come here from the first call to nextCoroutine.
	// The stack pointer is now again at stack_params - move_down.

	// Call the very first function in the microapp. This will in the end yield.
	(*stack_params->coroutineFunction)(stack_params->arg);

	// We should not end up here. This means that the microapp function actually returns while it should always yield.
	longjmp(stack_params->coroutine->bluenetContext, DONE);
}

/*
 * The nextCoroutine is called from the bluenet code base. The first time that setjmp is called it will save all
 * registers in the given buffer. It then resumes execution (returning 0). Now, when longjmp is given this buffer as
 * argument it restores all registers and execution resumes again from the same if statement but now setjmp returns 1
 * (or the value given by longjmp).
 */
int nextCoroutine(coroutine_t* cor) {
	// Save current context (registers) for bluenet.
	coroutine_t* coroutine = stack_params->coroutine;
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
void yieldCoroutine(coroutine_t* cor) {
	// Save current context (registers) for microapp 0.
	coroutine_t* coroutine = stack_params->coroutine;
	int ret                = setjmp(coroutine->microapp0Context);
	if (ret == 0) {
		// Context (registers) have been saved. Jump to bluenet.
		longjmp(coroutine->bluenetContext, YIELDING_MICROAPP0);
	}
	// continue with microapp code
}
