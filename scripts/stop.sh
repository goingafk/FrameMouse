#!/bin/sh
# Stop a background FrameMouse started with start.sh.
. "$(dirname "$0")/_host.sh"
if [ ! -f "$FRAMEMOUSE_PID" ]; then
    echo "FrameMouse is not running."
    exit 0
fi
PID="$(cat "$FRAMEMOUSE_PID")"
if kill "$PID" 2>/dev/null; then
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        kill -0 "$PID" 2>/dev/null || break
        sleep 0.2
    done
    kill -0 "$PID" 2>/dev/null && kill -9 "$PID"
    echo "FrameMouse stopped."
else
    echo "FrameMouse was not running."
fi
rm -f "$FRAMEMOUSE_PID"
