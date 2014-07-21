#!/bin/make -f

all:
	@mkdir -p build
	@cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../arm.toolchain.cmake .. && make

clean: 
	@cd build && make clean

.PHONY: all
