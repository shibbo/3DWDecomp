#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <prim/seadSafeString.h>

namespace al {
class PlacementId;
class PoseHistoryPath;
}
class GhostPlayerLoaderBase;
class GhostPlayerDisplayInfo;
class GhostMiiNameplate;
class GhostPresentBox;
class ActorStateSupportFreeze;

class GhostPlayer : public al::LiveActor {
public:
    GhostPlayer(const char* pName, int playerIndex, bool isShadow, bool hasPresent);
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void makeActorAppeared() override;
    void makeActorDead() override;
    void kill() override;
    void startClipped() override;
    bool initGhostPlayData(GhostPlayerLoaderBase*, int);
    void calcAnim() override;
    void control() override;
    void exeWaitStart();
    void hideGhost(bool);
    void tryStartFromCheckpointFlag(const al::PlacementId*);
    void exeWait();
    void exeWaitRestartPlay();
    void exePlay();
    void stopAndHide(bool);
    void showGhost(bool);
    void exeSupportFreeze();
    void exeStop();
    void tryStartFromObj(const char*);
    void restart();
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*,
                               al::ScreenPointTarget*) override;

private:
    u8* mPlayData = nullptr;                         // 0x148
    int mPlayerIndex;                               // 0x150
    bool mIsShadow;                                 // 0x154
    const GhostPlayerDisplayInfo* mDisplayInfo = nullptr;
    GhostMiiNameplate* mNameplate = nullptr;         // 0x160
    ActorStateSupportFreeze* mSupportFreeze = nullptr;
    bool mHasPresent;                               // 0x170
    GhostPresentBox* mPresentBox = nullptr;          // 0x178
    int mFrame = 0;                                 // 0x180
    al::PoseHistoryPath* mPoseHistory = nullptr;      // 0x188
    sead::FixedSafeString<64> mActionName;            // 0x190
    u32 _1e8;
    int mActionType = 0;                            // 0x1ec
    int mAppearTimer = 0;                           // 0x1f0
    bool mIsHidden = true;                          // 0x1f4
    int mHideTimer = 0;                             // 0x1f8
    bool mIsInWarp = false;
};

static_assert(sizeof(GhostPlayer) == 0x200);
