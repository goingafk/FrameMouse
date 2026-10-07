#include "vr_input.h"

#include <openvr.h>

#include <cstdio>
#include <unistd.h>

namespace {

const char* kAppKey = "framemouse.overlay";

bool check(vr::EVRInputError err, const char* what)
{
    if (err != vr::VRInputError_None) {
        std::fprintf(stderr, "framemouse: %s failed (EVRInputError %d)\n", what, static_cast<int>(err));
        return false;
    }
    return true;
}

bool digital(vr::VRActionHandle_t h, bool* rising = nullptr, bool* active = nullptr)
{
    vr::InputDigitalActionData_t d{};
    if (vr::VRInput()->GetDigitalActionData(h, &d, sizeof(d), vr::k_ulInvalidInputValueHandle) != vr::VRInputError_None)
        return false;
    if (rising) *rising = d.bActive && d.bState && d.bChanged;
    if (active) *active = d.bActive;
    return d.bActive && d.bState;
}

}  // namespace

VrInput::~VrInput() { shutdown(); }

bool VrInput::init(const std::string& actions_dir, bool exclusive, bool use_tip_pose)
{
    exclusive_ = exclusive;

    vr::EVRInitError ierr = vr::VRInitError_None;
    vr::VR_Init(&ierr, vr::VRApplication_Overlay);
    if (ierr != vr::VRInitError_None) {
        std::fprintf(stderr, "framemouse: VR_Init failed: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(ierr));
        return false;
    }
    initialized_ = true;

    // Register a temporary app manifest so SteamVR associates our bindings with a stable key.
    std::string manifest = actions_dir + "/framemouse.vrmanifest";
    vr::EVRApplicationError aerr = vr::VRApplications()->AddApplicationManifest(manifest.c_str(), true);
    if (aerr == vr::VRApplicationError_None)
        aerr = vr::VRApplications()->IdentifyApplication(static_cast<uint32_t>(getpid()), kAppKey);
    if (aerr != vr::VRApplicationError_None)
        std::fprintf(stderr, "framemouse: app manifest registration: %s (continuing)\n",
                     vr::VRApplications()->GetApplicationsErrorNameFromEnum(aerr));

    if (exclusive_) {
        // Overlay global priority only works with this developer setting enabled.
        vr::EVRSettingsError serr = vr::VRSettingsError_None;
        bool on = vr::VRSettings()->GetBool(vr::k_pch_SteamVR_Section, "globalActionSetPriority", &serr);
        if (!on) {
            vr::VRSettings()->SetBool(vr::k_pch_SteamVR_Section, "globalActionSetPriority", true, &serr);
            std::fprintf(stderr, "framemouse: enabled SteamVR setting steamvr/globalActionSetPriority%s\n",
                         serr == vr::VRSettingsError_None ? "" : " (FAILED)");
        }
    }

    std::string actions = actions_dir + "/actions.json";
    vr::IVRInput* in = vr::VRInput();
    return check(in->SetActionManifestPath(actions.c_str()), "SetActionManifestPath")
        && check(in->GetActionSetHandle("/actions/mouse", &set_), "GetActionSetHandle")
        && check(in->GetActionHandle("/actions/mouse/in/left_click", &left_), "GetActionHandle left_click")
        && check(in->GetActionHandle("/actions/mouse/in/right_click", &right_), "GetActionHandle right_click")
        && check(in->GetActionHandle("/actions/mouse/in/calibrate", &calib_), "GetActionHandle calibrate")
        && check(in->GetActionHandle(use_tip_pose ? "/actions/mouse/in/pose_tip" : "/actions/mouse/in/pose_raw",
                                     &pose_), "GetActionHandle pose");
}

void VrInput::shutdown()
{
    if (initialized_) {
        vr::VR_Shutdown();
        initialized_ = false;
    }
}

bool VrInput::poll(ControllerState& out)
{
    out = {};

    vr::VREvent_t ev;
    while (vr::VRSystem()->PollNextEvent(&ev, sizeof(ev))) {
        if (ev.eventType == vr::VREvent_Quit) {
            vr::VRSystem()->AcknowledgeQuit_Exiting();
            return false;
        }
    }

    vr::VRActiveActionSet_t active{};
    active.ulActionSet = set_;
    active.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
    active.nPriority = exclusive_ ? vr::k_nActionSetOverlayGlobalPriorityMin : 0;
    vr::VRInput()->UpdateActionState(&active, sizeof(active), 1);

    out.left = digital(left_, nullptr, &out.buttons_active);
    out.right = digital(right_);
    digital(calib_, &out.calibrate_pressed);

    vr::InputPoseActionData_t pose{};
    if (vr::VRInput()->GetPoseActionDataForNextFrame(pose_, vr::TrackingUniverseStanding, &pose, sizeof(pose),
                                                     vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None
        && pose.bActive && pose.pose.bPoseIsValid) {
        const auto& m = pose.pose.mDeviceToAbsoluteTracking.m;
        out.pose_valid = true;
        out.pos = {m[0][3], m[1][3], m[2][3]};
    } else {
        // SteamVR only activates an overlay's actions when it gets input; the tracked
        // device pose is always available, so fall back to the right hand's raw pose.
        vr::TrackedDeviceIndex_t idx =
            vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
        if (idx != vr::k_unTrackedDeviceIndexInvalid) {
            vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
            vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, poses, idx + 1);
            if (poses[idx].bPoseIsValid) {
                const auto& m = poses[idx].mDeviceToAbsoluteTracking.m;
                out.pose_valid = true;
                out.pos = {m[0][3], m[1][3], m[2][3]};
            }
        }
    }

    // The headset's facing direction defines "forward" (screen up) at calibration time.
    vr::TrackedDevicePose_t hmd{};
    vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, &hmd, 1);
    if (hmd.bPoseIsValid) {
        out.hmd_valid = true;
        out.hmd_yaw = yaw_from_matrix(hmd.mDeviceToAbsoluteTracking.m);
    }
    return true;
}
