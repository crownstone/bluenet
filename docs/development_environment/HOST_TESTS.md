## CMake structure

The toplevel folder `host` contains a cmake subproject for running (unit) tests.
The build flag `COMPILE_FOR_HOST` determines whether or not to build the unit tests
when building bluenet.

There are currently 4 sets of source file definitions defined in `source/conf/cmake/`.

`crownstone-application.src.cmake`:
Contains `.ccp` files that are platform independent. These sources are allowed to
include headers and make use of other cpp files that depend on the platform, but
do not introduce additional dependencies.

`crownstone-platform.src.cmake`:
Contains `.cpp` files that depend on platform dependent tools such as NRF, implement
interrupt routines, manipulate hardware registers, etc.

`crownstone-platform-mock.src.cmake`
Contains `.cpp` files that replace the implementation of a file defined in the platform specific list (`crownstone-platform.src.cmake`).
The source files included here are allowed to depend on host system functionality (e.g. the full std library,
rather than the smaller subset used in the embedded firmware).

`crownstone.src.cmake`
To make a clean new start, `.cpp` files contained in this file are slowly moved into the 
`src.cmake` files above.

A sourcefile must be added to exactly one of these four files.

## Running tests
To manually run the tests for a particular target:

```
mkdir build && cd build
cmake .. -DBOARDTARGET=${YOUR_CONFIG}
make -j
cd ./host/${YOUR_CONFIG}
ctest
```

## Writing tests

The base folder where the test files reside is `host/test`. The filename of our tests start with the prefix `test_`.
Any `.cpp` file with a main method can be added as test. It is considered succesfull if the process result is positive,
and a failure otherwise.

For example, this would already be sufficient:
```
#include <util/cs_Error.h>
int main() { 
	bool isImplemented = false;
	assert(isImplemented, "not implemented");
	return 0;
}
```

Tests need to be added to `./host/CMakeLists.txt`.

**Note:** By default `ctest` swallows the printf and std::cout streams. If you are developing new tests, it might be nice to know `ctest -V`.

## Mocking platform dependent header files

All bluenet and tools header files are included, so you don't need to do anything special to include bluenet header files.
However, some bluenet headers are platform dependent. (Like cs_Logger.h). 

Mock a headerfile `./source/${FOLDER}/${SUBFOLDER}/${PLATORMDEPENDENT_SOURCE}.h` takes just one step:
- add a file with identical name and path in the mock folder: `./mock/source/${FOLDER}/${SUBFOLDER}/${PLATORMDEPENDENT_SOURCE}.cpp`.

The CMake file `/host/CMakeLists.txt` takes care of including the mock file you have just defined by ensuring that the mock header folders
have higher priority when resolving `#include` statements.

## Mocking platform dependent source files

Source files that are platform dependent must be included in `crownstone-platform.src.cmake`. 

To mock the file `./source/${FOLDER}/${SUBFOLDER}/${PLATORMDEPENDENT_SOURCE}.cpp`:
- add a file with identical name and path in the mock folder: `./mock/source/${FOLDER}/${SUBFOLDER}/${PLATORMDEPENDENT_SOURCE}.cpp`.
- include this in `crownstone-platform-mock.src.cmake`.

That's all, `/host/CMakeLists.txt` takes care of including the mock file you have just defined.


## Mocked NRF files

When CMake is configured with `COMPILE_FOR_HOST=1` , an external project is added to the build to provide a mocked NRF sdk.
This is pulled from https://github.com/mrquincle/nrf5_sdk and the project name is set to `NORDIC_HOST_SDK_TARGET`. 

