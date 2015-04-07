#!/bin/sh

cd ..

killall cldoc
cldoc serve docs/html --address localhost:8888 &
