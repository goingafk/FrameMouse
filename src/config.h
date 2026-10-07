// Runtime options: defaults <- ~/.config/framemouse/framemouse.conf <- CLI flags.
#pragma once

#include "surface_tracker.h"

#include <string>

struct Options {
    TrackerConfig tracker;
    std::string pose = "raw";   // raw | tip | base  (which /pose/* to read)
    bool exclusive = true;      // hide bumper/trigger/A from other apps while running
    int rate_hz = 250;
    bool debug = false;
    bool setup_only = false;    // enable the SteamVR setting we need, then exit
    std::string config_path;    // empty => default location
};

// Returns false (after printing usage/errors) if the program should exit.
bool load_options(int argc, char** argv, Options& opt, int& exit_code);
