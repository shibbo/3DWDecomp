#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>

namespace al {
class AreaObjGroup;
}

class ActorStateSupportFreeze;
class FlyerStateChase;
class FlyerStateFindPlayer;
class FlyerStateReturnArea;
class FlyerStateWander;
class TargetFinder;
class TargetFinderParam;

/** @brief Flying Dry Bones (Para-Bones): wanders and chases the player, falls apart when hit. */
class KaronWing : public al::LiveActor {
public:
    KaronWing(const char* pName);

    virtual ~KaronWing() = default;
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void control();
    virtual void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf);
    virtual bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget);

    void killBySwitch();
    void exeWander();
    void resetEyes();
    void exeFindPlayer();
    void moveEyesToPlayer();
    void exeChase();
    void exeReturnArea();
    void exeTrampled();
    void exeBreak();
    void exeBreakGroundHit();
    void exeBreakWait();
    void exeBreakReaction();
    void exeRecoverSign();
    void exeRecover();
    void exeAttackHit();
    void exeAttackHitWait();
    void exeReaction();
    void exeBreakDown();
    void exeSupportFreeze();

private:
    bool mIsAppearItem = true;                            // 0x144
    f32 mInitHeight = 0.0f;                               // 0x148
    sead::Vector3f mEyeRotate = {0.0f, 0.0f, 0.0f};       // 0x14C
    FlyerStateChase* mStateChase = nullptr;               // 0x158
    FlyerStateFindPlayer* mStateFindPlayer = nullptr;     // 0x160
    FlyerStateWander* mStateWander = nullptr;             // 0x168
    FlyerStateReturnArea* mStateReturnArea = nullptr;     // 0x170
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;  // 0x178
    TargetFinder* mTargetFinder = nullptr;                // 0x180
    TargetFinderParam* mTargetFinderParam = nullptr;      // 0x188
    al::AreaObjGroup* mLinkAreaGroup = nullptr;           // 0x190
    sead::Vector3f mInitTrans = sead::Vector3f::zero;     // 0x198
    sead::Vector3f mInitFront = sead::Vector3f::ez;       // 0x1A4
    bool mIsRestored = false;                             // 0x1B0
    bool mIsSingleMode = false;                           // 0x1B1
};

static_assert(sizeof(KaronWing) == 0x1b8);
