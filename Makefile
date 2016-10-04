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

# BUILD_DIR=build
SOURCE_DIR=$(shell pwd)
COMPILE_WITH_J_PROCESSORS=4

all: build
	@cd $(BLUENET_BUILD_DIR) && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DGIT_BRANCH='"$(shell git symbolic-ref --short -q HEAD)"' -DGIT_HASH='"$(shell git rev-parse --short=25 HEAD)"' -DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake -DCMAKE_BUILD_TYPE=Debug $(SOURCE_DIR) && make -j${COMPILE_WITH_J_PROCESSORS}
	@if [ ! -z "${BLUENET_BUILD_DIR}" ]; then echo "Copy binaries to ${BLUENET_BIN_DIR}"; mkdir -p ${BLUENET_BIN_DIR}; cp $(BLUENET_BUILD_DIR)/*.hex $(BLUENET_BUILD_DIR)/*.bin $(BLUENET_BUILD_DIR)/*.elf $(BLUENET_BIN_DIR); fi

release: build
	@cd $(BUILD_DIR) && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake -DCMAKE_BUILD_TYPE=MinSizeRel $(SOURCE_DIR) && make -j${COMPILE_WITH_J_PROCESSORS}
	@if [ ! -z "${BLUENET_BUILD_DIR}" ]; then echo "Copy binaries to ${BLUENET_RELEASE_DIR}"; mkdir -p ${BLUENET_RELEASE_DIR}; cp $(BLUENET_BUILD_DIR)/*.hex $(BLUENET_BUILD_DIR)/*.bin $(BLUENET_BUILD_DIR)/*.elf $(BLUENET_RELEASE_DIR); fi

clean:
	@cd $(BUILD_DIR) && make clean

# The build target is only executed when there is no build directory! So if there is a build directory, but the test
# files cannot be found this will lead to an error on the first build. The next builds will be fine. So, the user only
# needs to build two times in a row.
build:
	@echo "++ Create build directory: $(BLUENET_BUILD_DIR)"
	@mkdir -p $(BLUENET_BUILD_DIR)
	@mkdir -p $(BLUENET_BUILD_DIR)/CMakeFiles/CMakeTmp
	@cp CMakeConfig.cmake $(BLUENET_BUILD_DIR)/CMakeFiles/CMakeTmp
	@if [ -e CMakeBuild.config ]; then cp -v CMakeBuild.config $(BLUENET_BUILD_DIR)/CMakeFiles/CMakeTmp; fi
	@if [ -e ${BLUENET_CONFIG_DIR}/CMakeBuild.config ]; then cp -v ${BLUENET_CONFIG_DIR}/CMakeBuild.config $(BLUENET_BUILD_DIR)/CMakeFiles/CMakeTmp; fi
	@mkdir -p $(BLUENET_BUILD_DIR)/CMakeFiles/CMakeTmp/conf
	@cp conf/nRF51822-softdevice.ld.in conf/nRF51822-softdevice.ld
	@sed -i "s/@APPLICATION_START_ADDRESS@/0x16000/" conf/nRF51822-softdevice.ld
	@sed -i "s/@APPLICATION_LENGTH@/2000/" conf/nRF51822-softdevice.ld
	@sed -i "s/@RAM_R1_BASE@/2000/" conf/nRF51822-softdevice.ld
	@sed -i "s/@RAM_APPLICATION_AMOUNT@/0x4000/" conf/nRF51822-softdevice.ld
	@sed -i "s/@HEAP_SIZE@/2700/" conf/nRF51822-softdevice.ld
	#@sed -i $(BLUENET_BUILD_DIR)/CMakeFiles/CMakeTmp/conf/nRF51822-softdevice.ld
	@cp conf/* $(BLUENET_BUILD_DIR)/CMakeFiles/CMakeTmp/conf
	@echo $(SOURCE_DIR) && cd $(BLUENET_BUILD_DIR) && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DGIT_BRANCH='"$(shell git symbolic-ref --short -q HEAD)"' -DGIT_HASH='"$(shell git rev-parse --short=25 HEAD)"' --debug-trycompile -DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake --target analyze $(SOURCE_DIR)

.PHONY: all build
