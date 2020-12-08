# Protocol

The protocol has to be designed yet. Currently, there are a few functions implemented with a hint of a protocol. At 
the microapp code side this will be completely hidden for the user, hence this protocol definition has to be seen as
a definition for someone who wants to understand how the microapp code maps to bluenet functionality. However, for
certain functions there are user-facing results.

## User-facing definitions

An implementation of a microapp can be found [here](https://github.com/mrquincle/crownstone-bluenet-module). It has 
a so-called `.ino` file with Arduino-like syntax:

```
static int counter = 0;

void setup() {
	Serial.begin();
	Serial.write("Hello world!");
	Serial.write(counter);
}

int loop() {
	counter++;

	// every 5 seconds
	if (counter % 5 == 0) {
		digitalWrite(1, 1);
		delay(1000);
		digitalWrite(1, 0);
	}
}

```

Here the relay is mapped to pin `1` and the value written with `digitalWrite` turns it on or off. The delay function
can be used to introduce a non-blocking delay. It is perfectly fine to write `delay(1000000)`, delay of 1000 seconds.
The microapp code will then resume 1000 seconds later.

## Microapp protocol

The protocol itself is implemented on both sides, at the bluenet side in the `microapp_callback` function and at the 
microapp side in for example the `main.c` code. Here we are only concerned in the bluenet side.

The callback contains an opcode:

```
enum CommandMicroapp {
	CS_MICROAPP_COMMAND_LOG          = 0x01,
	CS_MICROAPP_COMMAND_DELAY        = 0x02,
	CS_MICROAPP_COMMAND_PIN          = 0x03,
};
```

For each command there are several options again.

### Log

The logging function accepts a type: char, int, string and then the payload: `[COMMAND TYPE PAYLOAD]`.

### Delay

The delay function accepts a delay in milliseconds: `[COMMAND DELAY COARGS_PTR]`. A delay less than one second will be 
ignored. This is a function that requires a pointer to the coroutine argument which is used in `yield`.

### Pin

The pin command sets the relay: `[COMMAND PIN_NUMBER VALUE]`. The pin numbers are virtual pin numbers. Currently the 
relay is set to pin number 1.

# Implementation

## Calling the loop function

The callLoop function in the microapp code will be called with `cntr = 0` the first time. At this moment it sets up
a coroutine. The function in `cs_MicroApp.cpp` is more complex, but if we would call a function locally - within the
same file - it would look like the following:

```
void MicroApp::callLoop(int & cntr, int & skip) {
	if (cntr == 0) {
		_coargs = {&_coroutine, 1, 0};
		start(&_coroutine, &loop_local, &_coargs);
	}
	...
```

You see that a function `loop_local` will be called (later on) with as arguments `_coargs`. This function will be 
called using a different stack from the main program! You can check this by logging the stack pointer as shown in 
the following possible implementation of a `loop_local` function.

```
#define get_sp(p) asm volatile("mov %0, sp" : "=r"(p) : : )

void loop_local(void *p) {
	LOGi("Loop locally..");
	coargs* args = (coargs*) p;
	void* sp;
	get_sp(sp);
	LOGi("Stack pointer at 0x%x", sp);
	for (int i = 0; i < 2; ++i) {
		args->cntr++;
		args->delay = (i+1)*10;
		yield(args->c);
	}
}
```

The `loop_local` function either just returns or "yields" with `yield` and the coroutine struct as argument. When it
yields it will return to the main bluenet program. Due to the fact that we use a separate stack we do not need to 
worry about stack corruption. We just jump back exactly where we were (implementation detail: in `start` or `next`).

## The loop function

The actual loop function jumps to microapp code and back through function pointers stored by the microapp and the 
bluenet code in `IPC_RAM_DATA`. In the end it ends up in `microapp_callback`. Here we can `yield` as long as we have
preserved the pointer to the `coargs` struct. In the microapp code we store this pointer on each call and put it in
the payload if we callback with the delay opcode.
