#!/bin/sh
# Start FrameMouse in the background. Extra arguments are passed to framemouse
# (e.g. ./start.sh --sensitivity 6000). Use --foreground to run attached.
set -e
. "$(dirname "$0")/_host.sh"
BIN="$FRAMEMOUSE_ROOT/build/framemouse"
[ -x "$BIN" ] || "$FRAMEMOUSE_ROOT/scripts/build.sh"

if [ "${1:-}" = "--foreground" ]; then
    shift
    exec "$BIN" "$@"
fi

mkdir -p "$FRAMEMOUSE_RUN"
if [ -f "$FRAMEMOUSE_PID" ] && kill -0 "$(cat "$FRAMEMOUSE_PID")" 2>/dev/null; then
    echo "FrameMouse is already running (pid $(cat "$FRAMEMOUSE_PID"))."
    exit 0
fi

nohup "$BIN" "$@" >"$FRAMEMOUSE_LOG" 2>&1 &
echo $! >"$FRAMEMOUSE_PID"
sleep 1
if kill -0 "$(cat "$FRAMEMOUSE_PID")" 2>/dev/null; then
    echo "FrameMouse started (pid $(cat "$FRAMEMOUSE_PID")). Log: $FRAMEMOUSE_LOG"
    echo "Rest the right controller on your desk and press A to set the surface."
    if [ ! -f "$HOME/.config/environment.d/90-kwin-software-cursor.conf" ]; then
        echo "Tip: the Desktop Mode cursor is invisible by default on the Frame;"
        echo "     run scripts/setup.sh (or scripts/cursor-fix.sh) once to make it show."
    fi
else
    echo "FrameMouse failed to start:" >&2
    cat "$FRAMEMOUSE_LOG" >&2
    rm -f "$FRAMEMOUSE_PID"
    exit 1
fi
