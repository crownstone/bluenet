#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

cd $path/../build

cp prog.map prog.tmp.map
ex -c '%g/\.text\S*[\s]*$/j' -c "wq" prog.map
ex -c '%g/\.rodata\S*[\s]*$/j' -c "wq" prog.map
ex -c '%g/\.data\S*[\s]*$/j' -c "wq" prog.map
ex -c '%g/\.bss\S*[\s]*$/j' -c "wq" prog.map
sed -i 's/\.text\S*/.text/g' prog.map
sed -i 's/\.rodata\S*/.rodata/g' prog.map
sed -i 's/\.data\S*/.data/g' prog.map
sed -i 's/\.bss\S*/.bss/g' prog.map

cd $path/../util/memory

echo "Load now file prog.map from build directory in your browser"
gnome-open index.html

