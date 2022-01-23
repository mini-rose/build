#!/bin/sh

# Compiler options

CC=$(type clang gcc c99 | grep -v 'not found' | head -n 1 | cut -f 1 -d " ")
SOURCES=$(find . -type f -name '*.c' | grep -v test)

FLAGS='-Wall -Wextra'
[ "$DEBUG" = "1" ] \
    && FLAGS="$FLAGS -O0 -fsanitize=address" \
    || FLAGS="$FLAGS -O2"

TARGET='build'
MANPAGE='build.1'

$CC $FLAGS -o $TARGET $SOURCES

# Ensure the existance of both directories

cp -r $TARGET "/usr/bin/$TARGET" \
    && echo "Installed to /usr/bin/$TARGET"
cp -r $MANPAGE "/usr/share/man/man1/$MANPAGE" \
    && echo "Manpage build(1) installed"
