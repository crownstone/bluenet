#!/bin/sh

cd ..

echo "Kill cldoc if it is running or not"
killall cldoc
echo "Open browser at localhost:8888"
cldoc serve docs/html --address localhost:8888 &
