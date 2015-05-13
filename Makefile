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

BUILD_DIR=build
SOURCE_DIR=$(shell pwd)

all: build
	@cd $(BUILD_DIR) && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake -DCMAKE_BUILD_TYPE=Debug $(SOURCE_DIR) && make
	@if [ ! -z "${BLUENET_CONFIG_DIR}" ]; then echo "Copy binaries to ${BLUENET_CONFIG_DIR}/build"; mkdir -p ${BLUENET_CONFIG_DIR}/build; mkdir -p $(BUILD_DIR)/result; cp $(BUILD_DIR)/*.hex $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.elf $(BUILD_DIR)/result; cp $(BUILD_DIR)/result/* ${BLUENET_CONFIG_DIR}/build; fi

release: build
	@cd $(BUILD_DIR) && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake -DCMAKE_BUILD_TYPE=Release $(SOURCE_DIR) && make
	@if [ ! -z "${BLUENET_CONFIG_DIR}" ]; then echo "Copy binaries to ${BLUENET_CONFIG_DIR}/build"; mkdir -p ${BLUENET_CONFIG_DIR}/build; mkdir -p $(BUILD_DIR)/result; cp $(BUILD_DIR)/*.hex $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.elf $(BUILD_DIR)/result; cp $(BUILD_DIR)/result/* ${BLUENET_CONFIG_DIR}/build; fi

clean:
	@cd $(BUILD_DIR) && make clean

# The build target is only executed when there is no build directory! So if there is a build directory, but the test
# files cannot be found this will lead to an error on the first build. The next builds will be fine. So, the user only
# needs to build two times in a row.
build:
	@echo "++ Create build directory"
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/CMakeFiles/CMakeTmp
	@cp CMakeConfig.cmake $(BUILD_DIR)/CMakeFiles/CMakeTmp
	@if [ -e CMakeBuild.config ]; then cp -v CMakeBuild.config $(BUILD_DIR)/CMakeFiles/CMakeTmp; fi
	@if [ -e ${BLUENET_CONFIG_DIR}/CMakeBuild.config ]; then cp -v ${BLUENET_CONFIG_DIR}/CMakeBuild.config $(BUILD_DIR)/CMakeFiles/CMakeTmp; fi
	@mkdir -p $(BUILD_DIR)/CMakeFiles/CMakeTmp/conf
	@cp conf/nRF51822-softdevice.ld.in conf/nRF51822-softdevice.ld
	@sed -i "s/@APPLICATION_START_ADDRESS@/0x16000/" conf/nRF51822-softdevice.ld
	@sed -i "s/@APPLICATION_LENGTH@/2000/" conf/nRF51822-softdevice.ld
	@sed -i "s/@RAM_R1_BASE@/2000/" conf/nRF51822-softdevice.ld
	@sed -i "s/@RAM_APPLICATION_AMOUNT@/0x4000/" conf/nRF51822-softdevice.ld
	#@sed -i $(BUILD_DIR)/CMakeFiles/CMakeTmp/conf/nRF51822-softdevice.ld
	@cp conf/* $(BUILD_DIR)/CMakeFiles/CMakeTmp/conf
	@echo $(SOURCE_DIR) && cd $(BUILD_DIR) && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' --debug-trycompile -DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake --target analyze $(SOURCE_DIR)

.PHONY: all build
