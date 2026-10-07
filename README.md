# FrameMouse 🖱️

<p align="center">
	<img src="FrameMouse.png" alt="FrameMouse" width="120">
</p>

**Turn your Steam Frame's right controller into a desk mouse.**

Put the controller flat on your desk and slide it around. The cursor moves. Pick it up to reposition it, just like a real mouse.

| On the controller | What it does |
|---|---|
| **A** | Sets the desk height. Press it while the controller is resting on the desk. |
| **Bumper** | Left click |
| **Trigger** | Right click |
| Lift it up | Stops tracking, so you can move it without moving the cursor |

Made for **Desktop Mode** on the Steam Frame.

> ⚠️ **Early, experimental release.** It hasn't had much real-world testing yet. Bug reports and feedback are very welcome in [Issues](../../issues).

---

## Install

Do this once, in **Desktop Mode**. Open a terminal (Konsole) and paste:

```sh
git clone https://github.com/goingafk/FrameMouse.git ~/FrameMouse
cd ~/FrameMouse
./scripts/setup.sh
```

`setup.sh` does three things:
- builds FrameMouse. Everything it needs is already on the Frame, and nothing is installed system-wide.
- fixes the Steam Frame bug where the Desktop Mode **mouse cursor is invisible**.
- turns on the SteamVR setting that lets FrameMouse receive button presses.

Then **reboot the headset once**.

## Use it

```sh
cd ~/FrameMouse
./scripts/start.sh
```

1. Put the right controller **flat on your desk**.
2. Press **A** to set the surface.
3. Slide it to move the cursor. Bumper is left click, trigger is right click.

"Up" on the screen is the direction you were facing when you pressed A. If you change seats or desks, press A again.

To turn it off:

```sh
./scripts/stop.sh
```

While FrameMouse is running, the bumper, trigger and A buttons on the right controller belong to it. That stops your clicks from also triggering the laser pointer.

## Tweak it

Want it faster, slower or smoother? Copy the example settings file and edit it:

```sh
mkdir -p ~/.config/framemouse
cp framemouse.conf.example ~/.config/framemouse/framemouse.conf
```

| Setting | Default | What it does |
|---|---|---|
| `sensitivity` | 4000 | Cursor speed. Higher means faster. |
| `lift_mm` | 15 | How high (in mm) you lift it before it stops tracking |
| `land_mm` | 8 | How close (in mm) to the desk counts as touching |
| `smoothing` | 0.5 | 0 is raw. Higher is smoother but slightly laggier. |
| `invert_x` / `invert_y` | false | Flip a direction |

Restart FrameMouse after changing settings (`./scripts/stop.sh`, then `./scripts/start.sh`).

## Troubleshooting

**The cursor moves but I can't see it.**
Run `./scripts/cursor-fix.sh`, then reboot. You can check it worked with `./scripts/cursor-fix.sh status`.

**Nothing happens when I press A or click.**
Make sure you ran `./scripts/setup.sh` and rebooted afterwards. If you did, the log will say so: run `./scripts/start.sh --foreground` and look for a warning about button input.

**The cursor jitters or drifts.**
Raise `smoothing` (try 0.7). Also make sure the controller can be seen by the headset's cameras.

**It keeps losing contact with the desk.**
Raise `lift_mm` a little (try 20). Press A again while the controller is resting flat.

**I want to see what it's doing.**
Run `./scripts/start.sh --foreground --debug`. It prints the controller's height above the desk and every movement it detects. Press Ctrl+C to quit.

**Undo everything.**
Run `./scripts/stop.sh` and `./scripts/cursor-fix.sh remove`, then delete the `~/FrameMouse` folder.

## How does it work?

The Frame's controllers are tracked by SteamVR. FrameMouse runs quietly in the background as a SteamVR add-on and reads where the right controller is and which buttons are pressed. It then shows up on the desktop as an ordinary mouse. There's no root, no system changes, and no Steam or SteamVR files modified.

Curious about the internals? See [docs/TECHNICAL.md](docs/TECHNICAL.md).

## Credits

- The invisible-cursor fix (`KWIN_FORCE_SW_CURSOR=1`) comes from [steam-frame-utils](https://github.com/curiousjtuber/steam-frame-utils), originally credited to ThrillSeeker.
- `openvr.h` is from [ValveSoftware/openvr](https://github.com/ValveSoftware/openvr) (BSD-3-Clause).

FrameMouse is a fan project and isn't affiliated with Valve.
