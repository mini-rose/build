# Buildfile for the build tool

cc          clang
out         build
flags       -Wall -Wextra -O2
src         *.c !test/test.c
