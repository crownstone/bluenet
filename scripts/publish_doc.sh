#!/bin/sh

cd ..

git subtree push --prefix docs/html upstream gh-pages

echo "Go to https://dobots.github.com/bluenet"
