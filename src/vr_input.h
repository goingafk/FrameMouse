// SteamVR connection: registers FrameMouse as an overlay app and reads its actions.
#pragma once

#include "surface_tracker.h"

#include <cstdint>
#include <string>

struct ControllerState {
    bool pose_valid = false;
    Vec3 pos;
    bool left = false;          // bumper
    bool right = false;         // trigger
    bool calibrate_pressed = false;  // A, rising edge this frame
    bool buttons_active = false;     // SteamVR is delivering our button actions
    bool hmd_valid = false;
    double hmd_yaw = 0;
};

class VrInput {
public:
    ~VrInput();

    // actions_dir holds actions.json and framemouse.vrmanifest.
    bool init(const std::string& actions_dir, bool exclusive, bool use_tip_pose);
    void shutdown();

    // Pump events and read the current state. Returns false when SteamVR is quitting.
    bool poll(ControllerState& out);

private:
    bool initialized_ = false;
    bool exclusive_ = true;
    uint64_t set_ = 0, left_ = 0, right_ = 0, calib_ = 0, pose_ = 0;
};
