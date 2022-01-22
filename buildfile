# Buildfile for the build tool

cc          clang
out         build
flags       -Wall -Wextra -O2

@start      setup.sh
@finish     install.sh
