#!/bin/bash

option=${1:? "Usage: $0 (generate|memory_map|publish)"}

# optional target, have default option
target=${1:-default}

# Obtain current path and utils
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
#source $path/_check_targets.sh $target
#source $path/_config.sh

generate() {
	cs_info "Run doxygen with doxygen.config as configuration file"
	cd ..
	doxygen doxygen.config
}

view() {
	cd ../docs/html
	xdg-open index.html
}

memory_map() {
	cs_info "Get .map file in BLUENET_BUILD_DIR"
	cd $BLUENET_BUILD_DIR

	if  [ -e "prog.map" ]; then
		cs_info "Adjust prog.map for visualization"
		cp prog.map prog.tmp.map
		ex -c '%g/\.text\S*[\s]*$/j' -c "wq" prog.map
		ex -c '%g/\.rodata\S*[\s]*$/j' -c "wq" prog.map
		ex -c '%g/\.data\S*[\s]*$/j' -c "wq" prog.map
		ex -c '%g/\.bss\S*[\s]*$/j' -c "wq" prog.map
		sed -i 's/\.text\S*/.text/g' prog.map
		sed -i 's/\.rodata\S*/.rodata/g' prog.map
		sed -i 's/\.data\S*/.data/g' prog.map
		sed -i 's/\.bss\S*/.bss/g' prog.map
	fi

	cd $path/../util/memory

	cs_log "Load now file prog.map from build directory in your browser"
	cs_log "Doesn't load automatically, load manually from $BLUENET_BUILD_DIR/prog.map"
	google-chrome --allow-file-access-from-files index.html
}

publish() {
	cd ..
	git add -u .
	git commit 
	git subtree push --prefix docs/html origin gh-pages
	cs_info "Go to https://crownstone.github.io/bluenet"
}

case $option in
	generate) 
		generate
	;;
	memory_map) 
		memory_map
	;;
	publish)
		publish
	;;
	view)
		view
	;;
	*)
		cs_err "Unknown option: \"$option\""
	;;
esac
