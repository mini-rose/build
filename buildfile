# Buildfile for the build tool

cc          clang
out         target/build
flags       -Wall -Wextra -O3
src         *.c !test/test.c

@test       cd test; sh ./run_tests.sh
@install    sh ./target/install.sh
