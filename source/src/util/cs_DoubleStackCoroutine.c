#include <util/cs_DoubleStackCoroutine.h>
#include <cfg/cs_StaticConfig.h>

#include <stddef.h>
#include <stdint.h>

enum { WORKING=1, DONE };

typedef struct {
	coroutine_t* coroutine;
	coroutineFunc coroutineFunction;
	void* arg;
	void* oldStackPointer;
} stack_params_t;

// Get stack pointer
#define get_sp(p) asm volatile("mov %0, sp" : "=r"(p) : : )

// Set stack pointer
#define set_sp(p) asm volatile("mov sp, %0" : : "r"(p))

// We hardcode the stack at the end of the RAM dedicated to the microapp
stack_params_t *const stack_params = (stack_params_t *)(uintptr_t)(g_RAM_MICROAPP_END);

/*
 * Starts a new coroutine. We store the parameters that we need when we switch back again to the current stack.
 */
void start(coroutine_t* coroutine, coroutineFunc coroutineFunction, void* arg) {
	// Save parameters before the coroutine stack (in hardcoded area, see above)
	stack_params->coroutine = coroutine;
	stack_params->coroutineFunction = coroutineFunction;
	stack_params->arg = arg;
	get_sp(stack_params->oldStackPointer);
	
	// Set stack pointer for the coroutine (just after that struct, growing downward)
	size_t move_down = sizeof(stack_params_t);
	set_sp(stack_params - move_down);

	if(!setjmp(stack_params->coroutine->calleeContext)) {
		set_sp(stack_params->oldStackPointer);
		return;
	}
		
	// call the function that will - in the end - yield
	(*stack_params->coroutineFunction)(stack_params->arg);

	// jump to caller
	longjmp(stack_params->coroutine->callerContext, DONE);
}

void yield(coroutine_t* coroutine) {
	if(!setjmp(coroutine->calleeContext)) {
		longjmp(coroutine->callerContext, WORKING);
	}
}

int next(coroutine_t* coroutine) {
	int ret = setjmp(coroutine->callerContext);
	if(!ret) {
		longjmp(coroutine->calleeContext, WORKING);
	}
	else {
		return ret == WORKING;
	}
}
