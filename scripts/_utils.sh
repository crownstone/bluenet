#/bin/sh

# colour assignments, see -> http://linuxtidbits.wordpress.com/2008/08/13/output-color-on-bash-scripts-advanced/
ORANGE="\033[38;5;214m"
GREEN="\033[38;5;46m"
RED="\033[38;5;196m"
COLOR_RESET="\033[39m"

err() {
	echo -e $RED$1$COLOR_RESET
}

info() {
	echo -e $ORANGE$1$COLOR_RESET
}

succ() {
	echo -e $GREEN$1$COLOR_RESET
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
