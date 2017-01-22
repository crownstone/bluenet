#!/bin/sh

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

$path/gen_doc.sh

cd ..

git add -u .

git commit

git subtree push --prefix docs/html origin gh-pages

log "Go to https://crownstone.github.com/bluenet"
