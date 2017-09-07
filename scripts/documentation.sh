#!/bin/bash

option=${1:? "Usage: $0 (generate|memory_map|publish|view)"}

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
	cs_log "We assume you have checked out the repository in docs/html again, but then the gh-pages branch"
	cd ..
	if [ ! -d "docs/html" ]; then
		cs_err "The docs/html directory does not exist."
		exit
	fi
	msg=$(git log -1 --oneline | cat)

	cd docs/html
	if [ ! -d ".git" ]; then
		cs_err "The docs/html directory should be its own git repository. Clone bluenet in separate folder and mv to docs/html."
		cs_err "cd ~; git clone git@github.com:crownstone/bluenet; cd bluenet; git checkout gh-pages; cd ..; mv bluenet \$BLUENET_SOURCE_DIR/docs/html"
		exit
	fi
	branch=$(git rev-parse --abbrev-ref HEAD)
	echo $branch
	if [ "$branch" != "gh-pages" ]; then
		cs_err "The docs/html directory should be its own git repository. Clone bluenet in separate folder, mv to docs/html and checkout gh-pages."
		cs_err "cd ~; git clone git@github.com:crownstone/bluenet; cd bluenet; git checkout gh-pages; cd ..; mv bluenet \$BLUENET_SOURCE_DIR/docs/html"
		exit
	fi

	cs_info "Add all documentation changes to git"
	git add .

	cs_info "Commit changes to git, use as message $msg"
	git commit -m "$msg"

	cs_info "Pull existing changes from remote gh-pages branch"
	git pull

	cs_info "Push new changes"
	git push

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
