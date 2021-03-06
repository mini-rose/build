#!/bin/sh

# Compile the build tool, and then run the `install` target.

[ $(id -u) = 0 ] || {
    echo 'Must be ran as root'
    exit
}

[ "$(basename $PWD)" = "target" ] && echo 'Must be ran from the project root' \
    && exit 1

CC=$(type clang gcc c99 | grep -v 'not found' | head -n 1 | cut -f 1 -d " ")
SOURCES=$(find . -type f -name '*.c' | grep -v test)

$CC -o ./target/build -lpthread $SOURCES -O3
./target/build install
