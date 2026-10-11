#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ComboCounter;
}  // namespace al
class ActorStateSupportFreeze;
class Bomb;
class BombStateExplosion;
class TargetFinder;
class WalkerStateChase;
class WalkerStateFindPlayer;
class WalkerStateWander;

/**
 * @brief Bob-omb: a wind-up bomb enemy that wanders, lights its fuse when it finds a player and
 * explodes after a countdown. Stomping or attacking it turns it into a carryable Bomb.
 */
class BombHei : public al::LiveActor {
public:
    explicit BombHei(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void makeActorAppeared() override;
    void killBySwitch();
    void killForRespawn();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isLaunching() const;
    void changeToBomb(const al::SensorMsg* pMsg, al::HitSensor* pOther);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void appearLaunchReady(const al::LiveActor* pLauncher);
    void appearGenerate(const al::LiveActor* pGenerator);
    void launch();
    bool isDeadWithBomb() const;
    bool tryStartCountDown();
    bool updateCountDown();
    bool tryExplosionByAreaOrMaterialCode();
    void rotateSpring(f32 speed);

    void exeWander();
    void exeFindPlayer();
    void exeChase();
    void exeJump();
    void exeExplosion();
    void exeSupportFreeze();
    void exeLaunchReady();
    void exeLaunch();
    void exeLaunchLand();
    void exeGenerate();

private:
    TargetFinder* mTargetFinder = nullptr;                   // 0x148
    WalkerStateWander* mStateWander = nullptr;               // 0x150
    WalkerStateFindPlayer* mStateFindPlayer = nullptr;       // 0x158
    WalkerStateChase* mStateChase = nullptr;                 // 0x160
    BombStateExplosion* mStateExplosion = nullptr;           // 0x168
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;  // 0x170
    Bomb* mBomb = nullptr;                                   // 0x178
    s32 mCountDown = -1;                                     // 0x180
    f32 mSpringAngle = 0.0f;                                 // 0x184
    al::ComboCounter* mComboCounter = nullptr;               // 0x188
    s32 mControlUserId = -1;                                 // 0x190
    sead::Vector3f mInitTrans = sead::Vector3f::zero;        // 0x194
    sead::Vector3f mInitFront = sead::Vector3f::ez;          // 0x1a0
    bool mIsRestored = false;                                // 0x1ac
    bool mIsSingleMode = false;                              // 0x1ad
};

static_assert(sizeof(BombHei) == 0x1b0);
