#!/bin/sh

option=${1:? "Usage: $0 (generate|publish)"}

case $option in
	generate) 
		cd ..
		doxygen doxygen.config
	;;
	publish)
		cd ..
		git add -u .
		git commit 
		git subtree push --prefix docs/html origin gh-pages
		echo "Go to https://crownstone.github.com/bluenet"
	;;
	*)
		echo "Unknown option"
	;;
esac
