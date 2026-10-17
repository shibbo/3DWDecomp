#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Archive, action and timing settings shared by a set of EnemyEffectBullet actors. */
class EnemyEffectBulletInfo {
public:
    EnemyEffectBulletInfo(const char* pArchiveName, s32 lifeFrame, const char* pBodyEffect,
                          const char* pAppearEffect, const char* pDisappearEffect,
                          const char* pFlySe, const char* pVanishSe, const char* pSuffix,
                          bool isFire, bool isHoming, s32 homingStartFrame, f32 maxTurnDegree, f32 turnAcceleration);

    const char* mArchiveName;
    const char* mSuffix;
    s32 mLifeFrame;
    const char* mBodyEffect;
    const char* mAppearEffect;
    const char* mDisappearEffect;
    const char* mFlySe;
    const char* mVanishSe;
    bool mIsFire;
    bool mIsHoming;
    s32 mHomingStartFrame;
    f32 mMaxTurnDegree;
    f32 mTurnAcceleration;
};

static_assert(sizeof(EnemyEffectBulletInfo) == 0x50);

/** @brief A simple effect-driven bullet shot by enemies. */
class EnemyEffectBullet : public al::LiveActor {
public:
    EnemyEffectBullet(const char* pName, const EnemyEffectBulletInfo* pInfo);

    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    /** @brief Gets the effect's pose matrix. @return Current effect pose. */
    const sead::Matrix34f* getBaseMtx() const override { return &mEffectMtx; }
    void shot(const sead::Vector3f& rDir, f32 speed);

private:
    const EnemyEffectBulletInfo* mInfo;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    s32 mLifeStep = 0;
    al::LiveActor* mTarget = nullptr;
    f32 mTurnDegree = 0.0f;
};

static_assert(sizeof(EnemyEffectBullet) == 0x198);
