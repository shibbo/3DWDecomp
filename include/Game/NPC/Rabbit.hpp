#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CameraTicket;
class Nerve;
}

class ActorStateSupportFreeze;
class BgmBeatAnimeController;
class Bush;
class CoinBlowGenerator;
class GoalItem;
class GreenStar;
class ItemStatePopUpFrontParam;
class KinokoBig;
class RabbitInitPlacePoint;
class RabbitRoute;
class RabbitRouteRider;
class RabbitStateReverse;
class RabbitStateSwoon;

/**
 * @brief The rabbit that runs away along its route and drops an item when caught.
 */
class Rabbit : public al::LiveActor {
public:
    Rabbit(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void appear() override;
    void makeActorAppeared() override;
    void startClipped() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;

    void goalComplete();
    void cancel();
    bool isStateJump() const;
    bool isRabbitOnGround();
    void moveOnRoute(f32 speed);
    void faceToCamera();
    bool tryStartRun(const al::Nerve* pNerve, bool isTurn);
    bool isPlayerNear(const al::LiveActor* pPlayer);
    bool canStartReverse(const al::LiveActor* pPlayer) const;
    bool tryStartReverse(const al::LiveActor* pPlayer, const al::Nerve* pNerve);
    bool tryAppearKinokoBig(s32 step);
    f32 getMoveEndDistance() const;

    void exeHide();
    void exeAppear();
    void exeAppearWait();
    void exeWait();
    void exeWaitAtInitPoint();
    void exeFindAtInitPoint();
    void exeRunBreak();
    void exeFind();
    void exeRun();
    void exeJumpStart();
    void exeJump();
    void exeReverse();
    void exeSwoon();
    void exeCatch();
    void exeDisappear();
    void exeDisappearGoalItem();
    void exeSupportFreeze();
    void exePoof();

private:
    bool isScenarioComplete() const;
    void tryEmitItemAvailableEffect();
    void hideGuide();
    void startSwoon(const al::SensorMsg* pMsg);
    void startReverse(const al::Nerve* pNextNerve);
    void startReverseNoStop(const al::Nerve* pNextNerve);

    RabbitRoute* mRoute = nullptr;
    RabbitRouteRider* mRouteRider = nullptr;
    RabbitStateReverse* mStateReverse = nullptr;
    RabbitStateSwoon* mStateSwoon = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    const al::Nerve* mNerveAfterSupportFreeze = nullptr;
    CoinBlowGenerator* mCoinBlowGenerator = nullptr;
    GreenStar* mGreenStar = nullptr;
    GoalItem* mGoalItem = nullptr;
    Bush* mBush = nullptr;
    bool mIsBig = false;
    bool mIsAppearItemQuickly = false;
    RabbitInitPlacePoint* mInitPlacePoint = nullptr;
    f32 mAppearDistance = 1200.0f;
    f32 mMoveStartDistance = 1000.0f;
    f32 mMoveStartDistancePlessie = 1000.0f;
    f32 mMoveEndDistanceOffset = 300.0f;
    f32 mSpeed = 12.0f;
    al::HitSensor* mCatchSensor = nullptr;
    bool mIsCaught = false;
    bool mIsSwoonRequested = false;
    s32 mReverseCoolTime = 0;
    const al::Nerve* mNerveAfterReverse = nullptr;
    sead::FixedPtrArray<KinokoBig, 4> mKinokoBigs;
    ItemStatePopUpFrontParam* mKinokoBigPopUpParam;
    bool mIsBeatAnime = false;
    s32 mGuideTimer = -1;
    f32 mReverseStartDistance = 1000.0f;
    s32 mDisappearStep;
    bool mIsStopOnGroundOnly = false;
    bool mIsSingleMode = false;
    bool mIsItemAvailableEffect = false;
    s32 mJumpStartStep = 0;
    sead::Vector3f mGoalCameraPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mGoalCameraAt = {0.0f, 0.0f, 0.0f};
    al::CameraTicket* mGoalCamera = nullptr;
    s32 mIslandId = -1;
    s32 mScenarioId = -1;
    BgmBeatAnimeController* mBeatAnimeController = nullptr;
};

static_assert(sizeof(Rabbit) == 0x258);
