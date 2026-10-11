#pragma once

#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class PlacementId;
class PoseHistoryPath;
}  // namespace al
class ActorStateSupportFreeze;
class GhostMiiNameplate;
class GhostPlayerLoaderBase;
class GhostPresentBox;
struct GhostPlayerDisplayInfo;

/** @brief Replays recorded ghost play data (time attack rival or Miiverse-style ghost). */
class GhostPlayer : public al::LiveActor {
public:
    /** @brief What the replayed ghost is currently doing, derived from its action name. */
    enum class State : s32 {
        Normal = 0,
        KouraRiding = 1,
        Jump = 2,
        EnterDoor = 3,
        ExitDoor = 4,
    };

    GhostPlayer(const char* pName, s32 index, bool isTimeAttack, bool isEnablePresent);
    /** @brief Destroys the ghost player. */
    ~GhostPlayer() override = default;

    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void makeActorAppeared() override;
    void makeActorDead() override;
    void kill() override;
    void startClipped() override;
    bool initGhostPlayData(GhostPlayerLoaderBase* pLoader, s32 dataIndex);
    void calcAnim() override;
    void control() override;
    void exeWaitStart();
    void hideGhost(bool isForce);
    void tryStartFromCheckpointFlag(const al::PlacementId* pPlacementId);
    void exeWait();
    void exeWaitRestartPlay();
    void exePlay();
    void stopAndHide(bool isForce);
    void showGhost(bool isEnableEffect);
    void exeSupportFreeze();
    void exeStop();
    void tryStartFromObj(const char* pObjName);
    void restart();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

private:
    /** @brief Steps to wait before (re)starting, staggered by ghost index. */
    s32 getStartWaitStep() const { return (mIndex % 3) * 45 + 90; }

    u8* mPlayData = nullptr;
    s32 mIndex;
    bool mIsTimeAttack;
    const GhostPlayerDisplayInfo* mDisplayInfo = nullptr;
    GhostMiiNameplate* mNameplate = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    bool mIsEnablePresent;
    GhostPresentBox* mPresentBox = nullptr;
    s32 mFrame = 0;
    al::PoseHistoryPath* mPoseHistoryPath = nullptr;
    sead::FixedSafeString<64> mActionName;
    s32 _1e8;
    State mState = State::Normal;
    s32 mPresentStartDelay = 0;
    bool mIsHidden = true;
    s32 mShowDelay = 0;
    bool mIsActionEnd = false;
};

static_assert(sizeof(GhostPlayer) == 0x200);
