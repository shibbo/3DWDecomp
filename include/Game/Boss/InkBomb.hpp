#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class BombStateExplosion;

/** @brief Ink bomb thrown by Fury Bowser that leaves ink puddles behind. */
class InkBomb : public al::LiveActor {
public:
    /** @brief How a launched bomb moves before it settles. */
    enum Type {
        Type_Roll = 0,
        Type_Bound = 1,
    };

    explicit InkBomb(const char* pName, const al::LiveActor* pHost = nullptr);
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool goExplosion();
    bool canKicked(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) const;
    bool canBoomerangHit() const;
    void setupEffectMaterial();
    void startLandHitReaction();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void updateCollider() override;
    void reload(bool isBlink);
    void resetInner();
    void launch(Type type, const sead::Vector3f& rVelocity, bool isStartFuse, bool isUseGravity);
    void thrown(Type type, const sead::Vector3f& rVelocity, f32 gravity, bool isStartFuse,
                bool isKeepVelocity);
    void vanish();
    bool isExplodingOrKicked() const;
    f32 getSpeedH() const;
    void exeBound();
    void exeRoll();
    void endRoll();
    void exeKicked();
    void exeStop();
    void exeExplosion();
    void exeBreak();
    void exePlaced();
    void endPlaced();

    /** @brief Waits in the generator until launched. */
    void exeWait() {}

    /**
     * @brief Sets the actor that threw the bomb.
     * @param pHost Throwing actor.
     */
    void setHost(const al::LiveActor* pHost) { mHost = pHost; }

private:
    const al::LiveActor* mHost;                          // 0x148
    BombStateExplosion* mStateExplosion = nullptr;       // 0x150
    BombStateExplosion* mStateExplosionNoDamage = nullptr;  // 0x158
    BombStateExplosion* mStateBreak = nullptr;           // 0x160
    al::HitSensor* mKickerSensor = nullptr;              // 0x168
    s32 mControlUserId = -1;                             // 0x170
    s32 mFuseTimer = 0;                                  // 0x174
    Type mType = Type_Roll;                              // 0x178
    f32 mSpeedH = 10.0f;                                 // 0x17c
    sead::Vector3f mSmokePos = sead::Vector3f::zero;     // 0x180
    s32 mCollideStartStep = 5;                           // 0x18c
    f32 mGravity = 0.0f;                                 // 0x190
    f32 mGravityRate = 1.0f;                             // 0x194
    s32 mStrongKickGuardTimer = 0;                       // 0x198
    s32 mLandReactionTimer = 0;                          // 0x19c
    bool mIsInWater = false;                             // 0x1a0
};
static_assert(sizeof(InkBomb) == 0x1a8);
