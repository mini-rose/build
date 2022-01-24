# Buildfile for the build tool

cc          clang
out         target/build
flags       -Wall -Wextra -O2
src         *.c !test/test.c

@test       cd test; sh ./run_tests.sh
@install    cd target; sh ./install.sh
