// FrameMouse: right Steam Frame controller -> desk mouse.
#include "config.h"
#include "surface_tracker.h"
#include "uinput_mouse.h"
#include "vr_input.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <linux/input-event-codes.h>
#include <string>
#include <thread>
#include <unistd.h>

#ifndef FRAMEMOUSE_ACTIONS_DIR
#define FRAMEMOUSE_ACTIONS_DIR "actions"
#endif

namespace {

volatile std::sig_atomic_t g_stop = 0;
void on_signal(int) { g_stop = 1; }

std::string actions_dir()
{
    // Prefer actions/ next to the project (../actions relative to build/framemouse).
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        std::string exe(buf, static_cast<size_t>(n));
        std::string dir = exe.substr(0, exe.rfind('/'));
        std::string candidate = dir + "/../actions";
        if (access((candidate + "/actions.json").c_str(), R_OK) == 0) {
            char real[4096];
            if (realpath(candidate.c_str(), real)) return real;
        }
    }
    return FRAMEMOUSE_ACTIONS_DIR;
}

}  // namespace

int main(int argc, char** argv)
{
    Options opt;
    int code = 0;
    if (!load_options(argc, argv, opt, code))
        return code;

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);
    setvbuf(stdout, nullptr, _IOLBF, 0);

    VrInput vr;
    std::string dir = actions_dir();
    if (!vr.init(dir, opt.exclusive || opt.setup_only, opt.pose == "tip"))
        return 1;
    if (opt.setup_only) {
        std::printf("framemouse: SteamVR is set up. Restart SteamVR (or reboot) once for it to take effect.\n");
        return 0;
    }
    std::printf("framemouse: connected to SteamVR (actions: %s, exclusive: %s)\n", dir.c_str(),
                opt.exclusive ? "yes" : "no");

    UinputMouse mouse;
    if (!mouse.open())
        return 1;
    std::printf("framemouse: virtual mouse created. Rest the right controller on a surface and press A.\n");

    SurfaceTracker tracker(opt.tracker);
    bool left = false, right = false, was_down = false;
    const auto period = std::chrono::microseconds(1000000 / opt.rate_hz);
    const int debug_every = opt.rate_hz / 10 > 0 ? opt.rate_hz / 10 : 1;
    int frame = 0, dbg_dx = 0, dbg_dy = 0;
    int inactive_frames = 0;
    bool warned_inactive = false;
    auto next = std::chrono::steady_clock::now();

    while (!g_stop) {
        ControllerState s;
        if (!vr.poll(s)) {
            std::printf("framemouse: SteamVR is quitting\n");
            break;
        }

        inactive_frames = s.buttons_active ? 0 : inactive_frames + 1;
        if (!warned_inactive && inactive_frames > opt.rate_hz * 3) {
            std::printf("framemouse: WARNING: SteamVR is not sending button input to FrameMouse.\n"
                        "  If this is the first run, restart SteamVR once so the\n"
                        "  'globalActionSetPriority' developer setting takes effect.\n");
            warned_inactive = true;
        } else if (warned_inactive && s.buttons_active) {
            std::printf("framemouse: button input is now active\n");
            warned_inactive = false;
        }

        if (s.calibrate_pressed) {
            if (s.pose_valid) {
                tracker.calibrate(s.pos, s.hmd_valid ? s.hmd_yaw : 0.0);
                std::printf("framemouse: surface set at y=%.3f m\n", s.pos.y);
            } else {
                std::printf("framemouse: cannot set surface, controller not tracked\n");
            }
        }

        MouseDelta d = tracker.update(s.pos, s.pose_valid);
        bool dirty = false;
        if (d.dx || d.dy) {
            mouse.move(d.dx, d.dy);
            dirty = true;
        }
        if (s.left != left) {
            mouse.button(BTN_LEFT, s.left);
            left = s.left;
            dirty = true;
        }
        if (s.right != right) {
            mouse.button(BTN_RIGHT, s.right);
            right = s.right;
            dirty = true;
        }
        if (dirty)
            mouse.sync();

        if (opt.debug) {
            if (tracker.down() != was_down)
                std::printf("framemouse: %s\n", tracker.down() ? "DOWN" : "LIFTED");
            dbg_dx += d.dx;
            dbg_dy += d.dy;
            if (++frame % debug_every == 0) {
                std::printf("buttons=%d tracked=%d calibrated=%d height=%+7.1fmm %s dx=%+4d dy=%+4d L=%d R=%d\n",
                            s.buttons_active, s.pose_valid, tracker.calibrated(), tracker.height_above_surface_mm(),
                            tracker.down() ? "down  " : "lifted", dbg_dx, dbg_dy, left, right);
                dbg_dx = dbg_dy = 0;
            }
        }
        was_down = tracker.down();

        next += period;
        auto now = std::chrono::steady_clock::now();
        if (next < now)
            next = now;
        std::this_thread::sleep_until(next);
    }

    if (left) mouse.button(BTN_LEFT, false);
    if (right) mouse.button(BTN_RIGHT, false);
    mouse.sync();
    mouse.close();
    vr.shutdown();
    std::printf("framemouse: stopped\n");
    return 0;
}
