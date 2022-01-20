#!/bin/sh

# Compile the tool

CC='clang'
FLAGS='-O0 -fsanitize=address'
SOURCES='main.c'

[ -z "$(type $CC)" ] && CC='gcc'
[ -z "$(type $CC)" ] && CC='c99'

$CC $FLAGS -o build $SOURCES

# Install

[ "$(id -u)" = "0" ] && DEST='/usr/bin/build' || DEST="$HOME/.local/bin/build"

if [ -f "$DEST" ]; then
    echo "There is already something at $DEST"
    echo 'Press return to continue, or Ctrl+C to exit'
    read _
fi

cp -r ./build $DEST
echo "Installed to $DEST"
