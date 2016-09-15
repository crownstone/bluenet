# Todo list

A lot of items that were or on our TODO list have already been done. For example, the code has been refactored to make reuse of the same buffer in user-space as much as possible. This to reduce the RAM footprint.

* Create nice-looking and convenient [DoBeacon app](https://github.com/dobots/dobeacon-app).

## Memory use

Due to the fact that the SoftDevice S130 uses 10kB out of 16kB, we have to be really careful with the 6kB we have left. In the `util/memory` directory you can see a tool from Eliot Stock to visualize the footprint of functions. We removed for example C++ exception handling.

<p align="center">
<img src="docs/text.png?raw=true" alt="Memory .text section" height="500px"/>
</p>

<p align="center">
<img src="docs/rodata.png?raw=true" alt="Memory .rodata section" height="500px"/>
</p>

<p align="center">
<img src="docs/bss.png?raw=true" alt="Memory .bss section" height="500px"/>
</p>

Tips to reduce memory usage are really welcome!
