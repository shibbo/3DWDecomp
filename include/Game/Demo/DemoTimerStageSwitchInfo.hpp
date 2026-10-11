#pragma once

namespace al { struct PlacementInfo; }

/** @brief A stage switch transition scheduled at one demo frame. */
class DemoTimerStageSwitchInfo {
public:
    explicit DemoTimerStageSwitchInfo(const al::PlacementInfo& rInfo);
    const char* mSwitchName = nullptr;
    int mStep = 0;
    bool mIsOn = true;
};
