#######################################################################################################################
# The build systems uses CMake. All the automatically generated code falls under the Lesser General Public License 
# (LGPL GNU v3), the Apache License, or the MIT license, your choice.
#
# Author:	 Anne C. van Rossum (Distributed Organisms B.V.)
# Date: 	 Oct 28, 2013
#
# Copyright © 2013 Anne C. van Rossum <anne@dobots.nl>
#######################################################################################################################

#!/bin/make -f

all: build
	@cd build && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DCMAKE_TOOLCHAIN_FILE=../arm.toolchain.cmake -DCMAKE_BUILD_TYPE=Release .. && make
	@if [ ! -z "${BLUENET_CONFIG_DIR}" ]; then echo "Copy binaries to ${BLUENET_CONFIG_DIR}/build"; mkdir -p ${BLUENET_CONFIG_DIR}/build; mkdir -p build/result; cp build/*.hex build/*.bin build/*.elf build/result; cp build/result/* ${BLUENET_CONFIG_DIR}/build; fi

clean: 
	@cd build && make clean

build:
	@echo "++ Create build directory"
	@mkdir -p build
	@mkdir -p build/CMakeFiles/CMakeTmp
	@cp CMakeConfig.cmake build/CMakeFiles/CMakeTmp
	@if [ -e CMakeBuild.config ]; then cp -v CMakeBuild.config build/CMakeFiles/CMakeTmp; fi
	@if [ -e ${BLUENET_CONFIG_DIR}/CMakeBuild.config ]; then cp -v ${BLUENET_CONFIG_DIR}/CMakeBuild.config build/CMakeFiles/CMakeTmp; fi
	@mkdir -p build/CMakeFiles/CMakeTmp/conf
	@cp conf/nRF51822-softdevice.ld.in conf/nRF51822-softdevice.ld
	@sed -i "s/@APPLICATION_START_ADDRESS@/0x16000/" conf/nRF51822-softdevice.ld 
	@sed -i "s/@APPLICATION_LENGTH@/2000/" conf/nRF51822-softdevice.ld 
	@sed -i "s/@RAM_R1_BASE@/2000/" conf/nRF51822-softdevice.ld 
	@sed -i "s/@RAM_APPLICATION_AMOUNT@/0x4000/" conf/nRF51822-softdevice.ld 
	#@sed -i build/CMakeFiles/CMakeTmp/conf/nRF51822-softdevice.ld 
	@cp conf/* build/CMakeFiles/CMakeTmp/conf
	@cd build && cmake --debug-trycompile -DCMAKE_TOOLCHAIN_FILE=../arm.toolchain.cmake --target analyze  ..

.PHONY: all
