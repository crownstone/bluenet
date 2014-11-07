#!/bin/make -f

all:
	@mkdir -p build
	@cd build && cmake -DCOMPILATION_TIME='"$(shell date --iso=date)"' -DCMAKE_TOOLCHAIN_FILE=../arm.toolchain.cmake .. && make

clean: 
	@cd build && make clean

.PHONY: all
