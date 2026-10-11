#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class MtxConnector;
}  // namespace al
class ActorMicRumbler;
class ActorStateSupportFreeze;
class EnemyStateBlowDown;
class PackunFlowerHead;
class PackunStateHold;

/** @brief Potted Piranha Plant: a plant with three heads that can be carried by the player. */
class PackunFlowerWithPot : public al::LiveActor {
public:
    /** @brief The flower pot the plant sits in; it follows the plant and breaks with it. */
    class Pot : public al::LiveActor {
    public:
        Pot(const char* pName, PackunFlowerWithPot* pParent);
        /** @brief Destroys the pot actor. */
        ~Pot() override = default;

        bool hideActor() override;

    private:
        PackunFlowerWithPot* mParent;
    };

    explicit PackunFlowerWithPot(const char* pName);
    /** @brief Destroys the potted Piranha Plant. */
    ~PackunFlowerWithPot() override = default;

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    void setBlowFromWater();
    bool isEnableAttack();
    void makeActorAppeared() override;
    void appear() override;
    void makeActorDead() override;
    bool hideActor() override;
    void kill() override;
    void startRespawn();
    void updateCollider() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isDown();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeSleep();
    void exeFind();
    void exeWait();
    void exeTurn();
    void exeTurnFast();
    void exeAttack();
    void exeAfterAttack();
    void exePush();
    void exeHold();
    void exeRelease();
    void exeLand();
    void exePressDown();
    void exeDieDown();
    void exeBlowDown();
    void exeSupportFreeze();
    void exeRespawn();

    /** @brief Gets the flower pot actor. */
    al::LiveActor* getPot() const { return mPot; }

private:
    void attachToCollision();
    bool isOnGroundCollision() const;
    void emitFallLeafEffect();
    void blowDownByAttack(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    Pot* mPot = nullptr;
    PackunFlowerHead** mHeads = nullptr;
    EnemyStateBlowDown* mStateBlowDown = nullptr;
    al::MtxConnector* mMtxConnector = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    ActorMicRumbler* mMicRumbler = nullptr;
    f32 mTurnDegree = 4.0f;
    al::HitSensor* mHolderSensor = nullptr;
    const sead::Matrix34f* mHeadJointMtx = nullptr;
    PackunStateHold* mStateHold = nullptr;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    bool mIsEnableReset = false;
    bool mIsSingleMode = false;
    bool mIsPushed = false;
    bool mIsHiddenWhileHeld = false;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
    sead::Quatf mInitQuat;
    bool mIsShowGuide = false;
    bool mIsCarryable = false;
};

static_assert(sizeof(PackunFlowerWithPot) == 0x1f0);
