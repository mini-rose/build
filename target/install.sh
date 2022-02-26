#!/bin/sh
# Author: bellrise <bellrise@minirose.dev>, fenze <contact@fenze.dev>
# Install the build tool

[ $(basename $PWD) = target ] && {
		echo 'Must be ran from the project root'
    exit 1
}

[ $? = 0 ] || {
    echo 'build install must be run as root'
    exit 1
}

[ -f target/build ] && {
	mkdir -p /usr/share/man/man1
	cp -f ./target/build /usr/bin/build
	cp -f ./build.1 /usr/share/man/man1/build.1
	cp -f ./target/bashauto.sh /etc/bash_completion.d/build
	printf 'Installation complete. See 'man build' for more informations.'
} || {
    echo 'run a regular `build` first'
    exit 1
}
