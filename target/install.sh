#!/bin/sh

# Install the build tool

[ "$(basename $PWD)" = "target" ] && echo 'Must be ran from the project root' \
    && exit 1

mkdir -p /usr/share/man/man1
mv -f ./target/build /usr/bin/build
[ $? = 0 ] || {
    echo 'build install must be run as root'
    exit 1
}
cp -f ./build.1 /usr/share/man/man1/build.1

echo 'Installed to /usr/bin/build'
echo 'Installed manpage build(1)'
