#!/bin/sh

cmd_test=doxygen
cmd_package=doxygen
command -v "$cmd_test" >/dev/null 2>&1 || { echo >&2 "Alas, $cmd_test required, but not available. Please, install $cmd_package and run this script again."; exit 1; }

cmd_test=dot
cmd_package=graphviz
command -v "$cmd_test" >/dev/null 2>&1 || { echo >&2 "Alas, $cmd_test required, but not available. Please, install $cmd_package and run this script again."; exit 1; }

cd ..

doxygen doxygen.config
