# Developer Guide

For syntax see the separate [style document](/docs/STYLE.md).

## Embedded development

Bluenet is firmware, it is **embedded**, it is (almost) bare-metal. There is very restricted memory, both **flash** (the "disk" space) as well as **ram** (the "working" memory). In contrast to "normal" development, there are two guidelines:

* Limit the use of dynamic memory allocation
* Limit the use of libraries.

### Dynamic memory allocation

To convince yourself about the pitfalls of dynamic memory allocation, see all kind of arguments online:

* https://stackoverflow.com/questions/37812732/use-of-malloc-in-embedded-c/37814174
* https://electronics.stackexchange.com/questions/171257/realloc-wasting-lots-of-space-in-my-mcu/171581

Now, this does not mean that it is completely forbidden. The use of it should be restricted. For example, it is okay to have at boot a series of structures allocated on the heap through memory allocation. This is a one-time and deterministic affair. This is quite different from memory allocation and **freeing** up memory again. A free is a code smell. The implementation of malloc we are using comes from the `nanolib` library (even more optimized than `newlib` in the `arm-none-eabi` toolchain). It requires us to implement our own `_sbrk()` function. This function is responsible for returning `-1` when the heap clashes with the stack. If that is the case `malloc` returns `NULL` and the application should handle that of course.

In practice, what this means that it is often the right solution to create for example a queue or a buffer of a maximum size. This allows someone to add entities (even of different sizes), but it also stops it from growing indefinitely.

Preferably, the entire code base has a single **maximum** value for the heap size.

## Object pool buffers

To accommodate flexibility while keeping dynamic memory usage to a minimum, components are allowed to contain a buffer that can be used for dynamic data. Since the components in the firmware are known at compiletime, as well as their buffer size, it remains possible to assert that the device will not run into heap allocation problems.

Guidelines:
- Components must contain such buffers as a member to avoid double initialisation and avoid `nullptr` checks.
- Arrays, vectors, etc. that need to grow and shrink in size can use these buffers to do so. Effectively, all components have their own isolated miniheap.


## Libraries

Normally you have the C++ standard libraries to work with `std::vector`, `std::map`, etc. Very, very, carefully consider if you actually need this. A lot of functionality has been disabled by using compiler flags such as `-no-exceptions` (remove exception handling), `-no-unwind-tables`(also remove those), etc.

The nanolib standard libs can contain code that is not required on an embedded system. For example, for a multi-threading context (you can recognize reentrant functions by a `_r` suffix).

    arm-none-eabi-nm crownstone.elf | grep '_r\b'

Side note: this does not necessarily mean that the code is thread-safe, see all the online bug reports. Anyway, if using standard libraries, check the compiled code size. You can find size information in the `crownstone.map` file and this can be visualized by scripts in the script directory.

Another important reason to use POD (plain old data) types is that they tend to be close to how data will be stored in flash or how it will be communicated over the wire. Moreover, it can even be compressed further by **packing** it.
