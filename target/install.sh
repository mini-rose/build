#!/bin/sh
# Author: bellrise <contact@bellrise.net>, fenze <contact@fenze.dev>
# Install the build tool

[ $(basename $PWD) = target ] && {
    echo 'Must be ran from the project root'
    exit 1
}

[ $(id -u) = 0 ] || {
    echo 'build install must be run as root'
    exit 1
}

[ -f target/build ] && {
    mkdir -p /usr/share/man/man1
    cp -f ./target/build /usr/bin/build
    cp -f ./build.1 /usr/share/man/man1/build.1

	# Install autocomplete scripts too.
	case "$(basename $SHELL)" in
		zsh)
			cp -f ./target/zshcomplete.zsh /usr/share/zsh/functions/Completion/Unix/_build;;
		bash|dash)
			cp -f ./target/bashcomplete.sh /etc/bash_completion.d/build;;
	esac

    echo 'Installation complete. See `man build` for more information.'
} || {
    echo 'run a regular `build` first'
    exit 1
}
