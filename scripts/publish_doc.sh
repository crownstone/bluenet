#!/bin/sh

cd ..

git add -u .

git commit

git subtree push --prefix docs/html origin gh-pages

echo "Go to https://crownstone.github.com/bluenet"
