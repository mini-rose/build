#!/bin/sh

# Compilator options

CC=$(type clang cpp c99 | grep -m 1 -v 'not found' | cut -f 1 -d " ")
FLAGS='-O0 -fsanitize=address'
SOURCES='main.c'
TARGET='build'

$CC $FLAGS -o $TARGET $SOURCES

# Installation

[ $(id -u) = 0 ] && DEST="$HOME/.local/bin/$TARGET" || DEST="/usr/bin/$TARGET"

[ -f $DEST ] && {
    echo 'There is already something at' $DEST
    echo 'Press return to continue, or Ctrl+C to exit'
    read _
} || {
    cp -r ./build $DEST \
        && echo "Installed to $DEST"
}
