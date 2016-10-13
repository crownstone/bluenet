#/bin/sh

# colour assignments, see -> http://linuxtidbits.wordpress.com/2008/08/13/output-color-on-bash-scripts-advanced/
# ORANGE="\033[38;5;214m"
# GREEN="\033[38;5;46m"
# RED="\033[38;5;196m"
# COLOR_RESET="\033[39m"

red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
normal=$(tput sgr0)

err() {
	printf "$red$1$normal\n"
}

info() {
	printf "$yellow$1$normal\n"
}

succ() {
	printf "$green$1$normal\n"
}

log() {
	printf "$normal$1$normal\n"
}

checkError() {
	result=$?
	if [ $result -ne 0 ]; then
		if [ -n "$1" ]; then
			err "$1"
		fi
		exit $result
	fi
}
