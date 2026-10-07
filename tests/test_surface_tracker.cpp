#include "surface_tracker.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

TrackerConfig plain()
{
    TrackerConfig c;
    c.counts_per_meter = 1000.0;  // 1 count per mm
    c.smoothing = 0.0;
    c.deadzone_mm = 0.0;
    return c;
}

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            std::exit(1);                                                    \
        }                                                                    \
    } while (0)

void idle_until_calibrated()
{
    SurfaceTracker t(plain());
    MouseDelta d = t.update({0.1, 0.8, 0}, true);
    CHECK(d.dx == 0 && d.dy == 0);
    CHECK(!t.calibrated());
}

void moves_on_surface()
{
    // yaw 0: forward is -Z, right is +X
    SurfaceTracker t(plain());
    t.calibrate({0, 0.8, 0}, 0.0);
    MouseDelta d = t.update({0.010, 0.8, 0}, true);  // 10 mm right
    CHECK(d.dx == 10 && d.dy == 0);
    d = t.update({0.010, 0.8, -0.005}, true);        // 5 mm forward -> screen up (negative y)
    CHECK(d.dx == 0 && d.dy == -5);
}

void lift_and_land_hysteresis()
{
    SurfaceTracker t(plain());  // land 8 mm, lift 15 mm
    t.calibrate({0, 0.8, 0}, 0.0);
    t.update({0, 0.812, 0}, true);  // 12 mm: still down (below lift threshold)
    CHECK(t.down());
    t.update({0, 0.820, 0}, true);  // 20 mm: lifted
    CHECK(!t.down());
    t.update({0, 0.812, 0}, true);  // 12 mm: still lifted (above land threshold)
    CHECK(!t.down());
    t.update({0, 0.805, 0}, true);  // 5 mm: down again
    CHECK(t.down());
}

void no_motion_while_lifted_and_no_jump_on_land()
{
    SurfaceTracker t(plain());
    t.calibrate({0, 0.8, 0}, 0.0);
    t.update({0, 0.83, 0}, true);                    // lift
    MouseDelta d = t.update({0.2, 0.83, 0}, true);   // move 200 mm in the air
    CHECK(d.dx == 0 && d.dy == 0);
    d = t.update({0.2, 0.80, 0}, true);              // land somewhere else
    CHECK(d.dx == 0 && d.dy == 0);
    d = t.update({0.203, 0.80, 0}, true);            // then 3 mm right
    CHECK(d.dx == 3);
}

void lost_tracking_counts_as_lifted()
{
    SurfaceTracker t(plain());
    t.calibrate({0, 0.8, 0}, 0.0);
    t.update({0, 0.8, 0}, false);
    CHECK(!t.down());
    MouseDelta d = t.update({0.05, 0.8, 0}, true);   // reacquired 50 mm away: no jump
    CHECK(d.dx == 0);
}

void yaw_rotation()
{
    // Facing +X (yaw = -90 deg): moving +X is forward (screen up), moving +Z is right.
    SurfaceTracker t(plain());
    t.calibrate({0, 0.8, 0}, -M_PI / 2);
    MouseDelta d = t.update({0.010, 0.8, 0}, true);
    CHECK(d.dx == 0 && d.dy == -10);
    d = t.update({0.010, 0.8, 0.004}, true);
    CHECK(d.dx == 4 && d.dy == 0);
}

void subcount_accumulation()
{
    SurfaceTracker t(plain());
    t.calibrate({0, 0.8, 0}, 0.0);
    int total = 0;
    for (int i = 1; i <= 10; ++i)
        total += t.update({0.00031 * i, 0.8, 0}, true).dx;  // 0.31 mm per frame
    CHECK(total == 3);
}

void yaw_from_matrix_identity()
{
    float m[3][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}};
    CHECK(std::fabs(yaw_from_matrix(m)) < 1e-9);
    // 90 deg about +Y: forward (-Z) becomes -X
    float r[3][4] = {{0, 0, 1, 0}, {0, 1, 0, 0}, {-1, 0, 0, 0}};
    CHECK(std::fabs(yaw_from_matrix(r) - M_PI / 2) < 1e-6);
}

}  // namespace

int main()
{
    idle_until_calibrated();
    moves_on_surface();
    lift_and_land_hysteresis();
    no_motion_while_lifted_and_no_jump_on_land();
    lost_tracking_counts_as_lifted();
    yaw_rotation();
    subcount_accumulation();
    yaw_from_matrix_identity();
    std::puts("all surface tracker tests passed");
    return 0;
}
