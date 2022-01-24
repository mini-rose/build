#!/bin/sh

# Install the build tool

mkdir -p /usr/share/man/man1
mv -f ./build /usr/bin/build
cp -f ../build.1 /usr/share/man/man1/build.1

echo 'Installed to /usr/bin/build'
echo 'Installed manpage build(1)'
