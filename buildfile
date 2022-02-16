# Buildfile for the build tool

cc          clang
out         target/build
flags       -Wall -Wextra -O3
src         src/*.c !test/
libs        rose

@install    sh ./target/install.sh
