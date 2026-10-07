#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace {

const char* kUsage =
    "Usage: framemouse [options]\n"
    "Turn the right Steam Frame controller into a desk mouse.\n"
    "Rest it on a flat surface and press A to set the surface.\n"
    "Bumper = left click, Trigger = right click.\n"
    "\n"
    "Options (each also settable as key=value in the config file):\n"
    "  --sensitivity N    mouse counts per metre of travel (default 4000)\n"
    "  --land-mm N        height above surface counted as touching (default 8)\n"
    "  --lift-mm N        height above surface counted as lifted (default 15)\n"
    "  --deadzone-mm N    per-frame jitter deadzone (default 0.15)\n"
    "  --smoothing N      low-pass factor 0..0.95 (default 0.5)\n"
    "  --invert-x / --invert-y\n"
    "  --pose raw|tip     which controller pose to track (default raw)\n"
    "  --no-exclusive     do not take bumper/trigger/A away from other apps\n"
    "  --rate N           update rate in Hz (default 250)\n"
    "  --debug            print tracking state 10x per second\n"
    "  --setup-steamvr    turn on the SteamVR setting FrameMouse needs, then exit\n"
    "  --config PATH      config file (default ~/.config/framemouse/framemouse.conf)\n"
    "  -h, --help\n";

bool parse_bool(const std::string& v)
{
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

// Apply one key/value. Returns false for unknown keys or bad values.
bool apply(Options& o, const std::string& key, const std::string& val)
{
    char* end = nullptr;
    auto num = [&](double& dst) {
        double d = std::strtod(val.c_str(), &end);
        if (val.empty() || *end) return false;
        dst = d;
        return true;
    };
    TrackerConfig& t = o.tracker;
    if (key == "sensitivity") return num(t.counts_per_meter);
    if (key == "land-mm") return num(t.land_mm);
    if (key == "lift-mm") return num(t.lift_mm);
    if (key == "deadzone-mm") return num(t.deadzone_mm);
    if (key == "smoothing") {
        if (!num(t.smoothing)) return false;
        if (t.smoothing < 0) t.smoothing = 0;
        if (t.smoothing > 0.95) t.smoothing = 0.95;
        return true;
    }
    if (key == "invert-x") { t.invert_x = parse_bool(val); return true; }
    if (key == "invert-y") { t.invert_y = parse_bool(val); return true; }
    if (key == "exclusive") { o.exclusive = parse_bool(val); return true; }
    if (key == "debug") { o.debug = parse_bool(val); return true; }
    if (key == "pose") {
        if (val != "raw" && val != "tip") return false;
        o.pose = val;
        return true;
    }
    if (key == "rate") {
        double r;
        if (!num(r) || r < 10 || r > 1000) return false;
        o.rate_hz = static_cast<int>(r);
        return true;
    }
    return false;
}

std::string trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

bool load_file(Options& o, const std::string& path, bool must_exist)
{
    std::ifstream in(path);
    if (!in) {
        if (must_exist) std::fprintf(stderr, "framemouse: cannot read config %s\n", path.c_str());
        return !must_exist;
    }
    std::string line;
    int n = 0;
    while (std::getline(in, line)) {
        ++n;
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) continue;
        size_t eq = line.find('=');
        std::string key = trim(line.substr(0, eq));
        std::string val = eq == std::string::npos ? "true" : trim(line.substr(eq + 1));
        for (char& c : key)
            if (c == '_') c = '-';
        if (!apply(o, key, val)) {
            std::fprintf(stderr, "framemouse: %s:%d: bad setting '%s'\n", path.c_str(), n, line.c_str());
            return false;
        }
    }
    return true;
}

}  // namespace

bool load_options(int argc, char** argv, Options& opt, int& exit_code)
{
    exit_code = 0;
    std::string path;
    bool explicit_path = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            std::fputs(kUsage, stdout);
            return false;
        }
        if (a == "--config" && i + 1 < argc) {
            path = argv[i + 1];
            explicit_path = true;
        }
    }
    if (!explicit_path) {
        const char* xdg = std::getenv("XDG_CONFIG_HOME");
        const char* home = std::getenv("HOME");
        if (xdg && *xdg) path = std::string(xdg) + "/framemouse/framemouse.conf";
        else if (home) path = std::string(home) + "/.config/framemouse/framemouse.conf";
    }
    if (!path.empty() && !load_file(opt, path, explicit_path)) {
        exit_code = 2;
        return false;
    }
    opt.config_path = path;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--", 0) != 0) {
            std::fprintf(stderr, "framemouse: unexpected argument '%s'\n%s", a.c_str(), kUsage);
            exit_code = 2;
            return false;
        }
        std::string key = a.substr(2);
        if (key == "config") { ++i; continue; }
        std::string val;
        if (key == "setup-steamvr") { opt.setup_only = true; continue; }
        if (key == "invert-x" || key == "invert-y" || key == "debug") val = "true";
        else if (key == "no-exclusive") { key = "exclusive"; val = "false"; }
        else if (i + 1 < argc) val = argv[++i];
        if (!apply(opt, key, val)) {
            std::fprintf(stderr, "framemouse: bad option '%s'\n%s", a.c_str(), kUsage);
            exit_code = 2;
            return false;
        }
    }
    if (opt.tracker.lift_mm <= opt.tracker.land_mm) {
        std::fprintf(stderr, "framemouse: lift-mm must be greater than land-mm\n");
        exit_code = 2;
        return false;
    }
    return true;
}
