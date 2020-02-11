#!/bin/sh

path=${1:? "Usage: $0 path"}

find $path -regex '.*\.\(c\|h\|cpp\|hpp\)' -print0 | xargs -0 grep -P "^(\s+ +)|( +    +\s*)" 

