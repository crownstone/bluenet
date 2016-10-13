#!/bin/sh

source _utils.sh

./gen_doc.sh

cd ..

git add -u .

git commit

git subtree push --prefix docs/html origin gh-pages

log "Go to https://dobots.github.com/bluenet"
