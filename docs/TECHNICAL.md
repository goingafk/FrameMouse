# FrameMouse: technical notes

Use the right Steam Frame controller as a desk mouse.

- Rest the controller on a flat surface and press **A** to set the surface height.
- Slide it to move the cursor. "Forward" is the direction your headset faced when you pressed A.
- Lift it more than 15 mm to reposition it without moving the cursor, like a normal mouse.
- **Bumper** = left click, **Trigger** = right click.

While FrameMouse runs, it takes bumper, trigger and A away from the laser pointer and games, so clicks don't fire twice.

## How it works
The Frame controllers are not ordinary Linux input devices; SteamVR owns them. FrameMouse runs as a SteamVR overlay app and reads the right hand's pose and buttons through OpenVR. It then sends relative mouse events through a virtual `/dev/uinput` device called "FrameMouse", which KDE treats like any other mouse. Nothing is installed system-wide, and no root is needed.

## Usage
```sh
scripts/build.sh            # configure, build, run unit tests
scripts/start.sh            # start in the background (extra args go to framemouse)
scripts/start.sh --foreground --debug   # run attached and print tracking state
scripts/stop.sh             # stop
```
The scripts work from a normal terminal and from the VS Code Flatpak. Inside the Flatpak, they re-run themselves on the host with `flatpak-spawn --host`. The log is written to `$XDG_RUNTIME_DIR/framemouse/framemouse.log`.

### First run
`scripts/setup.sh` (and every FrameMouse start) turns on the SteamVR developer setting `steamvr/globalActionSetPriority` ("Experimental overlay input overrides"). Without it, SteamVR does not deliver button input to an overlay app. **Restart SteamVR once after the first run.** If button input isn't arriving, the log prints a warning.

### Invisible cursor in Desktop Mode
On the Frame, the Desktop Mode cursor moves but can't be seen. KWin draws it on a hardware plane, and the headset's desktop view doesn't show that plane. The fix is to make KWin draw the cursor in software:
```sh
scripts/cursor-fix.sh          # writes KWIN_FORCE_SW_CURSOR=1 to ~/.config/environment.d/
scripts/cursor-fix.sh status
scripts/cursor-fix.sh remove   # undo
```
It takes effect the next time Desktop Mode starts; reboot if the cursor still doesn't show. This is the same fix used by [steam-frame-utils](https://github.com/curiousjtuber/steam-frame-utils).

## Configuration
Copy `framemouse.conf.example` to `~/.config/framemouse/framemouse.conf`. Every key is also a command-line flag, for example `--sensitivity 6000`. Run `build/framemouse --help` to list them.

| key | default | meaning |
|---|---|---|
| `sensitivity` | 4000 | mouse counts per metre of travel |
| `land_mm` / `lift_mm` | 8 / 15 | height above the surface that counts as touching / lifted |
| `deadzone_mm` | 0.15 | ignore per-frame motion smaller than this (tracking jitter) |
| `smoothing` | 0.5 | 0 = raw, up to 0.95 = smooth |
| `invert_x` / `invert_y` | false | flip axes |
| `pose` | raw | `raw` or `tip`: which point on the controller is tracked |
| `exclusive` | true | hide bumper/trigger/A from other apps while running |
| `rate` | 250 | updates per second |

Button bindings can be changed in SteamVR's controller binding UI under the app "FrameMouse".

## Layout
- `src/surface_tracker.*` contains the surface, lift and motion logic. It is pure code and is unit-tested in `tests/`.
- `src/vr_input.*` is the OpenVR overlay app: actions and poses.
- `src/uinput_mouse.*` is the virtual mouse device.
- `actions/` holds the SteamVR action manifest, the Frame controller bindings and the app manifest.
- `third_party/openvr/openvr.h` is the header from [ValveSoftware/openvr](https://github.com/ValveSoftware/openvr) (BSD-3). It links against SteamVR's own `/opt/steamvr/bin/linuxarm64/libopenvr_api.so`.
