#!/bin/sh
# Configure, build and test FrameMouse.
set -e
. "$(dirname "$0")/_host.sh"
cmake -S "$FRAMEMOUSE_ROOT" -B "$FRAMEMOUSE_ROOT/build"
cmake --build "$FRAMEMOUSE_ROOT/build" -j"$(nproc)"
ctest --test-dir "$FRAMEMOUSE_ROOT/build" --output-on-failure
