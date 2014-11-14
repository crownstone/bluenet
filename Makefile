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

all:
	@mkdir -p build
	@cd build && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DCMAKE_TOOLCHAIN_FILE=../arm.toolchain.cmake .. && make

clean: 
	@cd build && make clean

.PHONY: all
