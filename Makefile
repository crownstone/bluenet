#!/bin/make -f

all:
	@mkdir -p build
	@cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../arm.toolchain.cmake .. && make

.PHONY: all
