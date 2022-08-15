/**
 * Author: Christopher Mason
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
//#ifdef SYSCALLS

extern int errno;

int _kill(int pid, int sig) {
//!    pid = pid; sig = sig;/** avoid warnings */
#define EINVAL 22
	errno = EINVAL;
	return -1;
}

void _exit(int status) {
	//!    xprintf("_exit called with parameter %d\n", status);
	while (1) {
		;
	}
}

int _getpid(void) {
	return 1;
}

int _close(int file) {
	//!    file = file;/** avoid warning
	return -1;
}

int _fstat(int file, void* st) {
	//!    file = file;/** avoid warning */
	return 0;
}

int _isatty(int file) {
	//!    file = file;/** avoid warning */
	return 1;
}

int _lseek(int file, int ptr, int dir) {
	//!    file = file;/** avoid warning
	//!    ptr = ptr;/** avoid warning
	//!    dir = dir;/** avoid warning
	return 0;
}

int _read(int file, char* ptr, int len) {
	//!    file = file;/** avoid warning */
	//!    ptr = ptr;/** avoid warning */
	//!    len = len;/** avoid warning */
	return 0;
}

int _write(int file, char* ptr, int len) {
	int todo;
	//!    file = file;/** avoid warning */
	for (todo = 0; todo < len; todo++) {
		//!        xputc(*ptr++);
	}
	return len;
}

void fsync() {}

//#define LET_HEAP_BE_FIXED
#define LET_HEAP_GROW_TO_STACKPOINTER

/**
 * The allocation of memory by malloc etc. uses this function internally. The end of the heap is set to _ebss in the
 * linker script.
 * Check also: https://devzone.nordicsemi.com/question/14581/nrf51822-with-gcc-stacksize-and-heapsize/
 */
#ifdef LET_HEAP_GROW_TO_STACKPOINTER

extern unsigned long __bss_end__;
static char* heap_end        = (char*)&__bss_end__;
static char* heap_end_max    = 0;
unsigned long sbrk_num_fails = 0;

void* _sbrk(int incr) {
	//! get stack pointer
	void* sp;
	asm("mov %0, sp" : "=r"(sp) : :);
	//! return (void*)-1 if stackpointer gets below (stack grows downwards) the end of the heap (goes upwards)
	if ((char*)sp <= heap_end + incr) {
		// TODO: Write something to indicate the problem eventually
		++sbrk_num_fails;
		return (void*)-1;
	}

	char* prev = heap_end;
	heap_end += incr;

	if (heap_end > heap_end_max) {
		heap_end_max = heap_end;
	}

	return prev;
}

const char* getHeapEnd() {
	return heap_end;
}

const char* getHeapEndMax() {
	return heap_end_max;
}

unsigned long getSbrkNumFails() {
	return sbrk_num_fails;
}

#elif defined(LET_HEAP_BE_FIXED)

extern unsigned long _heap_start;
extern unsigned long _heap_end;

/**
 * This implementation uses a heap with a fixed size, it doesn't grow till the stack pointer. However, you will now
 * run into memory allocation problems probably.
 */

/*
void * _sbrk(int incr)
{
	static char *heap_base = (char *)&_heap_start;
	static char *heap_limit = (char *)&_heap_end;
	//! return (void*)-1 if heap goes beyond artificial set limit
	if (heap_base+incr >= heap_limit) {
		__asm("BKPT"); //! for now stop by force!
		return (void*)-1;
	}

	char *prev = heap_base;
	//! heap grows upwards
	heap_base += incr;
	return prev;
}
*/
#endif

//#endif
