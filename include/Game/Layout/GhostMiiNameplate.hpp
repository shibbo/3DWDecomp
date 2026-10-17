#pragma once

#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"

namespace al {
class LiveActor;
class LayoutInitInfo;
}

class GhostMiiNameplate : public al::SimpleLayoutAppearWaitEnd {
public:
    GhostMiiNameplate(al::LiveActor*, const al::LayoutInitInfo&);
    void init(const char16_t*, bool, bool);
    void appear() override;
    void control() override;
    bool isHostBehindCamera() const;
    al::LiveActor* getHost() const { return mHost; }

private:
    al::LiveActor* mHost;
    const char16_t* mUserName = nullptr;
    bool mIsFriend = false;
    bool mIsShadow = false;
};

namespace GhostMiiNameplateFunction {
void updateShowHideFromCameraDistance(GhostMiiNameplate*, bool);
}

static_assert(sizeof(GhostMiiNameplate) == 0x140);
