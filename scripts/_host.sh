# Sourced by the other scripts. When running inside a Flatpak (e.g. the VS Code
# Flatpak), re-run the calling script on the host, where SteamVR and uinput live.
if [ -f /.flatpak-info ] && [ -z "${FRAMEMOUSE_ON_HOST:-}" ]; then
    exec flatpak-spawn --host --env=FRAMEMOUSE_ON_HOST=1 "$0" "$@"
fi
FRAMEMOUSE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FRAMEMOUSE_RUN="${XDG_RUNTIME_DIR:-/tmp}/framemouse"
FRAMEMOUSE_PID="$FRAMEMOUSE_RUN/framemouse.pid"
FRAMEMOUSE_LOG="$FRAMEMOUSE_RUN/framemouse.log"
