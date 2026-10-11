#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>

namespace al {
    class AreaObjGroup;
};

class FlyerStateChase;
class FlyerStateFindPlayer;
class TargetFinder;
class FlyerStateReturnArea;

/** @brief Boo enemy (and Big Boo): chases the player and hides its face when looked at. */
class Teresa : public al::LiveActor {
public:
    Teresa(const char* pName);

    virtual ~Teresa() = default;
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void kill();
    virtual void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf);
    virtual bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget);
    virtual void control();

    void killBySwitch();
    void appearBySwitch();
    void disappearBySwitch();
    void exeAppear();
    void changeNerveByTarget(TargetFinder* pTargetFinder, al::AreaObjGroup* pArea);
    void exeWait();
    void exeFindPlayer();
    void exeChase();
    void exeReturnArea();
    void exeShyStart();
    void exeShyWait();
    bool isPlayerSight();
    void exeShyEndSign();
    void exeShyEnd();
    void deleteShyEffect();
    void exeDamageLight();
    void exeReaction();
    void exeReactionWait();
    void exeReactionAppear();
    void exeDisappearOutArea();
    void exeAttackHit();
    void exeDown();

private:
    bool mIsReceiveLight = false;                      // 0x144
    bool mIsBig = false;                               // 0x145
    bool mIsTouchAssist = false;                       // 0x146
    bool mIsHeadlightFlash = false;                    // 0x147
    f32 mAlpha = 1.0f;                                 // 0x148
    f32 mShadowIntensity = 1.0f;                       // 0x14C
    s32 mDisappearReactionTime = 0;                    // 0x150
    sead::Vector3f mInitTrans = {0.0f, 0.0f, 0.0f};    // 0x154
    FlyerStateChase* mStateChase = nullptr;            // 0x160
    FlyerStateFindPlayer* mStateFindPlayer = nullptr;  // 0x168
    FlyerStateReturnArea* mStateReturnArea = nullptr;  // 0x170
    TargetFinder* mTargetFinder = nullptr;             // 0x178
    al::AreaObjGroup* mLinkAreaGroup = nullptr;        // 0x180
    al::HitSensor* mLightSensor = nullptr;             // 0x188
};

static_assert(sizeof(Teresa) == 0x190);
