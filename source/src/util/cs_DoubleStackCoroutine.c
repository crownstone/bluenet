#include <util/cs_DoubleStackCoroutine.h>

#include <stddef.h>
#include <stdint.h>

enum { WORKING=1, DONE };

void yield(coroutine* c) {
	if(!setjmp(c->callee_context)) {
		longjmp(c->caller_context, WORKING);
	}
}

int next(coroutine* c) {
	int ret = setjmp(c->caller_context);
	if(!ret) {
		longjmp(c->callee_context, 1);
	}
	else {
		return ret == WORKING;
	}
}

typedef struct {
	coroutine* c;
	coroutine_func f;
	void* arg;
	void* old_sp;
} start_params;

#define get_sp(p) asm volatile("mov %0, sp" : "=r"(p) : : )
#define set_sp(p) asm volatile("mov sp, %0" : : "r"(p))

// We hardcode the stack at the end of the RAM dedicated to the microapp
start_params *const params = (start_params *)(uintptr_t)(RAM_MICROAPP_BASE + RAM_MICROAPP_AMOUNT);

void start(coroutine* c, coroutine_func f, void* arg) {
	//save parameters before stack switching
	params->c = c;
	params->f = f;
	params->arg = arg;
	get_sp(params->old_sp);
	
	// set stack pointer for the coroutine (starts with start_params and goes down)
	size_t move_down = sizeof(start_params);
	set_sp(params - move_down);

	if(!setjmp(params->c->callee_context)) {
		set_sp(params->old_sp);
		return;
	}
		
	// call the function that will - in the end - yield
	(*params->f)(params->arg);

	// jump to caller
	longjmp(params->c->caller_context, DONE);
}
