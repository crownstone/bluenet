#######################################################################################################################
# The build system uses CMake. All the automatically generated code falls under the Lesser General Public License
# (LGPL GNU v3), the Apache License, or the MIT license, your choice.
#
# Authors:         Crownstone B.V. team
# Creation date:   Feb. 26, 2017
# License:         LGPLv3, MIT, Apache (triple-licensed)
#
# Copyright © 2017 Crownstone B.V. (http://crownstone.rocks/team)
#######################################################################################################################

#!/bin/make -f

#######################################################################################################################
# Check for the existence of required environmental variables 
#######################################################################################################################

# The build directory should be defined as environmental variable. We do not assume a default build directory such
# as ./build for example.
ifndef BLUENET_BUILD_DIR
$(error BLUENET_BUILD_DIR not defined!)
endif

ifndef BLUENET_BIN_DIR
$(error BLUENET_BIN_DIR not defined!)
endif

ifndef BLUENET_CONFIG_DIR
$(error BLUENET_CONFIG_DIR not defined!)
endif

ifndef BLUENET_WORKSPACE_DIR
	# do not throw error, but MESH_SUBDIR cannot be used: MESH_DIR with complete path is required
endif

#######################################################################################################################
# Optional configuration parameters that will be set to defaults if not set before
#######################################################################################################################

# The verbosity parameter is used in the cmake build files to e.g. display the definitions used.

ifndef VERBOSITY
VERBOSITY=1
endif

# TODO: Why not just use the -j flag instead of introducing a new one? This should not be hardcoded.

ifndef COMPILE_WITH_J_PROCESSORS
COMPILE_WITH_J_PROCESSORS=4
endif

#######################################################################################################################
# Additional configuration options
#######################################################################################################################

# We have a Makefile with some additional configuration options. Note that if the configurations options change, 
# cmake will force a rebuild of everything! That's why a COMPILATION_TIME macro, although useful, is not included 
# for the DEBUG_COMPILE_FLAGS. If we do a release we want to be sure we building the latest, hence then 
# COMPILATION_TIME as a macro is included.
#
# Also when the git hash changes this triggers a rebuild.

SOURCE_DIR=$(shell pwd)
COMPILATION_DAY=$(shell date --iso=date)
COMPILATION_TIME=$(shell date '+%H:%M')
GIT_BRANCH=$(shell git symbolic-ref --short -q HEAD)
GIT_HASH=$(shell git rev-parse --short=25 HEAD)

DEBUG_COMPILE_FLAGS=-DCOMPILATION_DAY="\"${COMPILATION_DAY}\"" \
			  -DCOMPILATION_TIME="\"${COMPILATION_TIME}\"" \			  
			  -DVERBOSITY="${VERBOSITY}" \
			  -DGIT_BRANCH="\"${GIT_BRANCH}\"" \
			  -DGIT_HASH="\"${GIT_HASH}\"" \
			  -DCMAKE_BUILD_TYPE=Debug

RELEASE_COMPILE_FLAGS=-DCOMPILATION_DAY="\"${COMPILATION_DAY}\"" \
			  -DCOMPILATION_TIME="\"${COMPILATION_TIME}\"" \
			  -DVERBOSITY="${VERBOSITY}" \
			  -DGIT_BRANCH="\"${GIT_BRANCH}\"" \
			  -DGIT_HASH="\"${GIT_HASH}\"" \
			  -DCMAKE_BUILD_TYPE=MinSizeRel

# We copy the cmake files we need to the bluenet folder. This:
#   + reduces the risk that someone overwrites the Makefile by running cmake in-source on accident;
#   + makes it fairly easy to swap out CMakeLists.txt for unit tests on the host (would otherwise clobber the system).
#
# The timestamp of the CMakeLists.txt or other files is NOT used by cmake to define a re-build 

define cross-compile-target-prepare
	@cp conf/cmake/CMakeLists.txt .
	@cp conf/cmake/arm.toolchain.cmake .
	@cp conf/cmake/CMakeBuild.config.default .
	@cp conf/cmake/CMakeConfig.cmake .
	#@if [ ! -f "CMakeLists.txt" ]; then ln -s conf/cmake/CMakeLists.txt .; fi
	#@if [ ! -f "arm.toolchain.cmake" ]; then ln -s conf/cmake/arm.toolchain.cmake .; fi
	#@if [ ! -f "CMakeBuild.config.default" ]; then ln -s conf/cmake/CMakeBuild.config.default .; fi
	#@if [ ! -f "CMakeConfig.cmake" ]; then ln -s conf/cmake/CMakeConfig.cmake .; fi
endef

define cross-compile-target-cleanup
	@rm -f CMakeLists.txt 
	@rm -f arm.toolchain.cmake 
	@rm -f CMakeBuild.config.default 
	@rm -f CMakeConfig.cmake 
	printf "++ Copy binaries to ${BLUENET_BIN_DIR}\n"
	@mkdir -p "${BLUENET_BIN_DIR}"
	@cp $(BLUENET_BUILD_DIR)/*.hex $(BLUENET_BUILD_DIR)/*.bin $(BLUENET_BUILD_DIR)/*.elf "$(BLUENET_BIN_DIR)"
endef

define host-compile-target-prepare
	@cp conf/cmake/CMakeLists.host_target.txt CMakeLists.txt
endef

define host-compile-target-cleanup
	@rm -f CMakeLists.txt
endef

#######################################################################################################################
# Targets
#######################################################################################################################

all: 
	@echo "Please call make with cross-compile-target or host-compile target"
	@echo "It is recommended to use the scripts/firmware.sh script"

release:
	$(call cross-compile-target-prepare)
	@cd $(BLUENET_BUILD_DIR) && cmake $(RELEASE_COMPILE_FLAGS) \
		-DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake $(SOURCE_DIR) && make -j${COMPILE_WITH_J_PROCESSORS}
	$(call cross-compile-target-cleanup)

cross-compile-target:
	$(call cross-compile-target-prepare)
	@mkdir -p $(BLUENET_BUILD_DIR)
	@cd $(BLUENET_BUILD_DIR) && cmake $(DEBUG_COMPILE_FLAGS) \
		$(SOURCE_DIR) -DCMAKE_TOOLCHAIN_FILE=$(SOURCE_DIR)/arm.toolchain.cmake && make -j${COMPILE_WITH_J_PROCESSORS}
	$(call cross-compile-target-cleanup)

host-compile-target:
	$(call host-compile-target-prepare)
	@mkdir -p $(BLUENET_BUILD_DIR)
	@cd $(BLUENET_BUILD_DIR) && cmake $(COMPILE_FLAGS) \
		$(SOURCE_DIR) && make -j${COMPILE_WITH_J_PROCESSORS}
	$(call host-compile-target-cleanup)

clean:
	$(call cross-compile-target-cleanup)
	$(call host-compile-target-cleanup)

list:
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | egrep -v -e '^[^[:alnum:]]' -e '^$@$$' | xargs

.PHONY: all cross-compile-target host-compile-target clean list release
