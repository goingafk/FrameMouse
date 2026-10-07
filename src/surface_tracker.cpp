#include "surface_tracker.h"

#include <cmath>

void SurfaceTracker::calibrate(const Vec3& pos, double yaw)
{
    calibrated_ = true;
    surface_y_ = pos.y;
    yaw_ = yaw;
    down_ = true;  // A is pressed while resting on the surface
    reset_motion();
    last_ = pos;
    have_last_ = true;
}

void SurfaceTracker::reset_motion()
{
    have_last_ = false;
    filt_r_ = filt_f_ = 0;
    rem_x_ = rem_y_ = 0;
}

MouseDelta SurfaceTracker::update(const Vec3& pos, bool pose_valid)
{
    MouseDelta out;
    if (!calibrated_)
        return out;

    if (!pose_valid) {
        // Lost tracking: behave as if lifted so re-acquiring never causes a jump.
        down_ = false;
        reset_motion();
        return out;
    }

    height_mm_ = (pos.y - surface_y_) * 1000.0;

    // Hysteresis between landing and lifting.
    if (down_ && height_mm_ > cfg_.lift_mm) {
        down_ = false;
        reset_motion();
    } else if (!down_ && height_mm_ < cfg_.land_mm) {
        down_ = true;
        reset_motion();
    }

    if (!down_)
        return out;

    if (!have_last_) {
        last_ = pos;
        have_last_ = true;
        return out;
    }

    double wx = pos.x - last_.x;
    double wz = pos.z - last_.z;
    last_ = pos;

    // Rotate the world-space delta into the calibrated frame.
    // forward = (-sin yaw, -cos yaw), right = (cos yaw, -sin yaw) in (x, z).
    double s = std::sin(yaw_), c = std::cos(yaw_);
    double right = wx * c - wz * s;
    double fwd = -wx * s - wz * c;

    double a = cfg_.smoothing;
    filt_r_ = a * filt_r_ + (1.0 - a) * right;
    filt_f_ = a * filt_f_ + (1.0 - a) * fwd;

    if (std::hypot(filt_r_, filt_f_) * 1000.0 < cfg_.deadzone_mm)
        return out;

    double mx = filt_r_ * cfg_.counts_per_meter * (cfg_.invert_x ? -1 : 1);
    double my = -filt_f_ * cfg_.counts_per_meter * (cfg_.invert_y ? -1 : 1);

    rem_x_ += mx;
    rem_y_ += my;
    out.dx = static_cast<int>(std::trunc(rem_x_));
    out.dy = static_cast<int>(std::trunc(rem_y_));
    rem_x_ -= out.dx;
    rem_y_ -= out.dy;
    return out;
}

double yaw_from_matrix(const float m[3][4])
{
    // Forward axis of the device is -Z: column 2 negated. Project onto the XZ plane.
    double fx = -m[0][2];
    double fz = -m[2][2];
    // forward = (-sin yaw, -cos yaw)  =>  yaw = atan2(-fx, -fz)
    return std::atan2(-fx, -fz);
}
