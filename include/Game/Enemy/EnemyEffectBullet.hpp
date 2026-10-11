#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Archive, action and timing settings shared by a set of EnemyEffectBullet actors. */
class EnemyEffectBulletInfo {
public:
    EnemyEffectBulletInfo(const char* pArchiveName, s32 lifeFrame, const char* pSensorName,
                          const char* pAppearAction, const char* pDisappearAction,
                          const char* pFlyAction, const char* pVanishSe, const char* pSuffix,
                          bool isUnk40, bool isUnk41, s32 unk44, f32 unk48, f32 unk4C);

    const char* mArchiveName;
    s32 mLifeFrame;
    const char* mSensorName;
    const char* mAppearAction;
    const char* mDisappearAction;
    const char* mFlyAction;
    const char* mVanishSe;
    const char* mSuffix;
    bool mIsUnk40;
    bool mIsUnk41;
    s32 mUnk44;
    f32 mUnk48;
    f32 mUnk4C;
};

static_assert(sizeof(EnemyEffectBulletInfo) == 0x50);

/** @brief A simple effect-driven bullet shot by enemies. */
class EnemyEffectBullet : public al::LiveActor {
public:
    EnemyEffectBullet(const char* pName, const EnemyEffectBulletInfo* pInfo);

    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void shot(const sead::Vector3f& rDir, f32 speed);

private:
    u8 mUnreconstructed[0x198 - sizeof(al::LiveActor)];
};

static_assert(sizeof(EnemyEffectBullet) == 0x198);
