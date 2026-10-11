#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class JointAimInfo;
class ScreenPointer;
}  // namespace al

class ActorStateSupportFreeze;
class TargetFinder;
class WalkerStateFindPlayer;
class WalkerStateWander;

/** @brief Bump-pushing enemy (Donketsu) that slides away when hit and knocks others back. */
class Donketsu : public al::LiveActor {
public:
    explicit Donketsu(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void killBySwitch();
    void appearBySwitch();
    bool isFallOrDead() const;
    bool trySlideChainPush(al::HitSensor* pSelf, al::HitSensor* pOther);
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableBreak(const al::SensorMsg* pMsg, al::HitSensor* pSelf) const;
    bool isEnableSlide(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                       al::HitSensor* pSelf) const;
    bool isMsgUseFrontDirToSlide(const al::SensorMsg* pMsg) const;
    static f32 calcSlideSpeed(const al::SensorMsg* pMsg);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isEnableSupportFreeze() const;
    bool trySendMsgDonketsuAttackCollide();
    void control() override;

    void exeWander();
    void exeFindPlayer();
    void exeChase();
    void exeChaseEnd();
    void exeSlide();
    void exeStay();
    void exeWallHit();
    void exeWallHitLand();
    void exeFall();
    void exeLand();
    void exeSupportFreeze();
    void exeDead();

private:
    void startSlide(const al::SensorMsg* pMsg);
    void setNerveByTarget();
    void addScoreAndKill();

    al::HitSensor* mLastAttackSensor = nullptr;         // 0x148
    al::ScreenPointer* mLastAttackPointer = nullptr;    // 0x150
    TargetFinder* mTargetFinder = nullptr;              // 0x158
    WalkerStateWander* mStateWander = nullptr;          // 0x160
    void* _168;                                         // 0x168
    WalkerStateFindPlayer* mStateFindPlayer = nullptr;  // 0x170
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;  // 0x178
    al::JointAimInfo* mJointAimInfo = nullptr;          // 0x180
    sead::Vector3f mSlideDir = sead::Vector3f::ez;      // 0x188
    s32 mDamageCoolTime = 0;                            // 0x194
    f32 mFallCheckOffsetY = 0.0f;                       // 0x198
    bool mIsSlideToStay = false;                        // 0x19C
    bool mIsEnableChainPush = false;                    // 0x19D
    bool mIsSingleMode = false;                         // 0x19E
};

static_assert(sizeof(Donketsu) == 0x1a0);
