#!/bin/sh
# One-time setup: build FrameMouse, fix the invisible Desktop Mode cursor, and turn on
# the SteamVR setting that lets FrameMouse receive button presses.
set -e
. "$(dirname "$0")/_host.sh"
"$FRAMEMOUSE_ROOT/scripts/build.sh"
"$FRAMEMOUSE_ROOT/scripts/cursor-fix.sh" install
if ! "$FRAMEMOUSE_ROOT/build/framemouse" --setup-steamvr; then
    echo "Couldn't reach SteamVR. Make sure the headset is on, then run this again." >&2
    exit 1
fi
echo
echo "All set! Reboot the headset once, then run ./scripts/start.sh"
