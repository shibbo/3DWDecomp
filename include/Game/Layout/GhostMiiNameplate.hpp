#pragma once

#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"

namespace al {
class LiveActor;
class LayoutInitInfo;
}  // namespace al

/** @brief Name plate layout that follows a ghost player actor. */
class GhostMiiNameplate : public al::SimpleLayoutAppearWaitEnd {
public:
    GhostMiiNameplate(al::LiveActor* pActor, const al::LayoutInitInfo& rInfo);

    void init(const char16_t* pName, bool isFriend, bool isTimeAttack);
    void appear() override;
    void control() override;
    bool isHostBehindCamera() const;
    /** @brief Returns the ghost actor followed by the name plate. */
    al::LiveActor* getHost() const { return mHost; }

private:
    al::LiveActor* mHost;
    const char16_t* mName = nullptr;
    bool mIsFriend = false;
    bool mIsTimeAttack = false;
};

namespace GhostMiiNameplateFunction {
void updateShowHideFromCameraDistance(GhostMiiNameplate* pNameplate, bool isClipped);
}  // namespace GhostMiiNameplateFunction
