#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AnimScaleController;
class CameraTicket;
class ClippingAreaActorInfo;
class Nerve;
}  // namespace al

class ActorMicRumbler;
class ActorStateDemoCamera;
class ActorStateDemoCameraParam;
class ActorStateSupportStroke;
class GoalItem;
class GreenStar;
class GuideBalloonBrigade;
class KinopioBrigadeWatcher;

/**
 * @brief A member of the Toad Brigade (キノピオ隊) that waits, waves at the player and hands out
 * a green star or a shine once its switch is turned on.
 *
 * In Bowser's Fury, each member hides somewhere on the island and walks to its discovered
 * location once found; the captain's watcher tracks how many of them were discovered.
 */
class KinopioBrigadeNpc : public al::LiveActor {
public:
    /// Movement type, read from the "State" placement argument.
    enum class State : s32 {
        Move = 0,
        RouteDokan = 1,
        Afraid = 2,
        MoveCheckEnemy = 3,
    };

    KinopioBrigadeNpc(const char* pName);

    void init(const al::ActorInitInfo& rInfo, bool isDiscoveredLocation);
    void moveToDiscoveredLocation(bool isRegisterClipping);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void startClipped() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool canPlaySingleModeReaction() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool canReceiveSingleModeMsg() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    GoalItem* getGoalItem();
    bool isDiscovered() const;
    bool checkDisaster();

    void exeAppear();
    void exeStandBy();
    bool isOnSwitchStartSingleMode();
    void exeReliefStart();
    void exeRelief();
    void exeTurnToCamera();
    void exeTakeOut();
    void exeMove();
    void exeRouteDokan();
    void exeWait();
    void exeWaitTurn();
    void exeTrampled();
    void exeReaction();
    void exeSpinReaction();
    void exeMicReaction();
    void exeTouch();
    void exeDisasterAfraid();

    /** @return Index of the member inside the brigade (0 for the captain). */
    s32 getMemberIndex() const { return mMemberType; }

private:
    bool isGoalItemScenarioComplete() const;
    bool isGivingReward() const;
    void initGuideBalloon(const al::ActorInitInfo& rInfo);
    bool isOnSwitchStartMultiMode();
    bool tryStartReaction();
    bool tryStartSpinReaction();
    void startReactionOrNegative(const al::Nerve* pNerve);
    void setNerveWaitOrWave();

    bool mIsTouched = false;
    bool mIsEnemyNear = false;
    bool mIsScenarioComplete = false;
    bool mIsDiscovered = false;
    State mState = State::Move;
    f32 mSpeed = 10.0f;
    f32 mMoveSpeed = 10.0f;
    s32 mTurnCounter = 0;
    GreenStar* mGreenStar = nullptr;
    GoalItem* mGoalItem = nullptr;
    ActorMicRumbler* mMicRumbler = nullptr;
    ActorStateSupportStroke* mStateSupportStroke = nullptr;
    al::AnimScaleController* mAnimScaleController = nullptr;
    ActorStateDemoCamera* mStateDemoCamera = nullptr;
    ActorStateDemoCameraParam* mDemoCameraParam = nullptr;
    bool mIsInWaveArea = false;
    s32 mMemberType = 0;
    bool mIsSingleMode = false;
    KinopioBrigadeWatcher* mWatcher = nullptr;
    GuideBalloonBrigade* mGuideBalloon = nullptr;
    bool mIsWaitGoalItemCollectSwitch = false;
    s32 mReactionCoolTime = 0;
    s32 mUnusedTimer = 0;
    sead::Vector3f mCameraPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mCameraAt = {0.0f, 0.0f, 0.0f};
    al::CameraTicket* mCameraTicket = nullptr;
    const al::Nerve* mNerveAfterSpinReaction = nullptr;
    sead::Vector3f mDiscoveredTrans = sead::Vector3f::zero;
    KinopioBrigadeNpc* mDiscoveredLocation = nullptr;
    al::ClippingAreaActorInfo* mDiscoveredClippingInfo = nullptr;
};

static_assert(sizeof(KinopioBrigadeNpc) == 0x208);
