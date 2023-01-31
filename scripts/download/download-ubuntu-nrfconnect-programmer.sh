#!/bin/bash

usage="$0 <workspace path>"

WORKSPACE_DIR=${1:? "Usage: $usage"}

REPOS=pc-nrfconnect-programmer
GIT_URL=https://github.com/NordicSemiconductor/$REPOS

TOOLS_DIR="$WORKSPACE_DIR/tools"
mkdir -p "$TOOLS_DIR"
cd "$TOOLS_DIR"

SKIP_CLONE=0
if [ -e "$REPOS" ]; then
	SKIP_CLONE=1
fi

if [ $SKIP_CLONE -eq 1 ]; then
	echo "Update repository"
	cd $REPOS && git pull
else
	echo "Clone repository"
	git clone $GIT_URL
	cd $REPOS
fi

npm install
