#!/bin/sh

# Compilator options

CC=$(type clang gcc c99 | grep -v 'not found' | head -n 1 | cut -f 1 -d " ")
FLAGS='-O2'
SOURCES='main.c'
TARGET='build'

$CC $FLAGS -o $TARGET $SOURCES

# Installation

[ $(id -u) = 0 ] && DEST="/usr/bin/$TARGET" || DEST="$HOME/.local/bin/$TARGET"

[ -f $DEST ] && {
    echo 'There is already something at' $DEST
    echo 'Press return to continue, or Ctrl+C to exit'
    read _
}

cp -r ./build $DEST && echo "Installed to $DEST"
