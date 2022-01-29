# Buildfile for the build tool

cc          clang
out         target/build
flags       -Wall -Wextra -O3
src         *.c !test/test.c

@install    sh ./target/install.sh
