#!/bin/sh

# Install the build tool

[ "$(basename $PWD)" = "target" ] && echo 'Must be ran from the project root' \
    && exit 1

[ ! -f ./target/build ] && {
    echo 'run a regular `build` first'
    exit 1
}

mkdir -p /usr/share/man/man1
cp -f ./target/build /usr/bin/build
[ $? = 0 ] || {
    echo 'build install must be run as root'
    exit 1
}
cp -f ./build.1 /usr/share/man/man1/build.1

echo 'Installed to /usr/bin/build'
echo 'Installed manpage build(1)'

[ "$(basename $SHELL)" = "bash" ] && {
    cp -f ./target/bashauto.sh /etc/bash_completion.d/build
    echo 'Installed bash autocompletions'
}
