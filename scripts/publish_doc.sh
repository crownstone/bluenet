#!/bin/sh

./gen_doc.sh

cd ..

git add -u .

git commit

git subtree push --prefix docs/html upstream gh-pages

echo "Go to https://dobots.github.com/bluenet"
