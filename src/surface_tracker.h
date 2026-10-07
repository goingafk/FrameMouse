// Pure surface-mouse logic: no OpenVR or uinput dependencies, so it is unit-testable.
#pragma once

struct TrackerConfig {
    double counts_per_meter = 4000.0;  // mouse counts per metre of controller travel
    double land_mm = 8.0;              // below surface+land => controller is "down"
    double lift_mm = 15.0;             // above surface+lift => controller is "lifted"
    double deadzone_mm = 0.15;         // per-frame motion below this is treated as jitter
    double smoothing = 0.5;            // low-pass factor 0 (none) .. <1 (heavy)
    bool invert_x = false;
    bool invert_y = false;
};

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

struct MouseDelta {
    int dx = 0;
    int dy = 0;
};

class SurfaceTracker {
public:
    explicit SurfaceTracker(const TrackerConfig& cfg = {}) : cfg_(cfg) {}

    // Record the surface height and the "forward" direction (yaw, radians, around +Y;
    // forward is -Z rotated by yaw, the OpenVR convention).
    void calibrate(const Vec3& pos, double yaw);

    // Feed one tracking sample. Returns whole mouse counts to emit this frame.
    MouseDelta update(const Vec3& pos, bool pose_valid);

    bool calibrated() const { return calibrated_; }
    bool down() const { return down_; }
    double height_above_surface_mm() const { return height_mm_; }

private:
    TrackerConfig cfg_;
    bool calibrated_ = false;
    double surface_y_ = 0;
    double yaw_ = 0;

    bool down_ = false;
    bool have_last_ = false;
    Vec3 last_{};
    double height_mm_ = 0;

    // filtered motion (metres in calibrated frame) and sub-count remainders
    double filt_r_ = 0, filt_f_ = 0;
    double rem_x_ = 0, rem_y_ = 0;

    void reset_motion();
};

// Extract yaw (rotation about +Y) from a 3x4 row-major OpenVR pose matrix.
double yaw_from_matrix(const float m[3][4]);
