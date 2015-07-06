#!/bin/sh

echo "Grep for license term"
cd ..
grep -R -L "icense"  | grep -v build | grep -v [.]git | grep -v docs
