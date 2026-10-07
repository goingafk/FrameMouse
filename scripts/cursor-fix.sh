#!/bin/sh
# Make the mouse cursor visible in Steam Frame Desktop Mode.
#
# KWin puts the cursor on a hardware plane, which the headset's desktop view doesn't
# show. KWIN_FORCE_SW_CURSOR=1 makes KWin draw the cursor into the frame instead.
# systemd reads ~/.config/environment.d at login, so it applies to the next Desktop
# Mode session (reboot if it doesn't show up after switching to Desktop Mode).
#
# Usage: cursor-fix.sh [install|remove|status]   (default: install)
. "$(dirname "$0")/_host.sh"
CONF="$HOME/.config/environment.d/90-kwin-software-cursor.conf"

case "${1:-install}" in
install)
    mkdir -p "$(dirname "$CONF")"
    echo "KWIN_FORCE_SW_CURSOR=1" >"$CONF"
    # Also set it in the running user manager, so a switch to Desktop Mode picks it up
    # without waiting for the next boot.
    systemctl --user set-environment KWIN_FORCE_SW_CURSOR=1 2>/dev/null || true
    echo "Cursor fix installed: $CONF"
    echo "Switch to Desktop Mode (or reboot) for it to take effect."
    ;;
remove)
    rm -f "$CONF"
    systemctl --user unset-environment KWIN_FORCE_SW_CURSOR 2>/dev/null || true
    echo "Cursor fix removed. Switch to Desktop Mode again (or reboot) to apply."
    ;;
status)
    if [ -f "$CONF" ]; then echo "installed: $CONF"; else echo "not installed"; fi
    ;;
*)
    echo "usage: $0 [install|remove|status]" >&2
    exit 2
    ;;
esac
